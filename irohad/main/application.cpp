/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main/application.hpp"

#include <boost/filesystem.hpp>

#include "ametsuchi/impl/flat_file_block_storage_factory.hpp"
#include "ametsuchi/impl/k_times_reconnection_strategy.hpp"
#include "ametsuchi/impl/storage_impl.hpp"
#include "ametsuchi/impl/tx_presence_cache_impl.hpp"
#include "ametsuchi/impl/wsv_restorer_impl.hpp"
#include "backend/protobuf/common_objects/proto_common_objects_factory.hpp"
#include "backend/protobuf/proto_block_json_converter.hpp"
#include "backend/protobuf/proto_permission_to_string.hpp"
#include "backend/protobuf/proto_proposal_factory.hpp"
#include "backend/protobuf/proto_query_response_factory.hpp"
#include "backend/protobuf/proto_transport_factory.hpp"
#include "backend/protobuf/proto_tx_status_factory.hpp"
#include "common/bind.hpp"
#include "consensus/yac/consistency_model.hpp"
#include "cryptography/crypto_provider/crypto_model_signer.hpp"
#include "interfaces/iroha_internal/transaction_batch_factory_impl.hpp"
#include "interfaces/iroha_internal/transaction_batch_parser_impl.hpp"
#include "logger/logger.hpp"
#include "logger/logger_manager.hpp"
#include "main/impl/consensus_init.hpp"
#include "main/server_runner.hpp"
#include "multi_sig_transactions/gossip_propagation_strategy.hpp"
#include "multi_sig_transactions/mst_processor_impl.hpp"
#include "multi_sig_transactions/mst_propagation_strategy_stub.hpp"
#include "multi_sig_transactions/mst_time_provider_impl.hpp"
#include "multi_sig_transactions/storage/mst_storage_impl.hpp"
#include "multi_sig_transactions/transport/mst_transport_grpc.hpp"
#include "multi_sig_transactions/transport/mst_transport_stub.hpp"
#include "network/impl/block_loader_impl.hpp"
#include "network/impl/peer_communication_service_impl.hpp"
#include "ordering/impl/on_demand_common.hpp"
#include "ordering/impl/on_demand_ordering_gate.hpp"
#include "pending_txs_storage/impl/pending_txs_storage_impl.hpp"
#include "simulator/impl/simulator.hpp"
#include "synchronizer/impl/synchronizer_impl.hpp"
#include "torii/impl/command_service_impl.hpp"
#include "torii/impl/command_service_transport_grpc.hpp"
#include "torii/impl/status_bus_impl.hpp"
#include "torii/processor/query_processor_impl.hpp"
#include "torii/processor/transaction_processor_impl.hpp"
#include "torii/query_service.hpp"
#include "validation/impl/chain_validator_impl.hpp"
#include "validation/impl/stateful_validator_impl.hpp"
#include "validators/default_validator.hpp"
#include "validators/field_validator.hpp"
#include "validators/protobuf/proto_block_validator.hpp"
#include "validators/protobuf/proto_proposal_validator.hpp"
#include "validators/protobuf/proto_query_validator.hpp"
#include "validators/protobuf/proto_transaction_validator.hpp"

using namespace iroha;
using namespace iroha::ametsuchi;
using namespace iroha::simulator;
using namespace iroha::validation;
using namespace iroha::network;
using namespace iroha::synchronizer;
using namespace iroha::torii;
using namespace iroha::consensus::yac;

using namespace std::chrono_literals;

/// Consensus consistency model type.
static constexpr iroha::consensus::yac::ConsistencyModel
    kConsensusConsistencyModel = iroha::consensus::yac::ConsistencyModel::kBft;

/**
 * Configuring iroha daemon
 */
Irohad::Irohad(const std::string &block_store_dir,
               const std::string &pg_conn,
               const std::string &listen_ip,
               size_t torii_port,
               size_t internal_port,
               size_t max_proposal_size,
               std::chrono::milliseconds proposal_delay,
               std::chrono::milliseconds vote_delay,
               std::chrono::minutes mst_expiration_time,
               const shared_model::crypto::Keypair &keypair,
               std::chrono::milliseconds max_rounds_delay,
               size_t stale_stream_max_rounds,
               boost::optional<shared_model::interface::types::PeerList>
                   opt_alternative_peers,
               logger::LoggerManagerTreePtr logger_manager,
               const boost::optional<GossipPropagationStrategyParams>
                   &opt_mst_gossip_params)
    : block_store_dir_(block_store_dir),
      pg_conn_(pg_conn),
      listen_ip_(listen_ip),
      torii_port_(torii_port),
      internal_port_(internal_port),
      max_proposal_size_(max_proposal_size),
      proposal_delay_(proposal_delay),
      vote_delay_(vote_delay),
      is_mst_supported_(opt_mst_gossip_params),
      mst_expiration_time_(mst_expiration_time),
      max_rounds_delay_(max_rounds_delay),
      stale_stream_max_rounds_(stale_stream_max_rounds),
      opt_alternative_peers_(std::move(opt_alternative_peers)),
      opt_mst_gossip_params_(opt_mst_gossip_params),
      keypair(keypair),
      ordering_init(logger_manager->getLogger()),
      yac_init(std::make_unique<iroha::consensus::yac::YacInit>()),
      consensus_gate_objects(consensus_gate_objects_lifetime),
      log_manager_(std::move(logger_manager)),
      log_(log_manager_->getLogger()) {
  log_->info("created");
  validators_config_ =
      std::make_shared<shared_model::validation::ValidatorsConfig>(
          max_proposal_size_);
  block_validators_config_ =
      std::make_shared<shared_model::validation::ValidatorsConfig>(
          max_proposal_size_, true);
  // Initializing storage at this point in order to insert genesis block before
  // initialization of iroha daemon
  initStorage();
}

Irohad::~Irohad() {
  consensus_gate_objects_lifetime.unsubscribe();
  consensus_gate_events_subscription.unsubscribe();
}

/**
 * Initializing iroha daemon
 */
Irohad::RunResult Irohad::init() {
  auto peers_updater = [&]() -> RunResult {
    if (opt_alternative_peers_) {
      return resetPeers(*opt_alternative_peers_);
    } else {
      return {};
    }
  };
  // clang-format off
  return initWsvRestorer() // Recover WSV from the existing ledger
                           // to be sure it is consistent
  | [this]{ return restoreWsv();}
  | peers_updater
  | [this]{ return initCryptoProvider();}
  | [this]{ return initBatchParser();}
  | [this]{ return initValidators();}
  | [this]{ return initNetworkClient();}
  | [this]{ return initFactories();}
  | [this]{ return initPersistentCache();}
  | [this]{ return initOrderingGate();}
  | [this]{ return initSimulator();}
  | [this]{ return initConsensusCache();}
  | [this]{ return initBlockLoader();}
  | [this]{ return initConsensusGate();}
  | [this]{ return initSynchronizer();}
  | [this]{ return initPeerCommunicationService();}
  | [this]{ return initStatusBus();}
  | [this]{ return initMstProcessor();}
  | [this]{ return initPendingTxsStorage();}

  // Torii
  | [this]{ return initTransactionCommandService();}
  | [this]{ return initQueryService();};
  // clang-format on
}

/**
 * Dropping iroha daemon storage
 */
void Irohad::dropStorage() {
  storage->reset();
}

/**
 * Initializing iroha daemon storage
 */
Irohad::RunResult Irohad::initStorage() {
  common_objects_factory_ =
      std::make_shared<shared_model::proto::ProtoCommonObjectsFactory<
          shared_model::validation::FieldValidator>>(validators_config_);
  auto perm_converter =
      std::make_shared<shared_model::proto::ProtoPermissionToString>();
  auto block_converter =
      std::make_shared<shared_model::proto::ProtoBlockJsonConverter>();
  auto block_storage_factory = std::make_unique<FlatFileBlockStorageFactory>(
      []() {
        return (boost::filesystem::temp_directory_path()
                / boost::filesystem::unique_path())
            .string();
      },
      block_converter,
      log_manager_);

  return StorageImpl::create(
             block_store_dir_,
             pg_conn_,
             common_objects_factory_,
             std::move(block_converter),
             perm_converter,
             std::move(block_storage_factory),
             std::make_unique<
                 iroha::ametsuchi::KTimesReconnectionStrategyFactory>(10),
             log_manager_->getChild("Storage"))
             | [&](auto &&v) -> RunResult {
    storage = std::move(v);
    log_->info("[Init] => storage");
    return {};
  };
}

Irohad::RunResult Irohad::restoreWsv() {
  return wsv_restorer_->restoreWsv(*storage) |
             [](const auto &value) -> RunResult {
    if (not value) {
      return iroha::expected::makeError<std::string>(
          "Did not get ledger state after WSV restoration!");
    }
    auto &ledger_state = value.value();
    assert(ledger_state);
    if (ledger_state->ledger_peers.empty()) {
      return iroha::expected::makeError<std::string>(
          "Have no peers in WSV after restoration!");
    }
    return {};
  };
}

Irohad::RunResult Irohad::resetPeers(
    const shared_model::interface::types::PeerList &alternative_peers) {
  storage->resetPeers();

  for (const auto &peer : alternative_peers) {
    auto result = storage->insertPeer(*peer);
    if (boost::get<expected::Error<std::string>>(&result)) {
      return result;
    }
  }
  return {};
}

/**
 * Initializing crypto provider
 */
Irohad::RunResult Irohad::initCryptoProvider() {
  crypto_signer_ =
      std::make_shared<shared_model::crypto::CryptoModelSigner<>>(keypair);

  log_->info("[Init] => crypto provider");
  return {};
}

Irohad::RunResult Irohad::initBatchParser() {
  batch_parser =
      std::make_shared<shared_model::interface::TransactionBatchParserImpl>();

  log_->info("[Init] => transaction batch parser");
  return {};
}

/**
 * Initializing validators
 */
Irohad::RunResult Irohad::initValidators() {
  auto factory = std::make_unique<shared_model::proto::ProtoProposalFactory<
      shared_model::validation::DefaultProposalValidator>>(validators_config_);
  auto validators_log_manager = log_manager_->getChild("Validators");
  stateful_validator = std::make_shared<StatefulValidatorImpl>(
      std::move(factory),
      batch_parser,
      validators_log_manager->getChild("Stateful")->getLogger());
  chain_validator = std::make_shared<ChainValidatorImpl>(
      getSupermajorityChecker(kConsensusConsistencyModel),
      validators_log_manager->getChild("Chain")->getLogger());

  log_->info("[Init] => validators");
  return {};
}

/**
 * Initializing network client
 */
Irohad::RunResult Irohad::initNetworkClient() {
  async_call_ =
      std::make_shared<network::AsyncGrpcClient<google::protobuf::Empty>>(
          log_manager_->getChild("AsyncNetworkClient")->getLogger());
  return {};
}

Irohad::RunResult Irohad::initFactories() {
  // proposal factory
  std::shared_ptr<
      shared_model::validation::AbstractValidator<iroha::protocol::Transaction>>
      proto_transaction_validator = std::make_shared<
          shared_model::validation::ProtoTransactionValidator>();
  std::unique_ptr<shared_model::validation::AbstractValidator<
      shared_model::interface::Proposal>>
      proposal_validator =
          std::make_unique<shared_model::validation::DefaultProposalValidator>(
              validators_config_);
  std::unique_ptr<
      shared_model::validation::AbstractValidator<iroha::protocol::Proposal>>
      proto_proposal_validator =
          std::make_unique<shared_model::validation::ProtoProposalValidator>(
              proto_transaction_validator);
  proposal_factory =
      std::make_shared<shared_model::proto::ProtoTransportFactory<
          shared_model::interface::Proposal,
          shared_model::proto::Proposal>>(std::move(proposal_validator),
                                          std::move(proto_proposal_validator));

  auto batch_validator =
      std::make_shared<shared_model::validation::BatchValidator>(
          validators_config_);
  // transaction factories
  transaction_batch_factory_ =
      std::make_shared<shared_model::interface::TransactionBatchFactoryImpl>(
          batch_validator);

  std::unique_ptr<shared_model::validation::AbstractValidator<
      shared_model::interface::Transaction>>
      transaction_validator = std::make_unique<
          shared_model::validation::DefaultOptionalSignedTransactionValidator>(
          validators_config_);
  transaction_factory =
      std::make_shared<shared_model::proto::ProtoTransportFactory<
          shared_model::interface::Transaction,
          shared_model::proto::Transaction>>(
          std::move(transaction_validator),
          std::move(proto_transaction_validator));

  // query factories
  query_response_factory_ =
      std::make_shared<shared_model::proto::ProtoQueryResponseFactory>();

  std::unique_ptr<shared_model::validation::AbstractValidator<
      shared_model::interface::Query>>
      query_validator = std::make_unique<
          shared_model::validation::DefaultSignedQueryValidator>(
          validators_config_);
  std::unique_ptr<
      shared_model::validation::AbstractValidator<iroha::protocol::Query>>
      proto_query_validator =
          std::make_unique<shared_model::validation::ProtoQueryValidator>();
  query_factory = std::make_shared<
      shared_model::proto::ProtoTransportFactory<shared_model::interface::Query,
                                                 shared_model::proto::Query>>(
      std::move(query_validator), std::move(proto_query_validator));

  auto blocks_query_validator = std::make_unique<
      shared_model::validation::DefaultSignedBlocksQueryValidator>(
      validators_config_);
  auto proto_blocks_query_validator =
      std::make_unique<shared_model::validation::ProtoBlocksQueryValidator>();

  blocks_query_factory =
      std::make_shared<shared_model::proto::ProtoTransportFactory<
          shared_model::interface::BlocksQuery,
          shared_model::proto::BlocksQuery>>(
          std::move(blocks_query_validator),
          std::move(proto_blocks_query_validator));

  log_->info("[Init] => factories");
  return {};
}

/**
 * Initializing persistent cache
 */
Irohad::RunResult Irohad::initPersistentCache() {
  persistent_cache = std::make_shared<TxPresenceCacheImpl>(storage);

  log_->info("[Init] => persistent cache");
  return {};
}

/**
 * Initializing ordering gate
 */
Irohad::RunResult Irohad::initOrderingGate() {
  auto block_query = storage->createBlockQuery();
  if (not block_query) {
    return iroha::expected::makeError<std::string>(
        "Failed to create block query");
  }
  // since delay is 2, it is required to get two more hashes from block store,
  // in addition to top block
  const size_t kNumBlocks = 3;
  auto top_height = (*block_query)->getTopBlockHeight();
  decltype(top_height) block_hashes =
      top_height > kNumBlocks ? kNumBlocks : top_height;

  auto hash_stub = shared_model::interface::types::HashType{std::string(
      shared_model::crypto::DefaultCryptoAlgorithmType::kHashLength, '0')};
  std::vector<shared_model::interface::types::HashType> hashes{
      kNumBlocks - block_hashes, hash_stub};

  for (decltype(top_height) i = top_height - block_hashes + 1; i <= top_height;
       ++i) {
    auto block_result = (*block_query)->getBlock(i);

    if (auto e = boost::get<expected::Error<std::string>>(&block_result)) {
      return iroha::expected::makeError(std::move(e->error));
    }

    auto &block =
        boost::get<
            expected::Value<std::unique_ptr<shared_model::interface::Block>>>(
            block_result)
            .value;
    hashes.push_back(block->hash());
  }

  auto factory = std::make_unique<shared_model::proto::ProtoProposalFactory<
      shared_model::validation::DefaultProposalValidator>>(validators_config_);

  const uint64_t kCounter = 0, kMaxLocalCounter = 2;
  // reject_delay and local_counter are local mutable variables of lambda
  const auto kMaxDelay(max_rounds_delay_);
  const auto kMaxDelayIncrement(std::chrono::milliseconds(1000));
  auto delay = [reject_delay = std::chrono::milliseconds(0),
                local_counter = kCounter,
                // MSVC requires const variables to be captured
                kMaxDelay,
                kMaxDelayIncrement,
                kMaxLocalCounter](const auto &commit) mutable {
    using iroha::synchronizer::SynchronizationOutcomeType;
    if (commit.sync_outcome == SynchronizationOutcomeType::kReject
        or commit.sync_outcome == SynchronizationOutcomeType::kNothing) {
      // Increment reject_counter each local_counter calls of function
      ++local_counter;
      if (local_counter == kMaxLocalCounter) {
        local_counter = 0;
        if (reject_delay < kMaxDelay) {
          reject_delay += std::min(kMaxDelay, kMaxDelayIncrement);
        }
      }
    } else {
      reject_delay = std::chrono::milliseconds(0);
    }
    return reject_delay;
  };

  ordering_gate =
      ordering_init.initOrderingGate(max_proposal_size_,
                                     proposal_delay_,
                                     std::move(hashes),
                                     transaction_factory,
                                     batch_parser,
                                     transaction_batch_factory_,
                                     async_call_,
                                     std::move(factory),
                                     proposal_factory,
                                     persistent_cache,
                                     delay,
                                     log_manager_->getChild("Ordering"));
  log_->info("[Init] => init ordering gate - [{}]",
             logger::logBool(ordering_gate));
  return {};
}

/**
 * Initializing iroha verified proposal creator and block creator
 */
Irohad::RunResult Irohad::initSimulator() {
  auto block_factory = std::make_unique<shared_model::proto::ProtoBlockFactory>(
      //  Block factory in simulator uses UnsignedBlockValidator because
      //  it is not required to check signatures of block here, as they
      //  will be checked when supermajority of peers will sign the block.
      //  It is also not required to validate signatures of transactions
      //  here because they are validated in the ordering gate, where they
      //  are received from the ordering service.
      std::make_unique<shared_model::validation::DefaultUnsignedBlockValidator>(
          block_validators_config_),
      std::make_unique<shared_model::validation::ProtoBlockValidator>());
  simulator = std::make_shared<Simulator>(
      ordering_gate,
      stateful_validator,
      storage,
      crypto_signer_,
      std::move(block_factory),
      log_manager_->getChild("Simulator")->getLogger());

  log_->info("[Init] => init simulator");
  return {};
}

/**
 * Initializing consensus block cache
 */
Irohad::RunResult Irohad::initConsensusCache() {
  consensus_result_cache_ = std::make_shared<consensus::ConsensusResultCache>();

  log_->info("[Init] => init consensus block cache");
  return {};
}

/**
 * Initializing block loader
 */
Irohad::RunResult Irohad::initBlockLoader() {
  block_loader =
      loader_init.initBlockLoader(storage,
                                  storage,
                                  consensus_result_cache_,
                                  block_validators_config_,
                                  log_manager_->getChild("BlockLoader"));

  log_->info("[Init] => block loader");
  return {};
}

/**
 * Initializing consensus gate
 */
Irohad::RunResult Irohad::initConsensusGate() {
  auto block_query = storage->createBlockQuery();
  if (not block_query) {
    return iroha::expected::makeError<std::string>(
        "Failed to create block query");
  }
  auto block_var =
      (*block_query)->getBlock((*block_query)->getTopBlockHeight());
  if (auto e = boost::get<expected::Error<std::string>>(&block_var)) {
    return iroha::expected::makeError<std::string>(
        "Failed to get the top block: " + e->error);
  }

  auto &block =
      boost::get<
          expected::Value<std::unique_ptr<shared_model::interface::Block>>>(
          &block_var)
          ->value;
  consensus_gate = yac_init->initConsensusGate(
      {block->height(), ordering::kFirstRejectRound},
      storage,
      simulator,
      block_loader,
      keypair,
      consensus_result_cache_,
      vote_delay_,
      async_call_,
      common_objects_factory_,
      kConsensusConsistencyModel,
      log_manager_->getChild("Consensus"));
  consensus_gate->onOutcome().subscribe(
      consensus_gate_events_subscription,
      consensus_gate_objects.get_subscriber());
  log_->info("[Init] => consensus gate");
  return {};
}

/**
 * Initializing synchronizer
 */
Irohad::RunResult Irohad::initSynchronizer() {
  synchronizer = std::make_shared<SynchronizerImpl>(
      consensus_gate,
      chain_validator,
      storage,
      storage,
      block_loader,
      log_manager_->getChild("Synchronizer")->getLogger());

  log_->info("[Init] => synchronizer");
  return {};
}

/**
 * Initializing peer communication service
 */
Irohad::RunResult Irohad::initPeerCommunicationService() {
  pcs = std::make_shared<PeerCommunicationServiceImpl>(
      ordering_gate,
      synchronizer,
      simulator,
      log_manager_->getChild("PeerCommunicationService")->getLogger());

  pcs->onProposal().subscribe([this](const auto &) {
    log_->info("~~~~~~~~~| PROPOSAL ^_^ |~~~~~~~~~ ");
  });

  pcs->onSynchronization().subscribe([this](const auto &event) {
    using iroha::synchronizer::SynchronizationOutcomeType;
    switch (event.sync_outcome) {
      case SynchronizationOutcomeType::kCommit:
        log_->info(R"(~~~~~~~~~| COMMIT =^._.^= |~~~~~~~~~ )");
        break;
      case SynchronizationOutcomeType::kReject:
        log_->info(R"(~~~~~~~~~| REJECT \(*.*)/ |~~~~~~~~~ )");
        break;
      case SynchronizationOutcomeType::kNothing:
        log_->info(R"(~~~~~~~~~| EMPTY (-_-)zzz |~~~~~~~~~ )");
        break;
      default:
        break;
    }
  });

  log_->info("[Init] => pcs");
  return {};
}

Irohad::RunResult Irohad::initStatusBus() {
  status_bus_ = std::make_shared<StatusBusImpl>();
  log_->info("[Init] => Tx status bus");
  return {};
}

Irohad::RunResult Irohad::initMstProcessor() {
  auto mst_logger_manager =
      log_manager_->getChild("MultiSignatureTransactions");
  auto mst_state_logger = mst_logger_manager->getChild("State")->getLogger();
  auto mst_completer = std::make_shared<DefaultCompleter>(mst_expiration_time_);
  auto mst_storage = std::make_shared<MstStorageStateImpl>(
      mst_completer,
      mst_state_logger,
      mst_logger_manager->getChild("Storage")->getLogger());
  std::shared_ptr<iroha::PropagationStrategy> mst_propagation;
  if (is_mst_supported_) {
    mst_transport = std::make_shared<iroha::network::MstTransportGrpc>(
        async_call_,
        transaction_factory,
        batch_parser,
        transaction_batch_factory_,
        persistent_cache,
        mst_completer,
        keypair.publicKey(),
        std::move(mst_state_logger),
        mst_logger_manager->getChild("Transport")->getLogger());
    mst_propagation = std::make_shared<GossipPropagationStrategy>(
        storage, rxcpp::observe_on_new_thread(), *opt_mst_gossip_params_);
  } else {
    mst_transport = std::make_shared<iroha::network::MstTransportStub>();
    mst_propagation = std::make_shared<iroha::PropagationStrategyStub>();
  }

  auto mst_time = std::make_shared<MstTimeProviderImpl>();
  auto fair_mst_processor = std::make_shared<FairMstProcessor>(
      mst_transport,
      mst_storage,
      mst_propagation,
      mst_time,
      mst_logger_manager->getChild("Processor")->getLogger());
  mst_processor = fair_mst_processor;
  mst_transport->subscribe(fair_mst_processor);
  log_->info("[Init] => MST processor");
  return {};
}

Irohad::RunResult Irohad::initPendingTxsStorage() {
  pending_txs_storage_ = std::make_shared<PendingTransactionStorageImpl>(
      mst_processor->onStateUpdate(),
      mst_processor->onPreparedBatches(),
      mst_processor->onExpiredBatches());
  log_->info("[Init] => pending transactions storage");
  return {};
}

/**
 * Initializing transaction command service
 */
Irohad::RunResult Irohad::initTransactionCommandService() {
  auto command_service_log_manager = log_manager_->getChild("CommandService");
  auto status_factory =
      std::make_shared<shared_model::proto::ProtoTxStatusFactory>();
  auto cs_cache = std::make_shared<::torii::CommandServiceImpl::CacheType>();
  auto tx_processor = std::make_shared<TransactionProcessorImpl>(
      pcs,
      mst_processor,
      status_bus_,
      status_factory,
      storage->on_commit(),
      command_service_log_manager->getChild("Processor")->getLogger());
  command_service = std::make_shared<::torii::CommandServiceImpl>(
      tx_processor,
      storage,
      status_bus_,
      status_factory,
      cs_cache,
      persistent_cache,
      command_service_log_manager->getLogger());
  command_service_transport =
      std::make_shared<::torii::CommandServiceTransportGrpc>(
          command_service,
          status_bus_,
          status_factory,
          transaction_factory,
          batch_parser,
          transaction_batch_factory_,
          consensus_gate_objects.get_observable().map([](const auto &) {
            return ::torii::CommandServiceTransportGrpc::ConsensusGateEvent{};
          }),
          stale_stream_max_rounds_,
          command_service_log_manager->getChild("Transport")->getLogger());

  log_->info("[Init] => command service");
  return {};
}

/**
 * Initializing query command service
 */
Irohad::RunResult Irohad::initQueryService() {
  auto query_service_log_manager = log_manager_->getChild("QueryService");
  auto query_processor = std::make_shared<QueryProcessorImpl>(
      storage,
      storage,
      pending_txs_storage_,
      query_response_factory_,
      query_service_log_manager->getChild("Processor")->getLogger());

  query_service = std::make_shared<::torii::QueryService>(
      query_processor,
      query_factory,
      blocks_query_factory,
      query_service_log_manager->getLogger());

  log_->info("[Init] => query service");
  return {};
}

Irohad::RunResult Irohad::initWsvRestorer() {
  wsv_restorer_ = std::make_shared<iroha::ametsuchi::WsvRestorerImpl>();
  return {};
}

/**
 * Run iroha daemon
 */
Irohad::RunResult Irohad::run() {
  using iroha::expected::operator|;
  using iroha::operator|;

  // Initializing torii server
  torii_server = std::make_unique<ServerRunner>(
      listen_ip_ + ":" + std::to_string(torii_port_),
      log_manager_->getChild("ToriiServerRunner")->getLogger(),
      false);

  // Initializing internal server
  internal_server = std::make_unique<ServerRunner>(
      listen_ip_ + ":" + std::to_string(internal_port_),
      log_manager_->getChild("InternalServerRunner")->getLogger(),
      false);

  // Run torii server
  return (torii_server->append(command_service_transport)
              .append(query_service)
              .run()
          |
          [&](const auto &port) {
            log_->info("Torii server bound on port {}", port);
            if (is_mst_supported_) {
              internal_server->append(
                  std::static_pointer_cast<MstTransportGrpc>(mst_transport));
            }
            // Run internal server
            return internal_server->append(ordering_init.service)
                .append(yac_init->getConsensusNetwork())
                .append(loader_init.service)
                .run();
          }) |
             [&](const auto &port) -> RunResult {
    log_->info("Internal server bound on port {}", port);
    log_->info("===> iroha initialized");
    // initiate first round
    auto block_query = storage->createBlockQuery();
    if (not block_query) {
      return expected::makeError("Failed to create block query");
    }
    auto block_var =
        (*block_query)->getBlock((*block_query)->getTopBlockHeight());
    if (auto e = boost::get<expected::Error<std::string>>(&block_var)) {
      return expected::makeError("Failed to get the top block: " + e->error);
    }

    auto &block =
        boost::get<
            expected::Value<std::unique_ptr<shared_model::interface::Block>>>(
            &block_var)
            ->value;
    auto block_height = block->height();

    auto peers = storage->createPeerQuery() |
        [](auto &&peer_query) { return peer_query->getLedgerPeers(); };

    auto initial_ledger_state = std::make_shared<LedgerState>(
        peers.value(), block->height(), block->hash());

    pcs->onSynchronization().subscribe(
        ordering_init.sync_event_notifier.get_subscriber());
    storage->on_commit().subscribe(
        ordering_init.commit_notifier.get_subscriber());

    ordering_init.commit_notifier.get_subscriber().on_next(std::move(block));

    ordering_init.sync_event_notifier.get_subscriber().on_next(
        synchronizer::SynchronizationEvent{
            SynchronizationOutcomeType::kCommit,
            {block_height, ordering::kFirstRejectRound},
            initial_ledger_state});
    return {};
  };
}

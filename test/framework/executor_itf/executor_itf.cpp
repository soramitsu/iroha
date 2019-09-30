/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "framework/executor_itf/executor_itf.hpp"

#include "ametsuchi/specific_query_executor.hpp"
#include "ametsuchi/tx_executor.hpp"
#include "cryptography/signed.hpp"
#include "framework/config_helper.hpp"
#include "framework/test_logger.hpp"
#include "interfaces/permissions.hpp"
#include "logger/logger.hpp"
#include "logger/logger_manager.hpp"
#include "module/shared_model/mock_objects_factories/mock_command_factory.hpp"
#include "module/shared_model/mock_objects_factories/mock_query_factory.hpp"

using namespace iroha::integration_framework;
using namespace iroha::ametsuchi;
using namespace common_constants;
using namespace iroha::expected;

namespace {
  logger::LoggerManagerTreePtr getDefaultLogManager() {
    return getTestLoggerManager()->getChild("ExecutorITF");
  }

  std::string getDefaultRole(const std::string &name,
                             const std::string &domain) {
    return name + "_at_" + domain + "_defrole";
  }

  std::string getDefaultRole(const std::string &name) {
    return name + "_defrole";
  }
}  // namespace

ExecutorItf::ExecutorItf(std::shared_ptr<CommandExecutor> cmd_executor,
                         std::shared_ptr<SpecificQueryExecutor> query_executor,
                         logger::LoggerManagerTreePtr log_manager)
    : log_manager_(std::move(log_manager)),
      log_(log_manager_->getLogger()),
      mock_command_factory_(
          std::make_unique<shared_model::interface::MockCommandFactory>()),
      mock_query_factory_(
          std::make_unique<shared_model::interface::MockQueryFactory>()),
      cmd_executor_(std::move(cmd_executor)),
      tx_executor_(std::make_unique<TransactionExecutor>(cmd_executor_)),
      query_executor_(std::move(query_executor)),
      query_counter_(0) {}

ExecutorItf::~ExecutorItf() {
  // storage cleanup
}

using CreateResult = Result<std::unique_ptr<ExecutorItf>, std::string>;

CreateResult ExecutorItf::create(ExecutorItfTarget target) {
  auto log_manager = getDefaultLogManager();
  std::unique_ptr<ExecutorItf> executor_itf(
      new ExecutorItf(std::move(target.command_executor),
                      std::move(target.query_executor),
                      log_manager));
  return executor_itf->createAdmin().match(
      [&executor_itf](const auto &) -> CreateResult {
        return std::move(executor_itf);
      },
      [](const auto &error) -> CreateResult { return error.error.toString(); });
}

CommandResult ExecutorItf::executeCommandAsAccount(
    const shared_model::interface::Command &cmd,
    const std::string &account_id) {
  last_executed_cmd_meta_ = ExecutedCommandMeta{};
  last_executed_cmd_meta_->creator_account_id = account_id;
  last_executed_cmd_meta_->tx_hash = std::to_string(command_counter_++);
  last_executed_cmd_meta_->cmd_index = 0;
  return cmd_executor_->execute(cmd,
                                account_id,
                                last_executed_cmd_meta_->tx_hash,
                                last_executed_cmd_meta_->cmd_index,
                                true);
}

Result<void, TxExecutionError> ExecutorItf::executeTransaction(
    const shared_model::interface::Transaction &transaction,
    bool do_validation) const {
  return tx_executor_->execute(transaction, do_validation);
}

iroha::ametsuchi::QueryExecutorResult ExecutorItf::executeQuery(
    const shared_model::interface::Query &query) const {
  return query_executor_->execute(query);
}

iroha::integration_framework::ExecutorItf::SpecificQueryResult<
    shared_model::interface::EngineResponse>
ExecutorItf::getLastEngineResultResponse() {
  assert(last_executed_cmd_meta_);
  auto query = getMockQueryFactory()->constructGetEngineResponse(
      last_executed_cmd_meta_->tx_hash);
  return executeQuery(
      *query, last_executed_cmd_meta_->creator_account_id, boost::none);
}

const std::unique_ptr<shared_model::interface::MockCommandFactory>
    &ExecutorItf::getMockCommandFactory() const {
  return mock_command_factory_;
}

const std::unique_ptr<shared_model::interface::MockQueryFactory>
    &ExecutorItf::getMockQueryFactory() const {
  return mock_query_factory_;
}

CommandResult ExecutorItf::createRoleWithPerms(
    const std::string &role_id,
    const shared_model::interface::RolePermissionSet &role_permissions) {
  return executeCommand(
      *getMockCommandFactory()->constructCreateRole(role_id, role_permissions));
}

CommandResult ExecutorItf::createUserWithPerms(
    const std::string &account_name,
    const std::string &domain,
    const shared_model::crypto::PublicKey &pubkey,
    const shared_model::interface::RolePermissionSet &role_perms) {
  return createUserWithPermsInternal(account_name, domain, pubkey, role_perms) |
      [&, this] { return this->grantAllToAdmin(account_name + "@" + domain); };
}

CommandResult ExecutorItf::createDomain(const std::string &name) {
  const std::string default_role = getDefaultRole(name);
  createRoleWithPerms(default_role, {});
  return executeCommand(
      *getMockCommandFactory()->constructCreateDomain(name, default_role));
}

CommandResult ExecutorItf::grantAllToAdmin(const std::string &account_id) {
  static const std::string admin_role_name =
      getDefaultRole(kAdminName, kDomain);
  shared_model::interface::GrantablePermissionSet all_grantable_perms;
  CommandResult grant_perm_result =
      executeCommand(*getMockCommandFactory()->constructAppendRole(
          account_id, admin_role_name));
  all_grantable_perms.setAll();
  all_grantable_perms.iterate(
      [this, &account_id, &grant_perm_result](const auto &perm) {
        grant_perm_result |= [this, perm, &account_id] {
          return this->executeCommandAsAccount(
              *this->getMockCommandFactory()->constructGrantPermission(kAdminId,
                                                                       perm),
              account_id);
        };
      });
  return grant_perm_result | [&, this] {
    return this->executeCommand(
        *this->getMockCommandFactory()->constructDetachRole(account_id,
                                                            admin_role_name));
  };
}

CommandResult ExecutorItf::createUserWithPermsInternal(
    const std::string &account_name,
    const std::string &domain,
    const shared_model::crypto::PublicKey &pubkey,
    const shared_model::interface::RolePermissionSet &role_perms) {
  createDomain(domain);

  const std::string account_id = account_name + "@" + domain;
  const std::string account_role_name = getDefaultRole(account_name, domain);

  return executeCommand(*getMockCommandFactory()->constructCreateAccount(
             account_name, domain, pubkey))
      | [&, this] { return createRoleWithPerms(account_role_name, role_perms); }
  | [&, this] {
      return this->executeCommand(
          *this->getMockCommandFactory()->constructAppendRole(
              account_id, account_role_name));
    };
}

CommandResult ExecutorItf::createAdmin() {
  shared_model::interface::RolePermissionSet all_role_perms;
  all_role_perms.setAll();
  return createUserWithPermsInternal(
      kAdminName, kDomain, kAdminKeypair.publicKey(), all_role_perms);
}

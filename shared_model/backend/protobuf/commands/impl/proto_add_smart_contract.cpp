#include "backend/protobuf/commands/proto_add_smart_contract.hpp"

namespace shared_model {
  namespace proto {

    template <typename CommandType>
    AddSmartContract::AddSmartContract(CommandType &&command)
      : CopyableProto(std::forward<CommandType>(command)),
        add_smart_contract_{proto_->add_smart_contract()} {}

    template AddSmartContract::AddSmartContract(AddSmartContract::TransportType &);
    template AddSmartContract::AddSmartContract(const AddSmartContract::TransportType &);
    template AddSmartContract::AddSmartContract(AddSmartContract::TransportType &&);

    AddSmartContract::AddSmartContract(const AddSmartContract &o)
      : AddSmartContract(o.proto_) {}

    AddSmartContract::AddSmartContract(AddSmartContract &&o) noexcept
      : AddSmartContract(std::move(o.proto_)) {}

    const interface::types::SmartContractCodeType &AddSmartContract::code() const {
      return add_smart_contract_.code();
    }

  }  // namespace proto
}  // namespace shared_model


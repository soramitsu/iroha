#include "interfaces/commands/add_smart_contract.hpp"

namespace shared_model {
  namespace interface {

    std::string AddSmartContract::toString() const {
      return detail::PrettyStringBuilder()
          .init("AddSmartContract")
          .append("code", code())
          .finalize();
    }

    bool AddSmartContract::operator==(const ModelType &rhs) const {
      return code() == rhs.code();
    }

  }  // namespace interface
}  // namespace shared_model

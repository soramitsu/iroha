#include "interfaces/commands/add_smart_contract.hpp"

namespace shared_model {
  namespace interface {

    std::string AddSmartContract::toString() const {
      return detail::PrettyStringBuilder()
          .init("AddSmartContract")
          .append("code", code())
          .append("callee", callee())
          .finalize();
    }

    bool AddSmartContract::operator==(const ModelType &rhs) const {
      return code() == rhs.code()
        && callee() == rhs.callee();
    }

  }  // namespace interface
}  // namespace shared_model

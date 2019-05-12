#ifndef IROHA_ADD_SMART_CONTRACT_HPP
#define IROHA_ADD_SMART_CONTRACT_HPP

#include "model/command.hpp"

namespace iroha {
  namespace model {

    /**
     * Add smart contract to transaction
     */
    struct AddSmartContract : public Command {
      /**
       * Code of the smart contract
       */
      std::string code;

     /**
       * Account of the callee.
       * Account of the caller is store
       * inside transaction metadata.
       */
      std::string callee;

      bool operator==(const Command &command) const override;

      AddSmartContract() = default;

      /**
       * @param code_of_smart_contract — smart contract code
       */
      AddSmartContract(
        const std::string &code_of_smart_contract,
        const std::string &callee_account
      ) : code(code_of_smart_contract),
          callee(callee_account)
      {}
    };
  }  // namespace model
}  // namespace iroha
#endif  // IROHA_ADD_SMART_CONTRACT_HPP

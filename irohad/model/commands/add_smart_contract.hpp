#ifndef IROHA_ADD_SMART_CONTRACT_HPP
#define IROHA_ADD_SMART_CONTRACT_HPP

#include "model/command.hpp"

namespace iroha {
  namespace model {

    /**
     * Add smart contract to transaction
     */
    struct AddSmartContract : public Command {

      /*
       * Account of the caller 
       * TODO(IvanTyulyandin)
       * It should be taken from
       * inside transaction metadata.
       */
      std::string caller;

     /**
       * Account of the callee.
       * Account of the caller is store
       * inside transaction metadata.
       */
      std::string callee;

      /**
       * Code of the smart contract
       */
      std::string code;

      /*
       * Input to call an existing code in the blockchain state
       */
      std::string input;


      bool operator==(const Command &command) const override;

      AddSmartContract() = default;

      /**
       * @param caller_account — account of the caller
       * @param callee_account — account of the callee
       * @param code_of_smart_contract — smart contract code
       * @param input_func_and_data — input with encoded function and params
       */
      AddSmartContract(
        const std::string &caller_account,
        const std::string &callee_account,
        const std::string &code_of_smart_contract,
        const std::string &input_func_and_data
      ) : caller(caller_account),
          callee(callee_account),
          code(code_of_smart_contract),
          input(input_func_and_data)
      {}
    };
  }  // namespace model
}  // namespace iroha
#endif  // IROHA_ADD_SMART_CONTRACT_HPP

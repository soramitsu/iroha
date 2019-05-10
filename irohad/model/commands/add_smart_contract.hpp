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

       bool operator==(const Command &command) const override;

       AddSmartContract() = default;

       /**
       * @param code_of_smart_contract — smart contract code
       */
      AddSmartContract(const std::string &code_of_smart_contract)
          : code(code_of_smart_contract){}
    };
  }  // namespace model
}  // namespace iroha
#endif  // IROHA_ADD_SMART_CONTRACT_HPP

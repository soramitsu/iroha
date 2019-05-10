#include <gtest/gtest.h>
#include "framework/integration_framework/integration_test_framework.hpp"
#include "integration/acceptance/acceptance_fixture.hpp"

using namespace integration_framework;
using namespace shared_model;
using namespace common_constants;

class AddSmartContract : public AcceptanceFixture {
  public:
    // TODO(IvanTyulyandin): add permissions for add_smart_contract
    auto makeUserWithPerms(const interface::RolePermissionSet &perms =
                           shared_model::interface::RolePermissionSet()) {
      return AcceptanceFixture::makeUserWithPerms(perms);
    }

};

/**
 * @given some user
 * @when execute tx with AddSmartContract command
 * @then there is the tx in proposal
 */
TEST_F(AddSmartContract, Basic) {
  IntegrationTestFramework(1)
      .setInitialState(kAdminKeypair)
      .sendTx(makeUserWithPerms())
      .skipProposal()
      .skipBlock()
      .sendTxAwait(
          complete(baseTx().addSmartContract("JustNonEmptyString")),
          [](auto &block) { ASSERT_EQ(block->transactions().size(), 1); });
}

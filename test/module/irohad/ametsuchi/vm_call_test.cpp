/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ametsuchi/vmCall.h"

#include <gtest/gtest.h>

TEST(VmCallTest, UsageTest) {
  /*

code is bytecode from the following Solidity code using online Remix IDE with
compiler version 0.4.0

pragma solidity ^0.4.0;

contract C {
  uint256 a;
  function setA(uint256 _a) {
    a = _a;
  }

  function getA() returns(uint256) {
    return a;
  }
}

*/

  char *code = const_cast<char *>(
      "606060405260a18060106000396000f360606040526000357c0100000000000000000000"
      "00000000000000000000000000000000000090048063d46300fd146043578063ee919d50"
      "14606857603f565b6002565b34600257605260048050506082565b604051808281526020"
      "0191505060405180910390f35b3460025760806004808035906020019091905050609356"
      "5b005b600060006000505490506090565b90565b806000600050819055505b5056");

  /*
    calling setA(uint256), bytes4(keccak256(setA(uint256))) == ee919d50, and
    append uint256 equal to 1 as the parameter
  */

  char *inputCallSetter = const_cast<char *>(
      "ee919d50000000000000000000000000000000000000000000000000000000000000000"
      "1");

  /*
    calling getA(), bytes4(keccak256(getA())) == d46300fd
  */

  char *inputCallGetter = const_cast<char *>("d46300fd");

  char *caller = const_cast<char *>("caller"),
       *callee = const_cast<char *>("Callee"), *empty = const_cast<char *>("");

  auto res = VmCall(code, empty, caller, callee);
  std::cout << "Vm output: " << res.r0 << std::endl;
  ASSERT_TRUE(res.r1);

  res = VmCall(empty, inputCallSetter, caller, callee);
  std::cout << "Vm output: " << res.r0 << std::endl;
  ASSERT_TRUE(res.r1);

  res = VmCall(empty, inputCallGetter, caller, callee);
  std::cout << "Vm output: " << res.r0 << std::endl;
  ASSERT_TRUE(res.r1);
}

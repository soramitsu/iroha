/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backend/protobuf/queries/proto_account_detail_record_id.hpp"

using namespace shared_model::proto;

AccountDetailRecordId::AccountDetailRecordId(const TransportType &query)
    : CopyableProto(query) {}

AccountDetailRecordId::AccountDetailRecordId(const AccountDetailRecordId &o)
    : AccountDetailRecordId(*o.proto_) {}

boost::optional<shared_model::interface::types::AccountIdType>
AccountDetailRecordId::writer() const {
  if (proto_->opt_writer_case()
      == TransportType::OptWriterCase::OPT_WRITER_NOT_SET) {
    return boost::none;
  }
  return proto_->writer();
}

boost::optional<shared_model::interface::types::AccountDetailKeyType>
AccountDetailRecordId::key() const {
  if (proto_->opt_key_case() == TransportType::OptKeyCase::OPT_KEY_NOT_SET) {
    return boost::none;
  }
  return proto_->key();
}

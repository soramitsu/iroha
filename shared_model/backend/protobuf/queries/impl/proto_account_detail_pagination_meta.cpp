/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backend/protobuf/queries/proto_account_detail_pagination_meta.hpp"

using namespace shared_model::proto;

AccountDetailPaginationMeta::AccountDetailPaginationMeta(
    const TransportType &query)
    : CopyableProto(query),
      first_record_id_{[this]() -> decltype(first_record_id_) {
        if (proto_->has_first_record_id_case()) {
          return AccountDetailRecordId{this->proto_->first_record_id()};
        }
        return boost::none;
      }()} {}

AccountDetailPaginationMeta::AccountDetailPaginationMeta(
    const AccountDetailPaginationMeta &o)
    : AccountDetailPaginationMeta(*o.proto_) {}

size_t AccountDetailPaginationMeta::pageSize() const {
  return proto_->page_size();
}

boost::optional<const shared_model::interface::AccountDetailRecordId &>
AccountDetailPaginationMeta::firstRecordId() const {
  if (first_record_id_) {
    return first_record_id_.value();
  }
  return {};
}

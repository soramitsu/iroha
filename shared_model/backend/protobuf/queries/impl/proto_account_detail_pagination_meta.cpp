/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "backend/protobuf/queries/proto_account_detail_pagination_meta.hpp"

using namespace shared_model::proto;

template <typename QueryType, typename>
AccountDetailPaginationMeta::AccountDetailPaginationMeta(QueryType &&query)
    : CopyableProto(std::forward<QueryType>(query)),
      first_record_id_{[this]() -> decltype(first_record_id_) {
        switch (proto_->opt_first_record_id_case()) {
          case iroha::protocol::AccountDetailPaginationMeta::kFirstRecordId:
            return AccountDetailRecordId{this->proto_->first_record_id()};
          default:
            return boost::none;
        }
      }()} {}

template AccountDetailPaginationMeta::AccountDetailPaginationMeta(
    AccountDetailPaginationMeta::TransportType &);
template AccountDetailPaginationMeta::AccountDetailPaginationMeta(
    const AccountDetailPaginationMeta::TransportType &);
template AccountDetailPaginationMeta::AccountDetailPaginationMeta(
    AccountDetailPaginationMeta::TransportType &&);

AccountDetailPaginationMeta::AccountDetailPaginationMeta(
    const AccountDetailPaginationMeta &o)
    : AccountDetailPaginationMeta(o.proto_) {}

AccountDetailPaginationMeta::AccountDetailPaginationMeta(
    AccountDetailPaginationMeta &&o) noexcept
    : AccountDetailPaginationMeta(std::move(o.proto_)) {}

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

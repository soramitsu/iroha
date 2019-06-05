/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef IROHA_SHARED_PROTO_MODEL_QUERY_ACCOUNT_DETAIL_RECORD_ID_HPP
#define IROHA_SHARED_PROTO_MODEL_QUERY_ACCOUNT_DETAIL_RECORD_ID_HPP

#include "backend/protobuf/common_objects/trivial_proto.hpp"
#include "interfaces/common_objects/types.hpp"
#include "interfaces/queries/account_detail_record_id.hpp"
#include "primitive.pb.h"

namespace shared_model {
  namespace proto {

    /// Provides query metadata for AccountDetail list pagination.
    class AccountDetailRecordId final
        : public CopyableProto<interface::AccountDetailRecordId,
                               iroha::protocol::AccountDetailRecordId,
                               AccountDetailRecordId> {
     public:
      template <typename QueryType,
                typename = std::enable_if_t<
                    std::is_same<std::decay_t<QueryType>, TransportType>::value
                    or std::is_same<std::decay_t<QueryType>,
                                    shared_model::detail::ReferenceHolder<
                                        TransportType>>::value>>
      explicit AccountDetailRecordId(QueryType &&query);

      AccountDetailRecordId(const AccountDetailRecordId &o);
      AccountDetailRecordId(AccountDetailRecordId &&o) noexcept;

      boost::optional<shared_model::interface::types::AccountIdType> writer()
          const override;

      boost::optional<shared_model::interface::types::AccountDetailKeyType>
      key() const override;

     private:
      const boost::optional<shared_model::interface::types::AccountIdType>
          writer_;
      const boost::optional<
          shared_model::interface::types::AccountDetailKeyType>
          key_;
    };
  }  // namespace proto
}  // namespace shared_model

#endif  // IROHA_SHARED_PROTO_MODEL_QUERY_ACCOUNT_DETAIL_RECORD_ID_HPP

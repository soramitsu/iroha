Queries
=======

A query is a request related to certain part of World State View — the latest state of blockchain.
Query cannot modify the contents of the chain and a response is returned
to any client immediately after receiving peer has processed a query.

Validation
^^^^^^^^^^

The validation for all queries includes:

- timestamp — shouldn't be from the past (24 hours prior to the peer time) or from the future (range of 5 minutes added to the peer time)
- signature of query creator — used for checking the identity of query creator
- query counter — checked to be incremented with every subsequent query from query creator
- roles — depending on the query creator's role: the range of state available to query can relate to to the same account, account in the domain, to the whole chain, or not allowed at all

Get Account
^^^^^^^^^^^

Purpose
-------

Purpose of get account query is to get the state of an account.

Request Schema
--------------

.. code-block:: proto

    message GetAccount {
        string account_id = 1;
    }

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Account ID", "account id to request its state", "<account_name>@<domain_id>", "alex@morgan"

Response Schema
---------------

.. code-block:: proto

    message AccountResponse {
        Account account = 1;
        repeated string account_roles = 2;
    }

    message Account {
        string account_id = 1;
        string domain_id = 2;
        uint32 quorum = 3;
        string json_data = 4;
    }


Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Account ID", "account id", "<account_name>@<domain_id>", "alex@morgan"
    "Domain ID", "domain where the account was created", "RFC1035 [#f1]_, RFC1123 [#f2]_ ", "morgan"
    "Quorum", "number of signatories needed to sign the transaction to make it valid", "0 < quorum ≤ 128", "5"
    "JSON data", "key-value account information", "JSON", "{ genesis: {name: alex} }"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get account", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get account", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"

Get Block
^^^^^^^^^

Purpose
-------

Purpose of get block query is to get a specific block, using its height as an identifier

Request Schema
--------------

.. code-block:: proto

    message GetBlock {
      uint64 height = 1;
    }


Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Height", "height of the block to be retrieved", "0 < height < 2^64", "42"

Response Schema
---------------

.. code-block:: proto

    message BlockResponse {
      Block block = 1;
    }

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Block", "the retrieved block", "block structure", "block"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get block", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have a permission to get block", "Grant the necessary permission"
    "3", "Invalid height", "Supplied height is not uint_64 or greater than the ledger's height", "Check the height and try again"

Get Signatories
^^^^^^^^^^^^^^^

Purpose
-------

Purpose of get signatories query is to get signatories, which act as an identity of the account.

Request Schema
--------------

.. code-block:: proto

    message GetSignatories {
        string account_id = 1;
    }

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Account ID", "account id to request signatories", "<account_name>@<domain_id>", "alex@morgan"

Response Schema
---------------

.. code-block:: proto

    message SignatoriesResponse {
        repeated bytes keys = 1;
    }

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Keys", "an array of public keys", "`ed25519 <https://ed25519.cr.yp.to>`_", "292a8714694095edce6be799398ed5d6244cd7be37eb813106b217d850d261f2"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get signatories", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get signatories", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"

Get Transactions
^^^^^^^^^^^^^^^^

Purpose
-------

GetTransactions is used for retrieving information about transactions, based on their hashes.
.. note:: This query is valid if and only if all the requested hashes are correct: corresponding transactions exist, and the user has a permission to retrieve them

Request Schema
--------------

.. code-block:: proto

    message GetTransactions {
        repeated bytes tx_hashes = 1;
    }

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Transactions hashes", "an array of hashes", "array with 32 byte hashes", "{hash1, hash2…}"

Response Schema
---------------

.. code-block:: proto

    message TransactionsResponse {
        repeated Transaction transactions = 1;
    }

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Transactions", "an array of transactions", "Committed transactions", "{tx1, tx2…}"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get transactions", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get transactions", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"
    "4", "Invalid hash", "At least one of the supplied hashes either does not exist in user's transaction list or creator of the query does not have permissions to see it", "Check the supplied hashes and try again"

Get Pending Transactions
^^^^^^^^^^^^^^^^^^^^^^^^

Purpose
-------

GetPendingTransactions is used for retrieving a list of pending (not fully signed) `multisignature transactions <../core_concepts/glossary.html#multisignature-transactions>`_
or `batches of transactions <../core_concepts/glossary.html#batch-of-transactions>`__ issued by account of query creator.

Request Schema
--------------

.. code-block:: proto

    message GetPendingTransactions {
    }

Response Schema
---------------

.. code-block:: proto

    message TransactionsResponse {
        repeated Transaction transactions = 1;
    }

Response Structure
------------------

The response contains a list of `pending transactions <../core_concepts/glossary.html#pending-transactions>`_.

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

        "Transactions", "an array of pending transactions", "Pending transactions", "{tx1, tx2…}"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get pending transactions", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get pending transactions", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"

Get Account Transactions
^^^^^^^^^^^^^^^^^^^^^^^^

Purpose
-------

In a case when a list of transactions per account is needed, `GetAccountTransactions` query can be formed.

.. note:: This query uses pagination for quicker and more convenient query responses.

Request Schema
--------------

.. code-block:: proto

    message TxPaginationMeta {
        uint32 page_size = 1;
        oneof opt_first_tx_hash {
            string first_tx_hash = 2;
        }
    }

    message GetAccountTransactions {
        string account_id = 1;
        TxPaginationMeta pagination_meta = 2;
    }

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Account ID", "account id to request transactions from", "<account_name>@<domain_id>", "makoto@soramitsu"
    "Page size", "size of the page to be returned by the query, if the response contains fewer transactions than a page size, then next tx hash will be empty in response", "page_size > 0", "5"
    "First tx hash", "hash of the first transaction in the page. If that field is not set — then the first transactions are returned", "hash in hex format", "bddd58404d1315e0eb27902c5d7c8eb0602c16238f005773df406bc191308929"

Response Schema
---------------

.. code-block:: proto

    message TransactionsPageResponse {
        repeated Transaction transactions = 1;
        uint32 all_transactions_size = 2;
        oneof next_page_tag {
            string next_tx_hash = 3;
        }
    }

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get account transactions", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get account transactions", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"
    "4", "Invalid pagination hash", "Supplied hash does not appear in any of the user's transactions", "Make sure hash is correct and try again"
    "5", "Invalid account id", "User with such account id does not exist", "Make sure account id is correct"

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Transactions", "an array of transactions for given account", "Committed transactions", "{tx1, tx2…}"
    "All transactions size", "total number of transactions created by the given account", "", "100"
    "Next transaction hash", "hash pointing to the next transaction after the last transaction in the page. Empty if a page contains the last transaction for the given account", "bddd58404d1315e0eb27902c5d7c8eb0602c16238f005773df406bc191308929"

Get Account Asset Transactions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Purpose
-------

`GetAccountAssetTransactions` query returns all transactions associated with given account and asset.

.. note:: This query uses pagination for query responses.

Request Schema
--------------

.. code-block:: proto

    message TxPaginationMeta {
        uint32 page_size = 1;
        oneof opt_first_tx_hash {
            string first_tx_hash = 2;
        }
    }

    message GetAccountAssetTransactions {
        string account_id = 1;
        string asset_id = 2;
        TxPaginationMeta pagination_meta = 3;
    }

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Account ID", "account id to request transactions from", "<account_name>@<domain_id>", "makoto@soramitsu"
    "Asset ID", "asset id in order to filter transactions containing this asset", "<asset_name>#<domain_id>", "jpy#japan"
    "Page size", "size of the page to be returned by the query, if the response contains fewer transactions than a page size, then next tx hash will be empty in response", "page_size > 0", "5"
    "First tx hash", "hash of the first transaction in the page. If that field is not set — then the first transactions are returned", "hash in hex format", "bddd58404d1315e0eb27902c5d7c8eb0602c16238f005773df406bc191308929"

Response Schema
---------------

.. code-block:: proto

    message TransactionsPageResponse {
        repeated Transaction transactions = 1;
        uint32 all_transactions_size = 2;
        oneof next_page_tag {
            string next_tx_hash = 3;
        }
    }

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Transactions", "an array of transactions for given account and asset", "Committed transactions", "{tx1, tx2…}"
    "All transactions size", "total number of transactions for given account and asset", "", "100"
    "Next transaction hash", "hash pointing to the next transaction after the last transaction in the page. Empty if a page contains the last transaction for given account and asset", "bddd58404d1315e0eb27902c5d7c8eb0602c16238f005773df406bc191308929"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get account asset transactions", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get account asset transactions", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"
    "4", "Invalid pagination hash", "Supplied hash does not appear in any of the user's transactions", "Make sure hash is correct and try again"
    "5", "Invalid account id", "User with such account id does not exist", "Make sure account id is correct"
    "6", "Invalid asset id", "Asset with such asset id does not exist", "Make sure asset id is correct"

Get Account Assets
^^^^^^^^^^^^^^^^^^

Purpose
-------

To get the state of all assets in an account (a balance), `GetAccountAssets` query can be used.

Request Schema
--------------

.. code-block:: proto

    message AccountAssetPaginationMeta {
        uint32 page_size = 1;
        oneof opt_first_asset_id {
            string first_asset_id = 2;
        }
    }

    message GetAccountAssets {
        string account_id = 1;
        AccountAssetPaginationMeta pagination_meta = 2;
    }

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Account ID", "account id to request balance from", "<account_name>@<domain_id>", "makoto@soramitsu"
    AccountAssetPaginationMeta.page_size, "Requested page size. The number of assets in response will not exceed this value. If the response was truncated, the asset id following the last one of response will be provided in next_asset_id.", 0 < page_size < 4294967296, 100
    AccountAssetPaginationMeta.first_asset_id, requested page start, name#domain, my_asset#my_domain

Response Schema
---------------
.. code-block:: proto

    message AccountAssetResponse {
        repeated AccountAsset account_assets = 1;
        uint32 total_number = 2;
        oneof opt_next_asset_id {
            string next_asset_id = 3;
        }
    }

    message AccountAsset {
        string asset_id = 1;
        string account_id = 2;
        string balance = 3;
    }

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Asset ID", "identifier of asset used for checking the balance", "<asset_name>#<domain_id>", "jpy#japan"
    "Account ID", "account which has this balance", "<account_name>@<domain_id>", "makoto@soramitsu"
    "Balance", "balance of the asset", "No less than 0", "200.20"
    total_number, number of assets matching query without page limits, 0 < total_number < 4294967296, 100500
    next_asset_id, the id of asset immediately following curent page, name#domain, my_asset#my_domain

.. note:: If page size is equal or greater than the number of assets matching other requested criteria, the next asset id will be unset in the response. Otherwie it contains the value that clients should use for the first asset id if they want to fetch the next page.


Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get account assets", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get account assets", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"
    "4", "Invalid pagination metadata", "Wrong page size or nonexistent first asset", "Set a valid page size, and make sure that asset id is valid, or leave first asset id unspecified"

Get Account Detail
^^^^^^^^^^^^^^^^^^

Purpose
-------

To get details of the account, `GetAccountDetail` query can be used. Account details are key-value pairs, splitted into writers categories. Writers are accounts, that added the corresponding account detail. Example of such structure is:

.. code-block:: json

    {
        "account@a_domain": {
            "age": 18,
            "hobbies": "crypto"
        },
        "account@b_domain": {
            "age": 20,
            "sports": "basketball"
        }
    }

Here, one can see four account details - "age", "hobbies" and "sports" - added by two writers - "account@a_domain" and "account@b_domain". All of these details, obviously, are about the same account.

Request Schema
--------------

.. code-block:: proto

    message GetAccountDetail {
      oneof opt_account_id {
        string account_id = 1;
      }
      oneof opt_key {
        string key = 2;
      }
      oneof opt_writer {
        string writer = 3;
      }
    }

.. note::
    Pay attention, that all fields are optional. Reasons will be described later.

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

        "Account ID", "account id to get details from", "<account_name>@<domain_id>", "account@domain"
        "Key", "key, under which to get details", "string", "age"
        "Writer", "account id of writer", "<account_name>@<domain_id>", "account@domain"

Response Schema
---------------

.. code-block:: proto

    message AccountDetailResponse {
      string detail = 1;
    }

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

        "Detail", "key-value pairs with account details", "JSON", "see below"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get account detail", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get account detail", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"

Usage Examples
--------------

Again, let's consider the example of details from the beginning and see how different variants of `GetAccountDetail` queries will change the resulting response.

.. code-block:: json

    {
        "account@a_domain": {
            "age": 18,
            "hobbies": "crypto"
        },
        "account@b_domain": {
            "age": 20,
            "sports": "basketball"
        }
    }

**account_id is not set**

If account_id is not set - other fields can be empty or not - it will automatically be substituted with query creator's account, which will lead to one of the next cases.

**only account_id is set**

In this case, all details about that account are going to be returned, leading to the following response:

.. code-block:: json

    {
        "account@a_domain": {
            "age": 18,
            "hobbies": "crypto"
        },
        "account@b_domain": {
            "age": 20,
            "sports": "basketball"
        }
    }

**account_id and key are set**

Here, details added by all writers under the key are going to be returned. For example, if we asked for the key "age", that's the response we would get:

.. code-block:: json

    {
        "account@a_domain": {
            "age": 18
        },
        "account@b_domain": {
            "age": 20
        }
    }

**account_id and writer are set**

Now, the response will contain all details about this account, added by one specific writer. For example, if we asked for writer "account@b_domain", we would get:

.. code-block:: json

    {
        "account@b_domain": {
            "age": 20,
            "sports": "basketball"
        }
    }

**account_id, key and writer are set**

Finally, if all three field are set, result will contain details, added the specific writer and under the specific key, for example, if we asked for key "age" and writer "account@a_domain", we would get:

.. code-block:: json

    {
        "account@a_domain": {
            "age": 18
        }
    }

Get Asset Info
^^^^^^^^^^^^^^

Purpose
-------

In order to get information on the given asset (as for now - its precision), user can send `GetAssetInfo` query.

Request Schema
--------------

.. code-block:: proto

    message GetAssetInfo {
        string asset_id = 1;
    }

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Asset ID", "asset id to know related information", "<asset_name>#<domain_id>", "jpy#japan"


Response Schema
---------------

.. code-block:: proto

    message Asset {
        string asset_id = 1;
        string domain_id = 2;
        uint32 precision = 3;
    }

.. note::
    Please note that due to a known issue you would not get any exception if you pass invalid precision value.
    Valid range is: 0 <= precision <= 255

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get asset info", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get asset info", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Asset ID", "identifier of asset used for checking the balance", "<asset_name>#<domain_id>", "jpy#japan"
    "Domain ID", "domain related to this asset", "RFC1035 [#f1]_, RFC1123 [#f2]_", "japan"
    "Precision", "number of digits after comma", "0 <= precision <= 255", "2"

Get Roles
^^^^^^^^^

Purpose
-------

To get existing roles in the system, a user can send `GetRoles` query to Iroha network.

Request Schema
--------------

.. code-block:: proto

    message GetRoles {
    }

Response Schema
---------------

.. code-block:: proto

    message RolesResponse {
        repeated string roles = 1;
    }

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Roles", "array of created roles in the network", "set of roles in the system", "{MoneyCreator, User, Admin, …}"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get roles", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get roles", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"

Get Role Permissions
^^^^^^^^^^^^^^^^^^^^

Purpose
-------

To get available permissions per role in the system, a user can send `GetRolePermissions` query to Iroha network.

Request Schema
--------------

.. code-block:: proto

    message GetRolePermissions {
        string role_id = 1;
    }

Request Structure
-----------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Role ID", "role to get permissions for", "existing role in the system", "MoneyCreator"

Response Schema
---------------

.. code-block:: proto

    message RolePermissionsResponse {
        repeated string permissions = 1;
    }

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Permissions", "array of permissions related to the role", "string of permissions related to the role", "{can_add_asset_qty, …}"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get role permissions", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get role permissions", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"

.. [#f1] https://www.ietf.org/rfc/rfc1035.txt
.. [#f2] https://www.ietf.org/rfc/rfc1123.txt


FetchCommits
^^^^^^^^^^^^^^^^^^^^

Purpose
-------

To get new blocks as soon as they are committed, a user can invoke `FetchCommits` RPC call to Iroha network.

Request Schema
--------------

No request arguments are needed


Response Schema
---------------

.. code-block:: proto

    message BlockQueryResponse {
      oneof response {
        BlockResponse block_response = 1;
        BlockErrorResponse block_error_response = 2;
      }
    }

Please note that it returns a stream of `BlockQueryResponse`.

Response Structure
------------------

.. csv-table::
    :header: "Field", "Description", "Constraint", "Example"
    :widths: 15, 30, 20, 15

    "Block", "Iroha block", "only committed blocks", "{ 'block_v1': ....}"

Possible Stateful Validation Errors
-----------------------------------

.. csv-table::
    :header: "Code", "Error Name", "Description", "How to solve"

    "1", "Could not get block streaming", "Internal error happened", "Try again or contact developers"
    "2", "No such permissions", "Query's creator does not have any of the permissions to get blocks", "Grant the necessary permission: individual, global or domain one"
    "3", "Invalid signatures", "Signatures of this query did not pass validation", "Add more signatures and make sure query's signatures are a subset of account's signatories"

Example
-------
You can check an example how to use this query here:
https://github.com/x3medima17/twitter


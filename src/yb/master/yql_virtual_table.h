// Copyright (c) YugaByte, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//

#ifndef YB_MASTER_YQL_VIRTUAL_TABLE_H
#define YB_MASTER_YQL_VIRTUAL_TABLE_H

#include "yb/common/entity_ids.h"
#include "yb/common/ql_rowblock.h"
#include "yb/common/ql_storage_interface.h"
#include "yb/master/master.h"
#include "yb/master/ts_descriptor.h"
#include "yb/master/util/yql_vtable_helpers.h"

namespace yb {
namespace master {

// A YQL virtual table which is based on in memory data.
class YQLVirtualTable : public common::YQLStorageIf {
 public:
  explicit YQLVirtualTable(const TableName& table_name,
                           const Master* const master,
                           const Schema& schema);

  // Access methods.
  const Schema& schema() const { return schema_; }

  const TableName& table_name() const { return table_name_; }

  //------------------------------------------------------------------------------------------------
  // CQL Support.
  //------------------------------------------------------------------------------------------------

  // Retrieves all the data for the yql virtual table in form of a QLRowBlock. This data is then
  // used by the iterator.
  virtual CHECKED_STATUS RetrieveData(const QLReadRequestPB& request,
                                      std::unique_ptr<QLRowBlock>* vtable) const = 0;

  CHECKED_STATUS GetIterator(const QLReadRequestPB& request,
                             const Schema& projection,
                             const Schema& schema,
                             const TransactionOperationContextOpt& txn_op_context,
                             const ReadHybridTime& read_time,
                             const common::QLScanSpec& spec,
                             std::unique_ptr<common::YQLRowwiseIteratorIf>* iter) const override;

  CHECKED_STATUS BuildYQLScanSpec(const QLReadRequestPB& request,
                                  const ReadHybridTime& read_time,
                                  const Schema& schema,
                                  bool include_static_columns,
                                  const Schema& static_projection,
                                  std::unique_ptr<common::QLScanSpec>* spec,
                                  std::unique_ptr<common::QLScanSpec>* static_row_spec,
                                  ReadHybridTime* req_read_time) const override;

  //------------------------------------------------------------------------------------------------
  // PGSQL Support.
  //------------------------------------------------------------------------------------------------

  virtual CHECKED_STATUS GetIterator(const PgsqlReadRequestPB& request,
                                     const Schema& projection,
                                     const Schema& schema,
                                     const TransactionOperationContextOpt& txn_op_context,
                                     const ReadHybridTime& read_time,
                                     const common::PgsqlScanSpec& spec,
                                     common::YQLRowwiseIteratorIf::UniPtr* iter) const override {
    LOG(FATAL) << "Postgresql system tables are not yet implemented";
    return Status::OK();
  }

  virtual CHECKED_STATUS BuildYQLScanSpec(const PgsqlReadRequestPB& request,
                                          const ReadHybridTime& read_time,
                                          const Schema& schema,
                                          std::unique_ptr<common::PgsqlScanSpec>* spec,
                                          ReadHybridTime* req_read_time) const override {
    LOG(FATAL) << "Postgresql system tables are not yet implemented";
    return Status::OK();
  }

 protected:
  // Finds the given column name in the schema and updates the specified column in the given row
  // with the provided value.
  template<class T>
  CHECKED_STATUS SetColumnValue(const std::string& col_name, const T& value, QLRow* row) const {
    int column_index = schema_.find_column(col_name);
    if (column_index == Schema::kColumnNotFound) {
      return STATUS_SUBSTITUTE(NotFound, "Couldn't find column $0 in schema", col_name);
    }
    const DataType data_type = schema_.column(column_index).type_info()->type();
    *(row->mutable_column(column_index)) = util::GetValue(value, data_type);
    return Status::OK();
  }

  // Get all live tserver descriptors sorted by their UUIDs. For cases like system.local and
  // system.peers tables to return the token map of each tserver node so that each maps to a
  // consistent token.
  void GetSortedLiveDescriptors(std::vector<std::shared_ptr<TSDescriptor>>* descs) const;

  const Master* const master_;
  TableName table_name_;
  Schema schema_;
};

}  // namespace master
}  // namespace yb
#endif // YB_MASTER_YQL_VIRTUAL_TABLE_H

//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// Parse Tree Declaration.
//
// This modules includes declarations of the base class for tree nodes. Parser whose rules are
// defined in parser_gram.y will create these nodes and link them together to form parse tree.
//--------------------------------------------------------------------------------------------------

#ifndef YB_SQL_PTREE_TREE_NODE_H_
#define YB_SQL_PTREE_TREE_NODE_H_

#include "yb/sql/ptree/yb_location.h"
#include "yb/sql/util/base_types.h"
#include "yb/sql/util/errcodes.h"

namespace yb {
namespace sql {
class SemContext;

enum class TreeNodeOpcode {
  kUndefined = 0,
  kTreeNode,
  kPTListNode,
  kPTCreateTable,
};

// TreeNode base class.
class TreeNode : public MCBase {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<TreeNode> SharedPtr;
  typedef MCSharedPtr<const TreeNode> SharedPtrConst;

  // Execute function.
  using ExecOperator = std::function<ErrorCode(TreeNode*)>;

  //------------------------------------------------------------------------------------------------
  // Public functions.
  explicit TreeNode(MemoryContext *memctx = nullptr, YBLocation::SharedPtr loc = nullptr);
  virtual ~TreeNode();

  // Node type.
  virtual TreeNodeOpcode opcode() const {
    return TreeNodeOpcode::kTreeNode;
  }

  // shared_ptr support.
  template<typename... TypeArgs>
  inline static TreeNode::SharedPtr MakeShared(MemoryContext *memctx, TypeArgs&&... args) {
    return MCMakeShared<TreeNode>(memctx, std::forward<TypeArgs>(args)...);
  }

  // Run semantics analysis on this node.
  virtual ErrorCode Analyze(SemContext *sem_context);

  // Access function to this node location.
  const YBLocation& loc() const {
    return *loc_;
  }

 protected:
  YBLocation::SharedPtr loc_;
};

}  // namespace sql
}  // namespace yb

#endif  // YB_SQL_PTREE_TREE_NODE_H_
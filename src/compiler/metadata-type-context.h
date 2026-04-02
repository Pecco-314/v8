#ifndef V8_COMPILER_METADATA_TYPE_CONTEXT_H_
#define V8_COMPILER_METADATA_TYPE_CONTEXT_H_

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/compiler/type-storage.h"

namespace v8::internal {
class OptimizedCompilationInfo;
}

namespace v8::internal::compiler {

class TFGraph;
class Node;

class MetadataTypeContext {
 public:
  explicit MetadataTypeContext(TFGraph* graph) {}

  void EnsureLoaded(OptimizedCompilationInfo* info, TypeStorage* storage);

  const std::vector<TypeAST>& param_types() const { return param_types_; }
  std::vector<TypeAST>& param_types() { return param_types_; }

  const TypeAST* StoreOwnedTypeAST(const TypeAST& ast);
  void SetNodeType(Node* node, const TypeAST* type_ast);
  const TypeAST* GetNodeType(Node* node) const;

  const std::string& script_hash() const { return script_hash_; }
  int current_start_pos() const { return current_start_pos_; }

 private:
  std::unordered_map<Node*, const TypeAST*> node_type_map_;
  std::deque<TypeAST> owned_typeasts_;
  std::vector<TypeAST> param_types_;
  std::string script_hash_;
  int current_start_pos_ = 0;
  bool loaded_ = false;
};

class MetadataTypeContextStore {
 public:
  static MetadataTypeContext& Get(TFGraph* graph);
};

}  // namespace v8::internal::compiler

#endif  // V8_COMPILER_METADATA_TYPE_CONTEXT_H_
#ifndef V8_COMPILER_METADATA_TYPE_HELPER_H_
#define V8_COMPILER_METADATA_TYPE_HELPER_H_

#include <optional>
#include <string>

#ifndef V8_COMPILER_TYPE_INJECTOR_DEBUG
#define V8_COMPILER_TYPE_INJECTOR_DEBUG 0
#endif

#include "src/codegen/optimized-compilation-info.h"
#include "src/compiler/common-operator.h"
#include "src/compiler/metadata-type-context.h"
#include "src/compiler/simplified-operator.h"
#include "src/compiler/turbofan-types.h"

namespace v8::internal::compiler {

class JSHeapBroker;

class MetadataTypeHelper {
 protected:
  MetadataTypeHelper(OptimizedCompilationInfo* compilation_info, TFGraph* graph,
                     CommonOperatorBuilder* common, JSHeapBroker* broker,
                     SimplifiedOperatorBuilder* simplified);

  void EnsureMetadataLoaded();

  const TypeAST* StoreOwnedTypeAST(const TypeAST& ast);
  void SetNodeType(Node* node, const TypeAST* type_ast);
  const TypeAST* GetNodeType(Node* node) const;

  Type TypeASTToType(const TypeAST& ast);
  std::optional<TypeAST> FindFieldInObj(const TypeAST& obj_ast,
                                        const std::string& field_name);
  std::optional<TypeAST> GetElementTypeInArray(const TypeAST& array_ast);
  std::optional<TypeAST> GetElementTypeInTuple(const TypeAST& tuple_ast,
                                               int index);
  std::optional<int> TryGetConstantIndex(Node* node);
  std::optional<TypeAST> GetNodeTypeAST(Node* node);
  std::optional<TypeAST> GetFunctionReturnType(int start_pos);

  OptimizedCompilationInfo* compilation_info_;
  TFGraph* graph_;
  CommonOperatorBuilder* common_;
  JSHeapBroker* broker_;
  SimplifiedOperatorBuilder* simplified_;
  TypeStorage* storage_;
  MetadataTypeContext& context_;
};

}  // namespace v8::internal::compiler

#endif  // V8_COMPILER_METADATA_TYPE_HELPER_H_
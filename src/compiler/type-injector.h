#ifndef V8_COMPILER_TYPE_INJECTOR_H_
#define V8_COMPILER_TYPE_INJECTOR_H_

#include <optional>
#include <vector>

#ifndef V8_COMPILER_TYPE_INJECTOR_DEBUG
#define V8_COMPILER_TYPE_INJECTOR_DEBUG 0
#endif

#include "src/codegen/optimized-compilation-info.h"
#include "src/compiler/turbofan-graph.h"
#include "src/compiler/type-storage.h"

namespace v8 {
namespace internal {
namespace compiler {

class TypeInjector {
 public:
  TypeInjector(OptimizedCompilationInfo* compilation_info, TFGraph* graph)
    : compilation_info_(compilation_info), graph_(graph) {}
  void Run();

 private:
  Type TypeASTToType(const TypeAST& ast);
  std::optional<TypeAST> FindFieldInInterface(const TypeAST& interface_ast,
                                               const std::string& field_name);
  std::optional<TypeAST> GetElementTypeInArray(const TypeAST& array_ast);
  std::optional<TypeAST> GetElementTypeInTuple(const TypeAST& tuple_ast,
                                                int index);
  std::optional<int> TryGetConstantIndex(Node* node);
  std::optional<TypeAST> GetNodeTypeAST(Node* node);
  std::optional<TypeAST> GetNodeTypeASTWithIndex(Node* node, int index);
  void ProcessLoadFieldNode(Node* node);
  void ProcessLoadElementNode(Node* node);
#if V8_COMPILER_TYPE_INJECTOR_DEBUG
  void InspectLoadFieldNode(Node* node);
  void InspectLoadElementNode(Node* node);
#endif
  
  OptimizedCompilationInfo* compilation_info_;
  TFGraph* graph_;

  std::vector<TypeAST> param_types_;
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_TYPE_INJECTOR_H_
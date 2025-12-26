#ifndef V8_COMPILER_TYPE_INJECTOR_H_
#define V8_COMPILER_TYPE_INJECTOR_H_

#include <optional>
#include <vector>

#ifndef V8_COMPILER_TYPE_INJECTOR_DEBUG
#define V8_COMPILER_TYPE_INJECTOR_DEBUG 0
#endif

#include "src/codegen/optimized-compilation-info.h"
#include "src/compiler/common-operator.h"
#include "src/compiler/machine-operator.h"
#include "src/compiler/simplified-operator.h"
#include "src/compiler/turbofan-graph.h"
#include "src/compiler/type-storage.h"

namespace v8 {
namespace internal {
namespace compiler {

class TypeInjector {
 public:
  TypeInjector(OptimizedCompilationInfo* compilation_info, TFGraph* graph,
               CommonOperatorBuilder* common, JSHeapBroker* broker,
               SimplifiedOperatorBuilder* simplified)
    : compilation_info_(compilation_info), graph_(graph), common_(common), broker_(broker), 
      machine_(graph->zone()), simplified_(simplified) {}
  void Run();

 private:
  Type TypeASTToType(const TypeAST& ast);
  std::optional<TypeAST> FindFieldInObj(const TypeAST& obj_ast,
                                         const std::string& field_name);
  std::optional<TypeAST> GetElementTypeInArray(const TypeAST& array_ast);
  std::optional<TypeAST> GetElementTypeInTuple(const TypeAST& tuple_ast,
                                                int index);
  std::optional<int> TryGetConstantIndex(Node* node);
  std::optional<TypeAST> GetNodeTypeAST(Node* node);
  std::optional<TypeAST> GetNodeTypeASTWithIndex(Node* node, int index);
  std::optional<TypeAST> GetFunctionReturnType(int start_pos);
  void ProcessLoadFieldNode(Node* node);
  void ProcessLoadElementNode(Node* node);
  void ProcessJSCallNode(Node* node);
  void ProcessCheckMapsNode(Node* node);
  void ProcessRawInt32BinaryOp(Node* node);
  void RemoveTupleBoundsCheck(Node* load_element_node, Node* check_bounds_node,
                               Node* index_constant);
  void ReplaceTupleLengthWithConstant(Node* load_field_node, int tuple_length);
  
  OptimizedCompilationInfo* compilation_info_;
  TFGraph* graph_;
  CommonOperatorBuilder* common_;
  JSHeapBroker* broker_;
  MachineOperatorBuilder machine_;
  SimplifiedOperatorBuilder* simplified_;

  std::vector<TypeAST> param_types_;
  std::string script_hash_;
  TypeStorage* storage_;
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_TYPE_INJECTOR_H_
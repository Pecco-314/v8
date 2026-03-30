#ifndef V8_COMPILER_METADATA_BASED_GRAPH_OPTIMIZER_H_
#define V8_COMPILER_METADATA_BASED_GRAPH_OPTIMIZER_H_

#include "src/compiler/all-nodes.h"
#include "src/compiler/js-graph.h"
#include "src/compiler/js-operator.h"
#include "src/compiler/metadata-type-helper.h"

namespace v8::internal::compiler {

class MetadataBasedGraphOptimizer : private MetadataTypeHelper {
 public:
  MetadataBasedGraphOptimizer(OptimizedCompilationInfo* compilation_info,
                              TFGraph* graph, CommonOperatorBuilder* common,
                              JSHeapBroker* broker,
                SimplifiedOperatorBuilder* simplified,
                JSOperatorBuilder* javascript,
                JSGraph* jsgraph)
      : MetadataTypeHelper(compilation_info, graph, common, broker,
               simplified),
    javascript_(javascript),
    jsgraph_(jsgraph) {
  }

  void Run();

 private:
  void OptimizeTupleLength(Node* node);
  void OptimizeLoadElementBounds(Node* node);
  void RemoveTupleBoundsCheck(Node* load_element_node, Node* check_bounds_node,
                              Node* index_constant);
  void ReplaceTupleLengthWithConstant(Node* load_field_node, int tuple_length);
  void ProcessCheckMapsNode(Node* node);
  void OptimizeRawIntDivModZeroGuard(Node* node);
  bool IsRawInt32Like(const std::optional<TypeAST>& type_opt) const;
  bool IsRawInt32Node(Node* node);
  bool IsNumericConstant(Node* node) const;

  JSOperatorBuilder* javascript_;
  JSGraph* jsgraph_;
};

}  // namespace v8::internal::compiler

#endif  // V8_COMPILER_METADATA_BASED_GRAPH_OPTIMIZER_H_
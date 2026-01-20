#ifndef V8_COMPILER_METADATA_BASED_GRAPH_OPTIMIZER_H_
#define V8_COMPILER_METADATA_BASED_GRAPH_OPTIMIZER_H_

#include "src/compiler/all-nodes.h"
#include "src/compiler/metadata-type-helper.h"

namespace v8::internal::compiler {

class MetadataBasedGraphOptimizer : private MetadataTypeHelper {
 public:
  MetadataBasedGraphOptimizer(OptimizedCompilationInfo* compilation_info,
                              TFGraph* graph, CommonOperatorBuilder* common,
                              JSHeapBroker* broker,
                              SimplifiedOperatorBuilder* simplified)
      : MetadataTypeHelper(compilation_info, graph, common, broker, simplified) {
  }

  void Run();

 private:
  void OptimizeTupleLength(Node* node);
  void OptimizeLoadElementBounds(Node* node);
  void RemoveTupleBoundsCheck(Node* load_element_node, Node* check_bounds_node,
                              Node* index_constant);
  void ReplaceTupleLengthWithConstant(Node* load_field_node, int tuple_length);
  void ProcessCheckMapsNode(Node* node);
};

}  // namespace v8::internal::compiler

#endif  // V8_COMPILER_METADATA_BASED_GRAPH_OPTIMIZER_H_
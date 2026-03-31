#ifndef V8_COMPILER_METADATA_TYPE_ANNOTATOR_H_
#define V8_COMPILER_METADATA_TYPE_ANNOTATOR_H_

#include "src/compiler/all-nodes.h"
#include "src/compiler/metadata-type-helper.h"

namespace v8::internal::compiler {

class MetadataTypeAnnotator : private MetadataTypeHelper {
 public:
  MetadataTypeAnnotator(OptimizedCompilationInfo* compilation_info,
                        TFGraph* graph, CommonOperatorBuilder* common,
                        JSHeapBroker* broker,
                        SimplifiedOperatorBuilder* simplified)
      : MetadataTypeHelper(compilation_info, graph, common, broker, simplified) {
  }

  void Run();

 private:
  void RunTypeAnnotation(AllNodes& all);
  void ProcessLoadFieldNode(Node* node);
  void ProcessLoadElementNode(Node* node);
  void ProcessJSKeyedPropertyNode(Node* node);
  void ProcessJSCallNode(Node* node);
};

}  // namespace v8::internal::compiler

#endif  // V8_COMPILER_METADATA_TYPE_ANNOTATOR_H_
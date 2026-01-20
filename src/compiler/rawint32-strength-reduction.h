#ifndef V8_COMPILER_RAWINT32_STRENGTH_REDUCTION_H_
#define V8_COMPILER_RAWINT32_STRENGTH_REDUCTION_H_

#include "src/codegen/optimized-compilation-info.h"
#include "src/compiler/common-operator.h"
#include "src/compiler/turbofan-graph.h"

namespace v8::internal::compiler {

class JSHeapBroker;
class SimplifiedOperatorBuilder;

class RawInt32StrengthReduction {
 public:
  RawInt32StrengthReduction(OptimizedCompilationInfo* compilation_info,
                            TFGraph* graph, CommonOperatorBuilder* common,
                            JSHeapBroker* broker,
                            SimplifiedOperatorBuilder* simplified) {}

  void Run();
};

}  // namespace v8::internal::compiler

#endif  // V8_COMPILER_RAWINT32_STRENGTH_REDUCTION_H_
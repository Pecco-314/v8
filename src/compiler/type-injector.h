#ifndef V8_COMPILER_TYPE_INJECTOR_H_
#define V8_COMPILER_TYPE_INJECTOR_H_

#include "src/codegen/optimized-compilation-info.h"
#include "src/compiler/turbofan-graph.h"

namespace v8 {
namespace internal {
namespace compiler {

class TypeInjector {
 public:
  TypeInjector(OptimizedCompilationInfo* compilation_info, TFGraph* graph)
    : compilation_info_(compilation_info), graph_(graph) {}
  void Run();

 private:
  OptimizedCompilationInfo* compilation_info_;
  TFGraph* graph_;
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_TYPE_INJECTOR_H_
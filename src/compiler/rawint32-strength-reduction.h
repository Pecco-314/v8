#ifndef V8_COMPILER_RAWINT32_STRENGTH_REDUCTION_H_
#define V8_COMPILER_RAWINT32_STRENGTH_REDUCTION_H_

#include "src/codegen/optimized-compilation-info.h"
#include "src/compiler/common-operator.h"
#include "src/compiler/metadata-type-helper.h"
#include "src/compiler/machine-operator.h"
#include "src/compiler/turbofan-graph.h"

namespace v8::internal::compiler {

class JSHeapBroker;
class SimplifiedOperatorBuilder;

class RawInt32StrengthReduction : private MetadataTypeHelper {
 public:
  enum class RawIntKind : uint8_t {
    kInt32,
    kUint32,
    kInt64,
    kUint64,
  };

  RawInt32StrengthReduction(OptimizedCompilationInfo* compilation_info,
                            TFGraph* graph, CommonOperatorBuilder* common,
                            JSHeapBroker* broker,
                            SimplifiedOperatorBuilder* simplified,
              MachineOperatorBuilder* machine)
      : MetadataTypeHelper(compilation_info, graph, common, broker,
                           simplified),
      machine_(machine) {}

  void Run();

 private:
  bool IsRawTyped(Node* node, RawIntKind kind);
  bool IsLiteralCompatible(Node* node, RawIntKind kind);
  bool IsTypeOrLiteralCompatible(Node* node, RawIntKind kind);

  void ReduceCheckedBinop(Node* node, const Operator* replacement_op,
                          RawIntKind kind);
  void ReduceCheckedInt32DivOrModClosed(Node* node, bool is_div);
  void ReduceCheckedUint32DivOrModClosed(Node* node, bool is_div);
  void ReduceCheckedInt64DivOrMod(Node* node, bool is_div);
  void MaybeChangeCheckedDivModOp(Node* node, const Operator* replacement_op,
                                  RawIntKind kind);
  void ReduceRawFloat64DivOrMod(Node* node, bool is_div);

  MachineOperatorBuilder* machine_;
};

}  // namespace v8::internal::compiler

#endif  // V8_COMPILER_RAWINT32_STRENGTH_REDUCTION_H_
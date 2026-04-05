#include "src/compiler/rawint32-strength-reduction.h"

#include "src/compiler/all-nodes.h"
#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"

#include "src/base/logging.h"
#include "src/base/division-by-constant.h"
#include "src/flags/flags.h"

#include <cmath>
#include <limits>

namespace v8::internal::compiler {

#if V8_COMPILER_TYPE_INJECTOR_DEBUG
#define RAWINT32_SR_DEBUG(...)                                      \
  do {                                                              \
    PrintF("[RawInt32StrengthReduction] " __VA_ARGS__);            \
    PrintF("\n");                                                  \
  } while (false)
#else
#define RAWINT32_SR_DEBUG(...) ((void)0)
#endif

#define RAWINT32_SR_TRACE(...)                                              \
  do {                                                                      \
    if (v8_flags.trace_turbo) {                                             \
      PrintF("[RawInt32StrengthReduction] " __VA_ARGS__);                  \
      PrintF("\n");                                                       \
    }                                                                       \
  } while (false)

namespace {

[[maybe_unused]] const char* RawIntKindName(
  RawInt32StrengthReduction::RawIntKind kind) {
  switch (kind) {
    case RawInt32StrengthReduction::RawIntKind::kInt32:
      return "rawint32";
    case RawInt32StrengthReduction::RawIntKind::kUint32:
      return "rawuint32";
    case RawInt32StrengthReduction::RawIntKind::kInt64:
      return "rawint64";
    case RawInt32StrengthReduction::RawIntKind::kUint64:
      return "rawuint64";
  }
}

[[maybe_unused]] const char* TypeKindName(TypeAST::TypeKind kind) {
  switch (kind) {
    case TypeAST::Any:
      return "any";
    case TypeAST::Num:
      return "num";
    case TypeAST::Str:
      return "str";
    case TypeAST::Bool:
      return "bool";
    case TypeAST::Symbol:
      return "symbol";
    case TypeAST::RawInt32:
      return "rawint32";
    case TypeAST::RawUint32:
      return "rawuint32";
    case TypeAST::RawInt64:
      return "rawint64";
    case TypeAST::RawUint64:
      return "rawuint64";
    case TypeAST::BigInt:
      return "bigint";
    case TypeAST::Arr:
      return "arr";
    case TypeAST::Tuple:
      return "tuple";
    case TypeAST::Obj:
      return "obj";
    case TypeAST::Void:
      return "void";
  }
}

const char* TypeKindNameOrNone(const std::optional<TypeAST>& type_opt) {
  if (!type_opt.has_value()) return "<none>";
  return TypeKindName(type_opt->kind);
}

TypeAST::TypeKind RawIntKindToTypeKind(
    RawInt32StrengthReduction::RawIntKind kind) {
  switch (kind) {
    case RawInt32StrengthReduction::RawIntKind::kInt32:
      return TypeAST::RawInt32;
    case RawInt32StrengthReduction::RawIntKind::kUint32:
      return TypeAST::RawUint32;
    case RawInt32StrengthReduction::RawIntKind::kInt64:
      return TypeAST::RawInt64;
    case RawInt32StrengthReduction::RawIntKind::kUint64:
      return TypeAST::RawUint64;
  }
}

TypeAST MakeRawIntAst(RawInt32StrengthReduction::RawIntKind kind) {
  TypeAST ast;
  ast.kind = RawIntKindToTypeKind(kind);
  return ast;
}

Node* Int32Add(TFGraph* graph, MachineOperatorBuilder* machine, Node* lhs,
               Node* rhs) {
  return graph->NewNode(machine->Int32Add(), lhs, rhs);
}

Node* Int32Sub(TFGraph* graph, MachineOperatorBuilder* machine, Node* lhs,
               Node* rhs) {
  return graph->NewNode(machine->Int32Sub(), lhs, rhs);
}

Node* Int32Mul(TFGraph* graph, MachineOperatorBuilder* machine, Node* lhs,
               Node* rhs) {
  return graph->NewNode(machine->Int32Mul(), lhs, rhs);
}

Node* Word32Sar(TFGraph* graph, MachineOperatorBuilder* machine,
                CommonOperatorBuilder* common, Node* lhs, uint32_t rhs) {
  if (rhs == 0) return lhs;
  return graph->NewNode(machine->Word32Sar(), lhs,
                        graph->NewNode(common->Int32Constant(rhs)));
}

Node* Word32Shr(TFGraph* graph, MachineOperatorBuilder* machine,
                CommonOperatorBuilder* common, Node* lhs, uint32_t rhs) {
  if (rhs == 0) return lhs;
  return graph->NewNode(machine->Word32Shr(), lhs,
                        graph->NewNode(common->Int32Constant(rhs)));
}

Node* BuildInt32DivByConst(TFGraph* graph, MachineOperatorBuilder* machine,
                           CommonOperatorBuilder* common, Node* dividend,
                           int32_t divisor) {
  DCHECK_NE(0, divisor);
  DCHECK_NE(std::numeric_limits<int32_t>::min(), divisor);

  base::MagicNumbersForDivision<uint32_t> const mag =
      base::SignedDivisionByConstant(base::bit_cast<uint32_t>(divisor));

  Node* quotient = graph->NewNode(
      machine->Int32MulHigh(), dividend,
      graph->NewNode(common->Int32Constant(
          base::bit_cast<int32_t>(mag.multiplier))));
  if (divisor > 0 && base::bit_cast<int32_t>(mag.multiplier) < 0) {
    quotient = Int32Add(graph, machine, quotient, dividend);
  } else if (divisor < 0 && base::bit_cast<int32_t>(mag.multiplier) > 0) {
    quotient = Int32Sub(graph, machine, quotient, dividend);
  }

  return Int32Add(graph, machine,
                  Word32Sar(graph, machine, common, quotient, mag.shift),
                  Word32Shr(graph, machine, common, dividend, 31));
}

Node* BuildUint32DivByConst(TFGraph* graph, MachineOperatorBuilder* machine,
                            CommonOperatorBuilder* common, Node* dividend,
                            uint32_t divisor) {
  DCHECK_LT(0u, divisor);

  unsigned const shift = base::bits::CountTrailingZeros(divisor);
  dividend = Word32Shr(graph, machine, common, dividend, shift);
  divisor >>= shift;

  base::MagicNumbersForDivision<uint32_t> const mag =
      base::UnsignedDivisionByConstant(divisor, shift);

  Node* quotient = graph->NewNode(
      machine->Uint32MulHigh(), dividend,
      graph->NewNode(common->Int32Constant(
          base::bit_cast<int32_t>(mag.multiplier))));
  if (mag.add) {
    DCHECK_LE(1u, mag.shift);
    quotient = Word32Shr(
        graph, machine, common,
        Int32Add(graph, machine,
                 Word32Shr(graph, machine, common,
                           Int32Sub(graph, machine, dividend, quotient), 1),
                 quotient),
        mag.shift - 1);
  } else {
    quotient = Word32Shr(graph, machine, common, quotient, mag.shift);
  }
  return quotient;
}

Node* BuildInt32VarDivClosed(TFGraph* graph, MachineOperatorBuilder* machine,
                             CommonOperatorBuilder* common, Node* lhs_i32,
                             Node* rhs_i32, Node* control_input) {
    Node* const zero_i32 = graph->NewNode(common->Int32Constant(0));
    Node* const minus_one_i32 = graph->NewNode(common->Int32Constant(-1));

    const Operator* const merge2 = common->Merge(2);
    const Operator* const phi_word32 =
      common->Phi(MachineRepresentation::kWord32, 2);

    Node* check_zero = graph->NewNode(machine->Word32Equal(), rhs_i32, zero_i32);
    Node* branch_zero = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_zero, control_input);
    Node* if_zero = graph->NewNode(common->IfTrue(), branch_zero);
    Node* if_nonzero = graph->NewNode(common->IfFalse(), branch_zero);

    Node* check_minus_one =
      graph->NewNode(machine->Word32Equal(), rhs_i32, minus_one_i32);
    Node* branch_minus_one = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_minus_one, if_nonzero);
    Node* if_minus_one = graph->NewNode(common->IfTrue(), branch_minus_one);
    Node* if_div = graph->NewNode(common->IfFalse(), branch_minus_one);

    Node* div_minus_one = graph->NewNode(machine->Int32Sub(), zero_i32, lhs_i32);
    Node* div_normal =
      graph->NewNode(machine->Int32Div(), lhs_i32, rhs_i32, if_div);
    Node* merge_nonzero = graph->NewNode(merge2, if_minus_one, if_div);
    Node* div_nonzero_word32 =
      graph->NewNode(phi_word32, div_minus_one, div_normal, merge_nonzero);

    Node* merge = graph->NewNode(merge2, if_zero, merge_nonzero);
    return graph->NewNode(phi_word32, zero_i32, div_nonzero_word32, merge);
  }

Node* BuildUint32VarDivClosed(TFGraph* graph, MachineOperatorBuilder* machine,
                              CommonOperatorBuilder* common, Node* lhs_u32,
                              Node* rhs_u32, Node* control_input) {
    Node* const zero_i32 = graph->NewNode(common->Int32Constant(0));
    const Operator* const merge2 = common->Merge(2);
    const Operator* const phi_word32 =
      common->Phi(MachineRepresentation::kWord32, 2);

    Node* check_zero = graph->NewNode(machine->Word32Equal(), rhs_u32, zero_i32);
    Node* branch_zero = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_zero, control_input);
    Node* if_zero = graph->NewNode(common->IfTrue(), branch_zero);
    Node* if_nonzero = graph->NewNode(common->IfFalse(), branch_zero);
    Node* div_u32 =
      graph->NewNode(machine->Uint32Div(), lhs_u32, rhs_u32, if_nonzero);

    Node* merge = graph->NewNode(merge2, if_zero, if_nonzero);
    return graph->NewNode(phi_word32, zero_i32, div_u32, merge);
  }

Node* BuildInt32VarModClosed(TFGraph* graph, MachineOperatorBuilder* machine,
                             CommonOperatorBuilder* common, Node* lhs_i32,
                             Node* rhs_i32, Node* control_input) {
    Node* const zero_i32 = graph->NewNode(common->Int32Constant(0));
    const Operator* const merge2 = common->Merge(2);
    const Operator* const phi_word32 =
      common->Phi(MachineRepresentation::kWord32, 2);

    Node* check_zero = graph->NewNode(machine->Word32Equal(), rhs_i32, zero_i32);
    Node* branch_zero = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_zero, control_input);
    Node* if_zero = graph->NewNode(common->IfTrue(), branch_zero);
    Node* if_nonzero = graph->NewNode(common->IfFalse(), branch_zero);
    Node* mod_nonzero =
      graph->NewNode(machine->Int32Mod(), lhs_i32, rhs_i32, if_nonzero);

    Node* merge = graph->NewNode(merge2, if_zero, if_nonzero);
    return graph->NewNode(phi_word32, zero_i32, mod_nonzero, merge);
  }

Node* BuildUint32VarModClosed(TFGraph* graph, MachineOperatorBuilder* machine,
                              CommonOperatorBuilder* common, Node* lhs_u32,
                              Node* rhs_u32, Node* control_input) {
    Node* const zero_i32 = graph->NewNode(common->Int32Constant(0));
    const Operator* const merge2 = common->Merge(2);
    const Operator* const phi_word32 =
      common->Phi(MachineRepresentation::kWord32, 2);

    Node* check_zero = graph->NewNode(machine->Word32Equal(), rhs_u32, zero_i32);
    Node* branch_zero = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_zero, control_input);
    Node* if_zero = graph->NewNode(common->IfTrue(), branch_zero);
    Node* if_nonzero = graph->NewNode(common->IfFalse(), branch_zero);
    Node* mod_nonzero =
      graph->NewNode(machine->Uint32Mod(), lhs_u32, rhs_u32, if_nonzero);

    Node* merge = graph->NewNode(merge2, if_zero, if_nonzero);
    return graph->NewNode(phi_word32, zero_i32, mod_nonzero, merge);
  }

void ReplaceValueUsesAndKill(TFGraph* graph, Node* old_node,
                             Node* replacement) {
  ZoneVector<std::pair<Node*, int>> value_edges(graph->zone());
  for (Edge edge : old_node->use_edges()) {
    Node* use = edge.from();
    int index = edge.index();
    if (index < NodeProperties::FirstEffectIndex(use)) {
      value_edges.push_back(std::make_pair(use, index));
    }
  }

  for (auto& pair : value_edges) {
    pair.first->ReplaceInput(pair.second, replacement);
  }
  old_node->Kill();
}

Node* UnwrapConstantNode(Node* node) {
  while (node != nullptr) {
    switch (node->opcode()) {
      case IrOpcode::kChangeInt32ToInt64:
      case IrOpcode::kChangeUint32ToUint64:
      case IrOpcode::kCheckBigInt:
      case IrOpcode::kCheckedBigIntToBigInt64:
      case IrOpcode::kTruncateBigIntToWord64:
      case IrOpcode::kCheckedUint32Bounds:
      case IrOpcode::kCheckedUint64Bounds:
      case IrOpcode::kCheckBounds:
        node = node->InputAt(0);
        break;
      default:
        return node;
    }
  }
  return node;
}

bool TryGetInt64Literal(Node* node, int64_t* out) {
  node = UnwrapConstantNode(node);
  if (node == nullptr) return false;
  switch (node->opcode()) {
    case IrOpcode::kInt32Constant:
      *out = static_cast<int64_t>(OpParameter<int32_t>(node->op()));
      return true;
    case IrOpcode::kInt64Constant:
      *out = OpParameter<int64_t>(node->op());
      return true;
    case IrOpcode::kNumberConstant: {
      double value = OpParameter<double>(node->op());
      if (!std::isfinite(value) || std::trunc(value) != value) return false;
      if (value < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
          value > static_cast<double>(std::numeric_limits<int64_t>::max())) {
        return false;
      }
      *out = static_cast<int64_t>(value);
      return true;
    }
    default:
      return false;
  }
}

bool IsBigIntConstantThroughCheckedConversion(Node* node) {
  bool saw_bigint_conversion = false;
  while (node != nullptr) {
    switch (node->opcode()) {
      case IrOpcode::kCheckBigInt:
      case IrOpcode::kCheckedBigIntToBigInt64:
      case IrOpcode::kTruncateBigIntToWord64:
        if (node->InputCount() == 0) return false;
        saw_bigint_conversion = true;
        node = node->InputAt(0);
        continue;
      case IrOpcode::kHeapConstant:
      case IrOpcode::kCompressedHeapConstant:
        return saw_bigint_conversion;
      default:
        return false;
    }
  }
  return false;
}

bool TryGetFloat64IntegralConstant(Node* node, int64_t* out) {
  node = UnwrapConstantNode(node);
  if (node == nullptr) return false;
  double value;
  switch (node->opcode()) {
    case IrOpcode::kFloat64Constant:
      value = OpParameter<double>(node->op());
      break;
    case IrOpcode::kNumberConstant:
      value = OpParameter<double>(node->op());
      break;
    case IrOpcode::kInt32Constant:
      *out = static_cast<int64_t>(OpParameter<int32_t>(node->op()));
      return true;
    case IrOpcode::kInt64Constant:
      *out = OpParameter<int64_t>(node->op());
      return true;
    default:
      return false;
  }

  if (!std::isfinite(value) || std::trunc(value) != value) return false;
  if (value < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
      value > static_cast<double>(std::numeric_limits<int64_t>::max())) {
    return false;
  }
  *out = static_cast<int64_t>(value);
  return true;
}

}  // namespace

bool RawInt32StrengthReduction::IsRawTyped(Node* node, RawIntKind kind) {
  while (node != nullptr) {
    switch (node->opcode()) {
      case IrOpcode::kChangeTaggedToFloat64:
      case IrOpcode::kCheckedTaggedToFloat64:
      case IrOpcode::kCheckedTaggedSignedToInt32:
      case IrOpcode::kCheckedTaggedToInt32:
      case IrOpcode::kChangeTaggedSignedToInt32:
      case IrOpcode::kChangeTaggedToInt32:
      case IrOpcode::kChangeFloat64ToInt32:
      case IrOpcode::kChangeFloat64ToUint32:
      case IrOpcode::kChangeInt32ToFloat64:
      case IrOpcode::kChangeUint32ToFloat64:
      case IrOpcode::kChangeFloat64ToTagged:
      case IrOpcode::kCheckBigInt:
      case IrOpcode::kCheckedBigIntToBigInt64:
      case IrOpcode::kTruncateBigIntToWord64:
      case IrOpcode::kCheckedUint32Bounds:
      case IrOpcode::kCheckedUint64Bounds:
      case IrOpcode::kCheckBounds:
      case IrOpcode::kChangeInt32ToInt64:
      case IrOpcode::kChangeUint32ToUint64:
        if (node->InputCount() == 0) break;
        node = node->InputAt(0);
        continue;
      default:
        break;
    }
    break;
  }

  auto type_opt = GetNodeTypeAST(node);
  if (!type_opt.has_value()) return false;
  switch (kind) {
    case RawIntKind::kInt32:
      return type_opt->kind == TypeAST::RawInt32;
    case RawIntKind::kUint32:
      return type_opt->kind == TypeAST::RawUint32;
    case RawIntKind::kInt64:
      return type_opt->kind == TypeAST::RawInt64;
    case RawIntKind::kUint64:
      return type_opt->kind == TypeAST::RawUint64;
  }
}

bool RawInt32StrengthReduction::IsLiteralCompatible(Node* node,
                                                    RawIntKind kind) {
  if ((kind == RawIntKind::kInt64 || kind == RawIntKind::kUint64) &&
      IsBigIntConstantThroughCheckedConversion(node)) {
    return true;
  }

  int64_t value = 0;
  if (!TryGetInt64Literal(node, &value)) return false;
  switch (kind) {
    case RawIntKind::kInt32:
      return value >= std::numeric_limits<int32_t>::min() &&
             value <= std::numeric_limits<int32_t>::max();
    case RawIntKind::kUint32:
      return value >= 0 &&
             static_cast<uint64_t>(value) <=
                 std::numeric_limits<uint32_t>::max();
    case RawIntKind::kInt64:
      return true;
    case RawIntKind::kUint64: {
      Node* unwrapped = UnwrapConstantNode(node);
      if (unwrapped != nullptr && unwrapped->opcode() == IrOpcode::kInt64Constant) {
        return true;
      }
      return value >= 0;
    }
  }
}

bool RawInt32StrengthReduction::IsTypeOrLiteralCompatible(Node* node,
                                                          RawIntKind kind) {
  return IsRawTyped(node, kind) || IsLiteralCompatible(node, kind);
}

bool RawInt32StrengthReduction::CurrentFunctionHasRawProofAnnotation() {
  auto typemap = storage_->GetTypeMap(context_.script_hash());
  RAWINT32_SR_TRACE("raw-proof-check script_hash=%s typemap_size=%d",
                    context_.script_hash().c_str(),
                    static_cast<int>(typemap.size()));

  auto& param_types = context_.param_types();
  RAWINT32_SR_TRACE("raw-proof-check start_pos=%d param_count=%d",
                    context_.current_start_pos(),
                    static_cast<int>(param_types.size()));
  for (const auto& ast : param_types) {
    RAWINT32_SR_TRACE("raw-proof-check param kind=%s", TypeKindName(ast.kind));
    if (ast.kind == TypeAST::RawInt32 || ast.kind == TypeAST::RawUint32 ||
        ast.kind == TypeAST::RawInt64 || ast.kind == TypeAST::RawUint64) {
      RAWINT32_SR_TRACE("raw-proof-check param-hit kind=%s",
                        TypeKindName(ast.kind));
      return true;
    }
  }

  auto return_type_opt = GetFunctionReturnType(context_.current_start_pos());
  if (!return_type_opt.has_value()) {
    RAWINT32_SR_TRACE("raw-proof-check return-miss start_pos=%d",
                      context_.current_start_pos());
    int printed = 0;
    for (const auto& entry : typemap) {
      RAWINT32_SR_TRACE("raw-proof-check typemap-key[%d]=%d", printed,
                        entry.first);
      if (++printed >= 8) break;
    }
    return false;
  }
  TypeAST::TypeKind kind = return_type_opt->kind;
  RAWINT32_SR_TRACE("raw-proof-check return-hit kind=%s", TypeKindName(kind));
  return kind == TypeAST::RawInt32 || kind == TypeAST::RawUint32 ||
         kind == TypeAST::RawInt64 || kind == TypeAST::RawUint64;
}

bool RawInt32StrengthReduction::IsInt32SemanticNode(Node* node, int depth) {
  if (node == nullptr) return false;
  if (depth > 8) return false;

  if (IsTypeOrLiteralCompatible(node, RawIntKind::kInt32)) {
    return true;
  }

  switch (node->opcode()) {
    case IrOpcode::kInt32Constant:
      return true;
    case IrOpcode::kCheckedTaggedSignedToInt32:
    case IrOpcode::kCheckedTaggedToInt32:
    case IrOpcode::kChangeTaggedSignedToInt32:
    case IrOpcode::kChangeTaggedToInt32:
    case IrOpcode::kChangeFloat64ToInt32:
      return true;
    case IrOpcode::kChangeInt32ToTagged:
    case IrOpcode::kChangeInt31ToTaggedSigned:
    case IrOpcode::kChangeUint32ToTagged:
    case IrOpcode::kCheckedUint32Bounds:
    case IrOpcode::kCheckedUint64Bounds:
    case IrOpcode::kCheckBounds:
    case IrOpcode::kChangeInt32ToInt64:
    case IrOpcode::kChangeUint32ToUint64:
      if (node->InputCount() == 0) return false;
      return IsInt32SemanticNode(node->InputAt(0), depth + 1);
    case IrOpcode::kInt32Add:
    case IrOpcode::kInt32Sub:
    case IrOpcode::kInt32Mul:
    case IrOpcode::kCheckedInt32Add:
    case IrOpcode::kCheckedInt32Sub:
    case IrOpcode::kCheckedInt32Mul:
      if (node->op()->ValueInputCount() < 2) return false;
      {
        Node* lhs = NodeProperties::GetValueInput(node, 0);
        Node* rhs = NodeProperties::GetValueInput(node, 1);

        if (lhs->opcode() == IrOpcode::kPhi &&
            IsInt32SemanticNode(rhs, depth + 1)) {
          return true;
        }
        if (rhs->opcode() == IrOpcode::kPhi &&
            IsInt32SemanticNode(lhs, depth + 1)) {
          return true;
        }

        int64_t lhs_literal = 0;
        int64_t rhs_literal = 0;
        bool lhs_is_literal = TryGetInt64Literal(lhs, &lhs_literal);
        bool rhs_is_literal = TryGetInt64Literal(rhs, &rhs_literal);

        if (lhs_is_literal &&
            (IsInt32SemanticNode(rhs, depth + 1) || rhs->opcode() == IrOpcode::kPhi ||
             rhs->opcode() == IrOpcode::kCheckedTaggedSignedToInt32 ||
             rhs->opcode() == IrOpcode::kCheckedTaggedToInt32 ||
             rhs->opcode() == IrOpcode::kChangeTaggedSignedToInt32 ||
             rhs->opcode() == IrOpcode::kChangeTaggedToInt32)) {
          return true;
        }
        if (rhs_is_literal &&
            (IsInt32SemanticNode(lhs, depth + 1) || lhs->opcode() == IrOpcode::kPhi ||
             lhs->opcode() == IrOpcode::kCheckedTaggedSignedToInt32 ||
             lhs->opcode() == IrOpcode::kCheckedTaggedToInt32 ||
             lhs->opcode() == IrOpcode::kChangeTaggedSignedToInt32 ||
             lhs->opcode() == IrOpcode::kChangeTaggedToInt32)) {
          return true;
        }

        return IsInt32SemanticNode(lhs, depth + 1) &&
               IsInt32SemanticNode(rhs, depth + 1);
      }
    case IrOpcode::kPhi: {
      int value_input_count = node->op()->ValueInputCount();
      if (value_input_count <= 0) return false;
      bool seen_int32_like = false;
      for (int i = 0; i < value_input_count; ++i) {
        Node* input = NodeProperties::GetValueInput(node, i);
        int64_t literal_value = 0;
        if (TryGetInt64Literal(input, &literal_value)) {
          continue;
        }
        if (!IsInt32SemanticNode(input, depth + 1)) {
          return false;
        }
        seen_int32_like = true;
      }
      return seen_int32_like;
    }
    default:
      return false;
  }
}

namespace {

[[maybe_unused]] bool IsMachineWord64Like(Node* node) {
  node = UnwrapConstantNode(node);
  if (node == nullptr) return false;
  switch (node->opcode()) {
    case IrOpcode::kInt64Constant:
    case IrOpcode::kInt64Add:
    case IrOpcode::kInt64Sub:
    case IrOpcode::kInt64Mul:
    case IrOpcode::kInt64Div:
    case IrOpcode::kInt64Mod:
    case IrOpcode::kUint64Div:
    case IrOpcode::kUint64Mod:
    case IrOpcode::kChangeInt32ToInt64:
    case IrOpcode::kChangeUint32ToUint64:
      return true;
    default:
      return false;
  }
}

}  // namespace

bool RawInt32StrengthReduction::ReduceCheckedBinop(
    Node* node, const Operator* replacement_op, RawIntKind kind) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  auto left_type = GetNodeTypeAST(left);
  auto right_type = GetNodeTypeAST(right);
  bool left_ok = IsTypeOrLiteralCompatible(left, kind);
  bool right_ok = IsTypeOrLiteralCompatible(right, kind);
  bool left_semantic = false;
  bool right_semantic = false;

  if (kind == RawIntKind::kInt32 && CurrentFunctionHasRawProofAnnotation()) {
    left_semantic = IsInt32SemanticNode(left);
    right_semantic = IsInt32SemanticNode(right);
  }

  RAWINT32_SR_TRACE(
      "check %s#%d start_pos=%d kind=%s left#%d(%s,%s,ok=%d,sem=%d) "
      "right#%d(%s,%s,ok=%d,sem=%d)",
      IrOpcode::Mnemonic(node->opcode()), node->id(), context_.current_start_pos(),
      RawIntKindName(kind), left != nullptr ? left->id() : -1,
      left != nullptr ? IrOpcode::Mnemonic(left->opcode()) : "<null>",
      TypeKindNameOrNone(left_type), left_ok ? 1 : 0, left_semantic ? 1 : 0,
      right != nullptr ? right->id() : -1,
      right != nullptr ? IrOpcode::Mnemonic(right->opcode()) : "<null>",
      TypeKindNameOrNone(right_type), right_ok ? 1 : 0, right_semantic ? 1 : 0);

  if ((!left_ok || !right_ok) && kind == RawIntKind::kInt32 &&
      CurrentFunctionHasRawProofAnnotation() &&
      left_semantic && right_semantic) {
    left_ok = true;
    right_ok = true;
    RAWINT32_SR_DEBUG(
        "fallback-accept %s#%d by int32-semantics in raw-annotated function",
        IrOpcode::Mnemonic(node->opcode()), node->id());
  }

  if (!left_ok || !right_ok) {
    RAWINT32_SR_DEBUG(
        "skip %s#%d kind=%s left#%d(%s,ok=%d) right#%d(%s,ok=%d)",
        IrOpcode::Mnemonic(node->opcode()), node->id(), RawIntKindName(kind),
        left != nullptr ? left->id() : -1,
        left_type.has_value() ? TypeKindName(left_type->kind) : "<none>",
        left_ok ? 1 : 0, right != nullptr ? right->id() : -1,
        right_type.has_value() ? TypeKindName(right_type->kind) : "<none>",
        right_ok ? 1 : 0);
    return false;
  }

  Node* effect_input = NodeProperties::GetEffectInput(node);
  Node* replacement = graph_->NewNode(replacement_op, left, right);
  AnnotateNode(replacement, MakeRawIntAst(kind));

  ZoneVector<std::pair<Node*, int>> value_edges(graph_->zone());
  ZoneVector<std::pair<Node*, int>> effect_edges(graph_->zone());
  for (Edge edge : node->use_edges()) {
    Node* use = edge.from();
    int index = edge.index();
    if (index < NodeProperties::FirstEffectIndex(use)) {
      value_edges.push_back(std::make_pair(use, index));
    } else if (NodeProperties::IsEffectEdge(edge)) {
      effect_edges.push_back(std::make_pair(use, index));
    }
  }

  for (auto& pair : value_edges) {
    pair.first->ReplaceInput(pair.second, replacement);
  }

  for (auto& pair : effect_edges) {
    pair.first->ReplaceInput(pair.second, effect_input);
  }

  RAWINT32_SR_DEBUG("replace %s#%d -> %s (kind=%s)",
                    IrOpcode::Mnemonic(node->opcode()), node->id(),
                    replacement_op->mnemonic(), RawIntKindName(kind));
  RAWINT32_SR_TRACE("replace %s#%d -> %s (kind=%s)",
                    IrOpcode::Mnemonic(node->opcode()), node->id(),
                    replacement_op->mnemonic(), RawIntKindName(kind));

  node->Kill();
  return true;
}

bool RawInt32StrengthReduction::ReduceCheckedInt64DivOrMod(Node* node,
                                                            bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  Node* control_input = NodeProperties::GetControlInput(node);

  auto replace_checked_divmod = [&](const Operator* replacement_op,
                                    RawIntKind kind) {
    Node* effect_input = NodeProperties::GetEffectInput(node);
    Node* replacement = graph_->NewNode(replacement_op, left, right, control_input);
    AnnotateNode(replacement, MakeRawIntAst(kind));

    ZoneVector<std::pair<Node*, int>> value_edges(graph_->zone());
    ZoneVector<std::pair<Node*, int>> effect_edges(graph_->zone());
    for (Edge edge : node->use_edges()) {
      Node* use = edge.from();
      int index = edge.index();
      if (index < NodeProperties::FirstEffectIndex(use)) {
        value_edges.push_back(std::make_pair(use, index));
      } else if (NodeProperties::IsEffectEdge(edge)) {
        effect_edges.push_back(std::make_pair(use, index));
      }
    }

    for (auto& pair : value_edges) {
      pair.first->ReplaceInput(pair.second, replacement);
    }
    for (auto& pair : effect_edges) {
      pair.first->ReplaceInput(pair.second, effect_input);
    }

    RAWINT32_SR_DEBUG("replace %s#%d -> %s (kind=%s)",
                      IrOpcode::Mnemonic(node->opcode()), node->id(),
                      replacement_op->mnemonic(), RawIntKindName(kind));
    RAWINT32_SR_TRACE("replace %s#%d -> %s (kind=%s)",
                      IrOpcode::Mnemonic(node->opcode()), node->id(),
                      replacement_op->mnemonic(), RawIntKindName(kind));

    node->Kill();
    return true;
  };

  if (IsTypeOrLiteralCompatible(left, RawIntKind::kUint64) &&
      IsTypeOrLiteralCompatible(right, RawIntKind::kUint64)) {
    return replace_checked_divmod(
        is_div ? machine_->Uint64Div() : machine_->Uint64Mod(),
        RawIntKind::kUint64);
  }

  if (IsTypeOrLiteralCompatible(left, RawIntKind::kInt64) &&
      IsTypeOrLiteralCompatible(right, RawIntKind::kInt64)) {
    return replace_checked_divmod(
        is_div ? machine_->Int64Div() : machine_->Int64Mod(),
        RawIntKind::kInt64);
  }

  auto left_type = GetNodeTypeAST(left);
  auto right_type = GetNodeTypeAST(right);
  RAWINT32_SR_DEBUG(
      "skip %s#%d (int64/uint64 mismatch) left#%d(%s) right#%d(%s)",
      IrOpcode::Mnemonic(node->opcode()), node->id(),
      left != nullptr ? left->id() : -1,
      left_type.has_value() ? TypeKindName(left_type->kind) : "<none>",
      right != nullptr ? right->id() : -1,
      right_type.has_value() ? TypeKindName(right_type->kind) : "<none>");

  return false;
}

bool RawInt32StrengthReduction::ReduceCheckedTaggedSignedToInt32(Node* node) {
  Node* value = NodeProperties::GetValueInput(node, 0);
  Type input_type = NodeProperties::GetType(value);
  bool proven_smi = input_type.Is(Type::SignedSmall()) ||
                    value->opcode() == IrOpcode::kCheckSmi;
  if (!IsRawTyped(value, RawIntKind::kInt32) || !proven_smi) {
    return false;
  }

  Node* replacement = graph_->NewNode(simplified_->ChangeTaggedSignedToInt32(),
                                      value);
  AnnotateNode(replacement, MakeRawIntAst(RawIntKind::kInt32));

  Node* effect_input = NodeProperties::GetEffectInput(node);
  ZoneVector<std::pair<Node*, int>> value_edges(graph_->zone());
  ZoneVector<std::pair<Node*, int>> effect_edges(graph_->zone());
  for (Edge edge : node->use_edges()) {
    Node* use = edge.from();
    int index = edge.index();
    if (index < NodeProperties::FirstEffectIndex(use)) {
      value_edges.push_back(std::make_pair(use, index));
    } else if (NodeProperties::IsEffectEdge(edge)) {
      effect_edges.push_back(std::make_pair(use, index));
    }
  }

  for (auto& pair : value_edges) {
    pair.first->ReplaceInput(pair.second, replacement);
  }
  for (auto& pair : effect_edges) {
    pair.first->ReplaceInput(pair.second, effect_input);
  }

  RAWINT32_SR_TRACE("replace %s#%d -> %s (kind=%s)",
                    IrOpcode::Mnemonic(node->opcode()), node->id(),
                    simplified_->ChangeTaggedSignedToInt32()->mnemonic(),
                    RawIntKindName(RawIntKind::kInt32));

  node->Kill();
  return true;
}

void ReplaceCheckedWithValue(TFGraph* graph, Node* node, Node* replacement) {
  Node* effect_input = NodeProperties::GetEffectInput(node);

  ZoneVector<std::pair<Node*, int>> value_edges(graph->zone());
  ZoneVector<std::pair<Node*, int>> effect_edges(graph->zone());
  for (Edge edge : node->use_edges()) {
    Node* use = edge.from();
    int index = edge.index();
    if (index < NodeProperties::FirstEffectIndex(use)) {
      value_edges.push_back(std::make_pair(use, index));
    } else if (NodeProperties::IsEffectEdge(edge)) {
      effect_edges.push_back(std::make_pair(use, index));
    }
  }

  for (auto& pair : value_edges) {
    pair.first->ReplaceInput(pair.second, replacement);
  }
  for (auto& pair : effect_edges) {
    pair.first->ReplaceInput(pair.second, effect_input);
  }
  node->Kill();
}

bool RawInt32StrengthReduction::ReduceCheckedInt32DivOrModClosed(Node* node,
                                                                  bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  Node* control = NodeProperties::GetControlInput(node);
  bool left_ok = IsTypeOrLiteralCompatible(left, RawIntKind::kInt32);
  bool right_ok = IsTypeOrLiteralCompatible(right, RawIntKind::kInt32);
  if (!left_ok || !right_ok) {
    auto left_type = GetNodeTypeAST(left);
    auto right_type = GetNodeTypeAST(right);
    RAWINT32_SR_DEBUG(
        "skip %s#%d kind=rawint32 left#%d(%s,ok=%d) right#%d(%s,ok=%d)",
        IrOpcode::Mnemonic(node->opcode()), node->id(),
        left != nullptr ? left->id() : -1,
        left_type.has_value() ? TypeKindName(left_type->kind) : "<none>",
        left_ok ? 1 : 0, right != nullptr ? right->id() : -1,
        right_type.has_value() ? TypeKindName(right_type->kind) : "<none>",
        right_ok ? 1 : 0);
    return false;
  }
  Node* replacement =
      is_div ? BuildInt32VarDivClosed(graph_, machine_, common_, left, right,
                                      control)
             : BuildInt32VarModClosed(graph_, machine_, common_, left, right,
                                      control);
  AnnotateNode(replacement, MakeRawIntAst(RawIntKind::kInt32));
  ReplaceCheckedWithValue(graph_, node, replacement);
  RAWINT32_SR_DEBUG("replace %s#%d -> %s (closed semantics)",
                    IrOpcode::Mnemonic(node->opcode()), node->id(),
                    is_div ? "Int32DivClosed" : "Int32ModClosed");
  return true;
}

bool RawInt32StrengthReduction::ReduceCheckedUint32DivOrModClosed(Node* node,
                                                                   bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  Node* control = NodeProperties::GetControlInput(node);
  bool left_ok = IsTypeOrLiteralCompatible(left, RawIntKind::kUint32);
  bool right_ok = IsTypeOrLiteralCompatible(right, RawIntKind::kUint32);
  if (!left_ok || !right_ok) {
    auto left_type = GetNodeTypeAST(left);
    auto right_type = GetNodeTypeAST(right);
    RAWINT32_SR_DEBUG(
        "skip %s#%d kind=rawuint32 left#%d(%s,ok=%d) right#%d(%s,ok=%d)",
        IrOpcode::Mnemonic(node->opcode()), node->id(),
        left != nullptr ? left->id() : -1,
        left_type.has_value() ? TypeKindName(left_type->kind) : "<none>",
        left_ok ? 1 : 0, right != nullptr ? right->id() : -1,
        right_type.has_value() ? TypeKindName(right_type->kind) : "<none>",
        right_ok ? 1 : 0);
    return false;
  }
  Node* replacement =
      is_div ? BuildUint32VarDivClosed(graph_, machine_, common_, left, right,
                                       control)
             : BuildUint32VarModClosed(graph_, machine_, common_, left, right,
                                       control);
  AnnotateNode(replacement, MakeRawIntAst(RawIntKind::kUint32));
  ReplaceCheckedWithValue(graph_, node, replacement);
  RAWINT32_SR_DEBUG("replace %s#%d -> %s (closed semantics)",
                    IrOpcode::Mnemonic(node->opcode()), node->id(),
                    is_div ? "Uint32DivClosed" : "Uint32ModClosed");
  return true;
}

void RawInt32StrengthReduction::MaybeChangeCheckedDivModOp(
    Node* node, const Operator* replacement_op, RawIntKind kind) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  if (!IsTypeOrLiteralCompatible(left, kind) ||
      !IsTypeOrLiteralCompatible(right, kind)) {
    return;
  }
  NodeProperties::ChangeOp(node, replacement_op);
}

bool RawInt32StrengthReduction::ReduceRawFloat64DivOrMod(Node* node,
                                                         bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);

  RAWINT32_SR_TRACE(
      "f64-%s-check node#%d left#%d(%s) right#%d(%s) start_pos=%d raw_proof=%d",
      is_div ? "div" : "mod", node->id(), left != nullptr ? left->id() : -1,
      left != nullptr ? IrOpcode::Mnemonic(left->opcode()) : "<null>",
      right != nullptr ? right->id() : -1,
      right != nullptr ? IrOpcode::Mnemonic(right->opcode()) : "<null>",
      context_.current_start_pos(), CurrentFunctionHasRawProofAnnotation() ? 1 : 0);

  Node* left_unwrapped = UnwrapConstantNode(left);
  Node* right_unwrapped = UnwrapConstantNode(right);
  RAWINT32_SR_TRACE(
      "f64-%s-unwrapped node#%d left_unwrap#%d(%s) right_unwrap#%d(%s)",
      is_div ? "div" : "mod", node->id(),
      left_unwrapped != nullptr ? left_unwrapped->id() : -1,
      left_unwrapped != nullptr ? IrOpcode::Mnemonic(left_unwrapped->opcode())
                                : "<null>",
      right_unwrapped != nullptr ? right_unwrapped->id() : -1,
      right_unwrapped != nullptr ? IrOpcode::Mnemonic(right_unwrapped->opcode())
                                 : "<null>");
  if (left_unwrapped == nullptr || right_unwrapped == nullptr) {
    RAWINT32_SR_TRACE("f64-%s-skip node#%d reason=unwrap-null",
                      is_div ? "div" : "mod", node->id());
    return false;
  }

  int64_t rhs = 0;
  bool rhs_is_integral_const = TryGetFloat64IntegralConstant(right, &rhs);
  if (!rhs_is_integral_const) {
    RAWINT32_SR_TRACE("f64-%s-rhs-not-integral node#%d right#%d(%s)",
                      is_div ? "div" : "mod", node->id(),
                      right != nullptr ? right->id() : -1,
                      right != nullptr ? IrOpcode::Mnemonic(right->opcode())
                                       : "<null>");
  } else {
    RAWINT32_SR_TRACE("f64-%s-rhs-const node#%d rhs=%lld",
                      is_div ? "div" : "mod", node->id(),
                      static_cast<long long>(rhs));
  }

  if (rhs_is_integral_const && IsRawTyped(left_unwrapped, RawIntKind::kInt32) &&
      rhs >= std::numeric_limits<int32_t>::min() &&
      rhs <= std::numeric_limits<int32_t>::max()) {
    int32_t rhs_i32_value = static_cast<int32_t>(rhs);
    if (rhs_i32_value == 0) {
      RAWINT32_SR_TRACE("f64-%s-skip node#%d reason=i32-div-by-zero-const",
                        is_div ? "div" : "mod", node->id());
      return false;
    }
    if (is_div && rhs_i32_value == std::numeric_limits<int32_t>::min()) {
      RAWINT32_SR_TRACE("f64-%s-skip node#%d reason=i32-min-overflow-const",
                        is_div ? "div" : "mod", node->id());
      return false;
    }

    Node* lhs_i32 = graph_->NewNode(machine_->ChangeFloat64ToInt32(), left);
    Node* result_i32 = nullptr;
    if (is_div) {
      result_i32 = BuildInt32DivByConst(graph_, machine_, common_, lhs_i32,
                                        rhs_i32_value);
    } else {
      Node* quotient = BuildInt32DivByConst(graph_, machine_, common_, lhs_i32,
                                            rhs_i32_value);
      Node* rhs_i32 = graph_->NewNode(common_->Int32Constant(rhs_i32_value));
      result_i32 = Int32Sub(graph_, machine_, lhs_i32,
                            Int32Mul(graph_, machine_, quotient, rhs_i32));
    }

    Node* replacement = graph_->NewNode(machine_->ChangeInt32ToFloat64(), result_i32);

    ReplaceValueUsesAndKill(graph_, node, replacement);
    RAWINT32_SR_TRACE("f64-%s-replace node#%d path=i32-const", is_div ? "div" : "mod",
                      node->id());
    return true;
  }

  if (rhs_is_integral_const && IsRawTyped(left_unwrapped, RawIntKind::kUint32) && rhs >= 0 &&
      static_cast<uint64_t>(rhs) <= std::numeric_limits<uint32_t>::max()) {
    uint32_t rhs_u32_value = static_cast<uint32_t>(rhs);
    if (rhs_u32_value == 0) {
      RAWINT32_SR_TRACE("f64-%s-skip node#%d reason=u32-div-by-zero-const",
                        is_div ? "div" : "mod", node->id());
      return false;
    }

    Node* lhs_u32 = graph_->NewNode(machine_->ChangeFloat64ToUint32(), left);
    Node* result_u32 = nullptr;
    if (is_div) {
      result_u32 = BuildUint32DivByConst(graph_, machine_, common_, lhs_u32,
                                         rhs_u32_value);
    } else {
      Node* quotient = BuildUint32DivByConst(graph_, machine_, common_, lhs_u32,
                                             rhs_u32_value);
      Node* rhs_u32 =
          graph_->NewNode(common_->Int32Constant(static_cast<int32_t>(rhs_u32_value)));
      result_u32 = Int32Sub(graph_, machine_, lhs_u32,
                            Int32Mul(graph_, machine_, quotient, rhs_u32));
    }

    Node* replacement = graph_->NewNode(machine_->ChangeUint32ToFloat64(), result_u32);

    ReplaceValueUsesAndKill(graph_, node, replacement);
    RAWINT32_SR_TRACE("f64-%s-replace node#%d path=u32-const", is_div ? "div" : "mod",
                      node->id());
    return true;
  }

    bool left_is_raw_i32 = IsRawTyped(left_unwrapped, RawIntKind::kInt32);
    bool right_is_raw_i32 = IsRawTyped(right_unwrapped, RawIntKind::kInt32);
    bool left_is_raw_u32 = IsRawTyped(left_unwrapped, RawIntKind::kUint32);
    bool right_is_raw_u32 = IsRawTyped(right_unwrapped, RawIntKind::kUint32);
    if (is_div && left_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      right_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      left_is_raw_i32 && right_is_raw_i32) {
    Node* control_input = nullptr;
    if (right_unwrapped->op()->ControlInputCount() > 0) {
      control_input = NodeProperties::GetControlInput(right_unwrapped);
    } else if (left_unwrapped->op()->ControlInputCount() > 0) {
      control_input = NodeProperties::GetControlInput(left_unwrapped);
    }
    if (control_input == nullptr) {
      control_input = graph_->start();
    }

    RAWINT32_SR_TRACE(
        "f64-div-var-int32 node#%d control#%d(%s) left_ctrl_inputs=%d right_ctrl_inputs=%d",
        node->id(), control_input != nullptr ? control_input->id() : -1,
        control_input != nullptr ? IrOpcode::Mnemonic(control_input->opcode())
                                 : "<null>",
        left_unwrapped->op()->ControlInputCount(),
        right_unwrapped->op()->ControlInputCount());

    Node* lhs_i32 = graph_->NewNode(machine_->ChangeFloat64ToInt32(), left);
    Node* rhs_i32 = graph_->NewNode(machine_->ChangeFloat64ToInt32(), right);
    Node* result_i32 = BuildInt32VarDivClosed(graph_, machine_, common_,
                                              lhs_i32, rhs_i32, control_input);
    Node* replacement =
        graph_->NewNode(machine_->ChangeInt32ToFloat64(), result_i32);

    ReplaceValueUsesAndKill(graph_, node, replacement);
    RAWINT32_SR_TRACE("f64-div-replace node#%d path=i32-var-closed", node->id());
    return true;
  }

  if (is_div && left_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      right_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      (!left_is_raw_i32 || !right_is_raw_i32)) {
    RAWINT32_SR_TRACE(
        "f64-div-skip node#%d reason=var-i32-not-raw left_raw=%d right_raw=%d",
        node->id(), left_is_raw_i32 ? 1 : 0, right_is_raw_i32 ? 1 : 0);
  }

  if (!is_div && left_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      right_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      left_is_raw_i32 && right_is_raw_i32) {
    Node* control_input = nullptr;
    if (right_unwrapped->op()->ControlInputCount() > 0) {
      control_input = NodeProperties::GetControlInput(right_unwrapped);
    } else if (left_unwrapped->op()->ControlInputCount() > 0) {
      control_input = NodeProperties::GetControlInput(left_unwrapped);
    }
    if (control_input == nullptr) {
      control_input = graph_->start();
    }

    RAWINT32_SR_TRACE(
        "f64-mod-var-int32 node#%d control#%d(%s) left_ctrl_inputs=%d right_ctrl_inputs=%d",
        node->id(), control_input != nullptr ? control_input->id() : -1,
        control_input != nullptr ? IrOpcode::Mnemonic(control_input->opcode())
                                 : "<null>",
        left_unwrapped->op()->ControlInputCount(),
        right_unwrapped->op()->ControlInputCount());

    Node* lhs_i32 = graph_->NewNode(machine_->ChangeFloat64ToInt32(), left);
    Node* rhs_i32 = graph_->NewNode(machine_->ChangeFloat64ToInt32(), right);
    Node* result_i32 = BuildInt32VarModClosed(graph_, machine_, common_,
                                              lhs_i32, rhs_i32, control_input);
    Node* replacement =
        graph_->NewNode(machine_->ChangeInt32ToFloat64(), result_i32);

    ReplaceValueUsesAndKill(graph_, node, replacement);
    RAWINT32_SR_TRACE("f64-mod-replace node#%d path=i32-var-closed", node->id());
    return true;
  }

  if (!is_div && left_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      right_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      left_is_raw_u32 && right_is_raw_u32) {
    Node* control_input = nullptr;
    if (right_unwrapped->op()->ControlInputCount() > 0) {
      control_input = NodeProperties::GetControlInput(right_unwrapped);
    } else if (left_unwrapped->op()->ControlInputCount() > 0) {
      control_input = NodeProperties::GetControlInput(left_unwrapped);
    }
    if (control_input == nullptr) {
      control_input = graph_->start();
    }

    RAWINT32_SR_TRACE(
        "f64-mod-var-uint32 node#%d control#%d(%s) left_ctrl_inputs=%d right_ctrl_inputs=%d",
        node->id(), control_input != nullptr ? control_input->id() : -1,
        control_input != nullptr ? IrOpcode::Mnemonic(control_input->opcode())
                                 : "<null>",
        left_unwrapped->op()->ControlInputCount(),
        right_unwrapped->op()->ControlInputCount());

    Node* lhs_u32 = graph_->NewNode(machine_->ChangeFloat64ToUint32(), left);
    Node* rhs_u32 = graph_->NewNode(machine_->ChangeFloat64ToUint32(), right);
    Node* result_u32 = BuildUint32VarModClosed(graph_, machine_, common_,
                                               lhs_u32, rhs_u32, control_input);
    Node* replacement =
        graph_->NewNode(machine_->ChangeUint32ToFloat64(), result_u32);

    ReplaceValueUsesAndKill(graph_, node, replacement);
    RAWINT32_SR_TRACE("f64-mod-replace node#%d path=u32-var-closed", node->id());
    return true;
  }

  if (!is_div && left_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      right_unwrapped->opcode() == IrOpcode::kCheckedTaggedToFloat64 &&
      (!left_is_raw_i32 || !right_is_raw_i32) &&
      (!left_is_raw_u32 || !right_is_raw_u32)) {
    RAWINT32_SR_TRACE(
        "f64-mod-skip node#%d reason=var-mod-not-raw left_i32=%d right_i32=%d left_u32=%d right_u32=%d",
        node->id(), left_is_raw_i32 ? 1 : 0, right_is_raw_i32 ? 1 : 0,
        left_is_raw_u32 ? 1 : 0, right_is_raw_u32 ? 1 : 0);
  }

  RAWINT32_SR_TRACE("f64-%s-no-match node#%d", is_div ? "div" : "mod", node->id());
  return false;
}

void RawInt32StrengthReduction::Run() {
  EnsureMetadataLoaded();
  RAWINT32_SR_TRACE("run start_pos=%d raw_proof=%d", context_.current_start_pos(),
                    CurrentFunctionHasRawProofAnnotation() ? 1 : 0);

  // Use multiple rounds because newly reduced machine nodes can unlock
  // additional checked nodes in the same graph.
  constexpr int kMaxRounds = 4;
  for (int round = 0; round < kMaxRounds; ++round) {
    bool changed = false;
    [[maybe_unused]] int checked_i32_add_seen = 0;
    [[maybe_unused]] int checked_i32_add_replaced = 0;
    [[maybe_unused]] int checked_i32_sub_seen = 0;
    [[maybe_unused]] int checked_i32_sub_replaced = 0;
    [[maybe_unused]] int checked_i32_mul_seen = 0;
    [[maybe_unused]] int checked_i32_mul_replaced = 0;
    AllNodes all(graph_->zone(), graph_);

    for (Node* node : all.reachable) {
      switch (node->opcode()) {
        case IrOpcode::kCheckedTaggedSignedToInt32:
          changed |= ReduceCheckedTaggedSignedToInt32(node);
          break;
        case IrOpcode::kCheckedInt32Add:
          checked_i32_add_seen++;
          if (ReduceCheckedBinop(node, machine_->Int32Add(),
                                 RawIntKind::kInt32)) {
            checked_i32_add_replaced++;
            changed = true;
          }
          break;
        case IrOpcode::kCheckedInt32Sub:
          checked_i32_sub_seen++;
          if (ReduceCheckedBinop(node, machine_->Int32Sub(),
                                 RawIntKind::kInt32)) {
            checked_i32_sub_replaced++;
            changed = true;
          }
          break;
        case IrOpcode::kCheckedInt32Mul:
          checked_i32_mul_seen++;
          if (ReduceCheckedBinop(node, machine_->Int32Mul(),
                                 RawIntKind::kInt32)) {
            checked_i32_mul_replaced++;
            changed = true;
          }
          break;
        case IrOpcode::kCheckedInt32Div:
          changed |= ReduceCheckedInt32DivOrModClosed(node, true);
          break;
        case IrOpcode::kCheckedInt32Mod:
          changed |= ReduceCheckedInt32DivOrModClosed(node, false);
          break;
        case IrOpcode::kCheckedUint32Div:
          changed |= ReduceCheckedUint32DivOrModClosed(node, true);
          break;
        case IrOpcode::kCheckedUint32Mod:
          changed |= ReduceCheckedUint32DivOrModClosed(node, false);
          break;
        case IrOpcode::kCheckedInt64Add:
          if (IsTypeOrLiteralCompatible(node->InputAt(0),
                                        RawIntKind::kInt64) &&
              IsTypeOrLiteralCompatible(node->InputAt(1),
                                        RawIntKind::kInt64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Add(),
                                          RawIntKind::kInt64);
          } else if (IsTypeOrLiteralCompatible(node->InputAt(0),
                                               RawIntKind::kUint64) &&
                     IsTypeOrLiteralCompatible(node->InputAt(1),
                                               RawIntKind::kUint64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Add(),
                                          RawIntKind::kUint64);
          }
          break;
        case IrOpcode::kCheckedInt64Sub:
          if (IsTypeOrLiteralCompatible(node->InputAt(0),
                                        RawIntKind::kInt64) &&
              IsTypeOrLiteralCompatible(node->InputAt(1),
                                        RawIntKind::kInt64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Sub(),
                                          RawIntKind::kInt64);
          } else if (IsTypeOrLiteralCompatible(node->InputAt(0),
                                               RawIntKind::kUint64) &&
                     IsTypeOrLiteralCompatible(node->InputAt(1),
                                               RawIntKind::kUint64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Sub(),
                                          RawIntKind::kUint64);
          }
          break;
        case IrOpcode::kCheckedInt64Mul:
          if (IsTypeOrLiteralCompatible(node->InputAt(0),
                                        RawIntKind::kInt64) &&
              IsTypeOrLiteralCompatible(node->InputAt(1),
                                        RawIntKind::kInt64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Mul(),
                                          RawIntKind::kInt64);
          } else if (IsTypeOrLiteralCompatible(node->InputAt(0),
                                               RawIntKind::kUint64) &&
                     IsTypeOrLiteralCompatible(node->InputAt(1),
                                               RawIntKind::kUint64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Mul(),
                                          RawIntKind::kUint64);
          }
          break;
        case IrOpcode::kCheckedInt64Div:
          changed |= ReduceCheckedInt64DivOrMod(node, true);
          break;
        case IrOpcode::kCheckedInt64Mod:
          changed |= ReduceCheckedInt64DivOrMod(node, false);
          break;
        default:
          break;
      }
    }

    for (Node* node : all.reachable) {
      switch (node->opcode()) {
        case IrOpcode::kFloat64Div:
          RAWINT32_SR_TRACE("run round=%d visit Float64Div node#%d start_pos=%d",
                            round, node->id(), context_.current_start_pos());
          changed |= ReduceRawFloat64DivOrMod(node, true);
          break;
        case IrOpcode::kFloat64Mod:
          RAWINT32_SR_TRACE("run round=%d visit Float64Mod node#%d start_pos=%d",
                            round, node->id(), context_.current_start_pos());
          changed |= ReduceRawFloat64DivOrMod(node, false);
          break;
        default:
          break;
      }
    }

    RAWINT32_SR_DEBUG(
        "round=%d checked-int32 add %d/%d sub %d/%d mul %d/%d changed=%d",
        round, checked_i32_add_replaced, checked_i32_add_seen,
        checked_i32_sub_replaced, checked_i32_sub_seen,
        checked_i32_mul_replaced, checked_i32_mul_seen, changed ? 1 : 0);

    if (!changed) break;
  }
}

}  // namespace v8::internal::compiler
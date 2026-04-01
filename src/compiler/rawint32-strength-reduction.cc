#include "src/compiler/rawint32-strength-reduction.h"

#include "src/compiler/all-nodes.h"
#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"

#include "src/base/logging.h"
#include "src/base/division-by-constant.h"

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
      case IrOpcode::kChangeFloat64ToInt32:
      case IrOpcode::kChangeFloat64ToUint32:
      case IrOpcode::kChangeInt32ToFloat64:
      case IrOpcode::kChangeUint32ToFloat64:
      case IrOpcode::kChangeFloat64ToTagged:
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
  auto& param_types = context_.param_types();
  for (const auto& ast : param_types) {
    if (ast.kind == TypeAST::RawInt32 || ast.kind == TypeAST::RawUint32 ||
        ast.kind == TypeAST::RawInt64 || ast.kind == TypeAST::RawUint64) {
      return true;
    }
  }

  auto return_type_opt = GetFunctionReturnType(context_.current_start_pos());
  if (!return_type_opt.has_value()) return false;
  TypeAST::TypeKind kind = return_type_opt->kind;
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
        int64_t lhs_literal = 0;
        int64_t rhs_literal = 0;
        bool lhs_is_literal = TryGetInt64Literal(lhs, &lhs_literal);
        bool rhs_is_literal = TryGetInt64Literal(rhs, &rhs_literal);

        if (lhs_is_literal &&
            (IsInt32SemanticNode(rhs, depth + 1) || rhs->opcode() == IrOpcode::kPhi ||
             rhs->opcode() == IrOpcode::kCheckedTaggedSignedToInt32 ||
             rhs->opcode() == IrOpcode::kCheckedTaggedToInt32)) {
          return true;
        }
        if (rhs_is_literal &&
            (IsInt32SemanticNode(lhs, depth + 1) || lhs->opcode() == IrOpcode::kPhi ||
             lhs->opcode() == IrOpcode::kCheckedTaggedSignedToInt32 ||
             lhs->opcode() == IrOpcode::kCheckedTaggedToInt32)) {
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

bool IsMachineWord64Like(Node* node) {
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
  bool left_ok = IsTypeOrLiteralCompatible(left, kind);
  bool right_ok = IsTypeOrLiteralCompatible(right, kind);

  if ((!left_ok || !right_ok) && kind == RawIntKind::kInt32 &&
      CurrentFunctionHasRawProofAnnotation() &&
      IsInt32SemanticNode(left) && IsInt32SemanticNode(right)) {
    left_ok = true;
    right_ok = true;
    RAWINT32_SR_DEBUG(
        "fallback-accept %s#%d by int32-semantics in raw-annotated function",
        IrOpcode::Mnemonic(node->opcode()), node->id());
  }

  if (!left_ok || !right_ok) {
    auto left_type = GetNodeTypeAST(left);
    auto right_type = GetNodeTypeAST(right);
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

  node->Kill();
  return true;
}

bool RawInt32StrengthReduction::ReduceCheckedInt64DivOrMod(Node* node,
                                                            bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);

  if (IsTypeOrLiteralCompatible(left, RawIntKind::kUint64) &&
      IsTypeOrLiteralCompatible(right, RawIntKind::kUint64)) {
    NodeProperties::ChangeOp(node,
                             is_div ? machine_->Uint64Div()
                                    : machine_->Uint64Mod());
    AnnotateNode(node, MakeRawIntAst(RawIntKind::kUint64));
    RAWINT32_SR_DEBUG("replace %s#%d -> %s (kind=rawuint64)",
                      IrOpcode::Mnemonic(node->opcode()), node->id(),
                      is_div ? "Uint64Div" : "Uint64Mod");
    return true;
  }

  if (IsTypeOrLiteralCompatible(left, RawIntKind::kInt64) &&
      IsTypeOrLiteralCompatible(right, RawIntKind::kInt64)) {
    NodeProperties::ChangeOp(node,
                             is_div ? machine_->Int64Div()
                                    : machine_->Int64Mod());
    AnnotateNode(node, MakeRawIntAst(RawIntKind::kInt64));
    RAWINT32_SR_DEBUG("replace %s#%d -> %s (kind=rawint64)",
                      IrOpcode::Mnemonic(node->opcode()), node->id(),
                      is_div ? "Int64Div" : "Int64Mod");
    return true;
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

  Node* left_unwrapped = UnwrapConstantNode(left);
  Node* right_unwrapped = UnwrapConstantNode(right);
  if (left_unwrapped == nullptr || right_unwrapped == nullptr) return false;

  int64_t rhs = 0;
  if (!TryGetFloat64IntegralConstant(right, &rhs)) return false;

  if (IsRawTyped(left_unwrapped, RawIntKind::kInt32) &&
      rhs >= std::numeric_limits<int32_t>::min() &&
      rhs <= std::numeric_limits<int32_t>::max()) {
    int32_t rhs_i32_value = static_cast<int32_t>(rhs);
    if (rhs_i32_value == 0) {
      return false;
    }
    if (is_div && rhs_i32_value == std::numeric_limits<int32_t>::min()) {
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
    return true;
  }

  if (IsRawTyped(left_unwrapped, RawIntKind::kUint32) && rhs >= 0 &&
      static_cast<uint64_t>(rhs) <= std::numeric_limits<uint32_t>::max()) {
    uint32_t rhs_u32_value = static_cast<uint32_t>(rhs);
    if (rhs_u32_value == 0) {
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
    return true;
  }

  return false;
}

void RawInt32StrengthReduction::Run() {
  EnsureMetadataLoaded();

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
          if (IsMachineWord64Like(node->InputAt(0)) &&
              IsMachineWord64Like(node->InputAt(1)) &&
              IsTypeOrLiteralCompatible(node->InputAt(0),
                                        RawIntKind::kInt64) &&
              IsTypeOrLiteralCompatible(node->InputAt(1),
                                        RawIntKind::kInt64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Add(),
                                          RawIntKind::kInt64);
          } else if (IsMachineWord64Like(node->InputAt(0)) &&
                     IsMachineWord64Like(node->InputAt(1)) &&
                     IsTypeOrLiteralCompatible(node->InputAt(0),
                                               RawIntKind::kUint64) &&
                     IsTypeOrLiteralCompatible(node->InputAt(1),
                                               RawIntKind::kUint64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Add(),
                                          RawIntKind::kUint64);
          }
          break;
        case IrOpcode::kCheckedInt64Sub:
          if (IsMachineWord64Like(node->InputAt(0)) &&
              IsMachineWord64Like(node->InputAt(1)) &&
              IsTypeOrLiteralCompatible(node->InputAt(0),
                                        RawIntKind::kInt64) &&
              IsTypeOrLiteralCompatible(node->InputAt(1),
                                        RawIntKind::kInt64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Sub(),
                                          RawIntKind::kInt64);
          } else if (IsMachineWord64Like(node->InputAt(0)) &&
                     IsMachineWord64Like(node->InputAt(1)) &&
                     IsTypeOrLiteralCompatible(node->InputAt(0),
                                               RawIntKind::kUint64) &&
                     IsTypeOrLiteralCompatible(node->InputAt(1),
                                               RawIntKind::kUint64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Sub(),
                                          RawIntKind::kUint64);
          }
          break;
        case IrOpcode::kCheckedInt64Mul:
          if (IsMachineWord64Like(node->InputAt(0)) &&
              IsMachineWord64Like(node->InputAt(1)) &&
              IsTypeOrLiteralCompatible(node->InputAt(0),
                                        RawIntKind::kInt64) &&
              IsTypeOrLiteralCompatible(node->InputAt(1),
                                        RawIntKind::kInt64)) {
            changed |= ReduceCheckedBinop(node, machine_->Int64Mul(),
                                          RawIntKind::kInt64);
          } else if (IsMachineWord64Like(node->InputAt(0)) &&
                     IsMachineWord64Like(node->InputAt(1)) &&
                     IsTypeOrLiteralCompatible(node->InputAt(0),
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
          changed |= ReduceRawFloat64DivOrMod(node, true);
          break;
        case IrOpcode::kFloat64Mod:
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
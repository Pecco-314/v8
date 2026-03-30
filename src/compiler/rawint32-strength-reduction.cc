#include "src/compiler/rawint32-strength-reduction.h"

#include "src/compiler/all-nodes.h"
#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"

#include "src/base/division-by-constant.h"

#include <cmath>
#include <limits>

namespace v8::internal::compiler {

namespace {

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
                 Node* rhs_i32) {
    Node* const zero_i32 = graph->NewNode(common->Int32Constant(0));
    Node* const minus_one_i32 = graph->NewNode(common->Int32Constant(-1));

    const Operator* const merge2 = common->Merge(2);
    const Operator* const phi_word32 =
      common->Phi(MachineRepresentation::kWord32, 2);

    Node* check_zero = graph->NewNode(machine->Word32Equal(), rhs_i32, zero_i32);
    Node* branch_zero = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_zero, graph->start());
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
                  Node* rhs_u32) {
    Node* const zero_i32 = graph->NewNode(common->Int32Constant(0));
    const Operator* const merge2 = common->Merge(2);
    const Operator* const phi_word32 =
      common->Phi(MachineRepresentation::kWord32, 2);

    Node* check_zero = graph->NewNode(machine->Word32Equal(), rhs_u32, zero_i32);
    Node* branch_zero = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_zero, graph->start());
    Node* if_zero = graph->NewNode(common->IfTrue(), branch_zero);
    Node* if_nonzero = graph->NewNode(common->IfFalse(), branch_zero);
    Node* div_u32 =
      graph->NewNode(machine->Uint32Div(), lhs_u32, rhs_u32, if_nonzero);

    Node* merge = graph->NewNode(merge2, if_zero, if_nonzero);
    return graph->NewNode(phi_word32, zero_i32, div_u32, merge);
  }

  Node* BuildInt32VarModClosed(TFGraph* graph, MachineOperatorBuilder* machine,
                 CommonOperatorBuilder* common, Node* lhs_i32,
                 Node* rhs_i32) {
    Node* const zero_i32 = graph->NewNode(common->Int32Constant(0));
    const Operator* const merge2 = common->Merge(2);
    const Operator* const phi_word32 =
      common->Phi(MachineRepresentation::kWord32, 2);

    Node* check_zero = graph->NewNode(machine->Word32Equal(), rhs_i32, zero_i32);
    Node* branch_zero = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_zero, graph->start());
    Node* if_zero = graph->NewNode(common->IfTrue(), branch_zero);
    Node* if_nonzero = graph->NewNode(common->IfFalse(), branch_zero);
    Node* mod_nonzero =
      graph->NewNode(machine->Int32Mod(), lhs_i32, rhs_i32, if_nonzero);

    Node* merge = graph->NewNode(merge2, if_zero, if_nonzero);
    return graph->NewNode(phi_word32, zero_i32, mod_nonzero, merge);
  }

  Node* BuildUint32VarModClosed(TFGraph* graph, MachineOperatorBuilder* machine,
                  CommonOperatorBuilder* common, Node* lhs_u32,
                  Node* rhs_u32) {
    Node* const zero_i32 = graph->NewNode(common->Int32Constant(0));
    const Operator* const merge2 = common->Merge(2);
    const Operator* const phi_word32 =
      common->Phi(MachineRepresentation::kWord32, 2);

    Node* check_zero = graph->NewNode(machine->Word32Equal(), rhs_u32, zero_i32);
    Node* branch_zero = graph->NewNode(
      common->Branch(BranchHint::kFalse, BranchSemantics::kMachine),
      check_zero, graph->start());
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

void RawInt32StrengthReduction::ReduceCheckedBinop(
    Node* node, const Operator* replacement_op, RawIntKind kind) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  if (!IsTypeOrLiteralCompatible(left, kind) ||
      !IsTypeOrLiteralCompatible(right, kind)) {
    return;
  }

  Node* effect_input = NodeProperties::GetEffectInput(node);
  Node* replacement = graph_->NewNode(replacement_op, left, right);

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

  node->Kill();
}

void RawInt32StrengthReduction::ReduceCheckedInt64DivOrMod(Node* node,
                                                            bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);

  if (IsTypeOrLiteralCompatible(left, RawIntKind::kUint64) &&
      IsTypeOrLiteralCompatible(right, RawIntKind::kUint64)) {
    NodeProperties::ChangeOp(node,
                             is_div ? machine_->Uint64Div()
                                    : machine_->Uint64Mod());
    return;
  }

  if (IsTypeOrLiteralCompatible(left, RawIntKind::kInt64) &&
      IsTypeOrLiteralCompatible(right, RawIntKind::kInt64)) {
    NodeProperties::ChangeOp(node,
                             is_div ? machine_->Int64Div()
                                    : machine_->Int64Mod());
  }
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

void RawInt32StrengthReduction::ReduceCheckedInt32DivOrModClosed(Node* node,
                                                                  bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  if (!IsTypeOrLiteralCompatible(left, RawIntKind::kInt32) ||
      !IsTypeOrLiteralCompatible(right, RawIntKind::kInt32)) {
    return;
  }
  Node* replacement =
      is_div ? BuildInt32VarDivClosed(graph_, machine_, common_, left, right)
             : BuildInt32VarModClosed(graph_, machine_, common_, left, right);
  ReplaceCheckedWithValue(graph_, node, replacement);
}

void RawInt32StrengthReduction::ReduceCheckedUint32DivOrModClosed(Node* node,
                                                                   bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);
  if (!IsTypeOrLiteralCompatible(left, RawIntKind::kUint32) ||
      !IsTypeOrLiteralCompatible(right, RawIntKind::kUint32)) {
    return;
  }
  Node* replacement =
      is_div ? BuildUint32VarDivClosed(graph_, machine_, common_, left, right)
             : BuildUint32VarModClosed(graph_, machine_, common_, left, right);
  ReplaceCheckedWithValue(graph_, node, replacement);
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

void RawInt32StrengthReduction::ReduceRawFloat64DivOrMod(Node* node,
                                                          bool is_div) {
  Node* left = node->InputAt(0);
  Node* right = node->InputAt(1);

  Node* left_unwrapped = UnwrapConstantNode(left);
  Node* right_unwrapped = UnwrapConstantNode(right);
  if (left_unwrapped == nullptr || right_unwrapped == nullptr) return;

  int64_t rhs = 0;
  if (!TryGetFloat64IntegralConstant(right, &rhs)) return;

  if (IsRawTyped(left_unwrapped, RawIntKind::kInt32) &&
      rhs >= std::numeric_limits<int32_t>::min() &&
      rhs <= std::numeric_limits<int32_t>::max()) {
    int32_t rhs_i32_value = static_cast<int32_t>(rhs);
    if (rhs_i32_value == 0) {
      return;
    }
    if (is_div && rhs_i32_value == std::numeric_limits<int32_t>::min()) return;

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
    return;
  }

  if (IsRawTyped(left_unwrapped, RawIntKind::kUint32) && rhs >= 0 &&
      static_cast<uint64_t>(rhs) <= std::numeric_limits<uint32_t>::max()) {
    uint32_t rhs_u32_value = static_cast<uint32_t>(rhs);
    if (rhs_u32_value == 0) {
      return;
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
  }
}

void RawInt32StrengthReduction::Run() {
  EnsureMetadataLoaded();

  AllNodes all(graph_->zone(), graph_);
  for (Node* node : all.reachable) {
    switch (node->opcode()) {
      case IrOpcode::kCheckedInt32Add:
        ReduceCheckedBinop(node, machine_->Int32Add(), RawIntKind::kInt32);
        break;
      case IrOpcode::kCheckedInt32Sub:
        ReduceCheckedBinop(node, machine_->Int32Sub(), RawIntKind::kInt32);
        break;
      case IrOpcode::kCheckedInt32Mul:
        ReduceCheckedBinop(node, machine_->Int32Mul(), RawIntKind::kInt32);
        break;
      case IrOpcode::kCheckedInt32Div:
        ReduceCheckedInt32DivOrModClosed(node, true);
        break;
      case IrOpcode::kCheckedInt32Mod:
        ReduceCheckedInt32DivOrModClosed(node, false);
        break;
      case IrOpcode::kCheckedUint32Div:
        ReduceCheckedUint32DivOrModClosed(node, true);
        break;
      case IrOpcode::kCheckedUint32Mod:
        ReduceCheckedUint32DivOrModClosed(node, false);
        break;
      case IrOpcode::kCheckedInt64Add:
        if (IsMachineWord64Like(node->InputAt(0)) &&
            IsMachineWord64Like(node->InputAt(1)) &&
            IsTypeOrLiteralCompatible(node->InputAt(0), RawIntKind::kInt64) &&
            IsTypeOrLiteralCompatible(node->InputAt(1), RawIntKind::kInt64)) {
          ReduceCheckedBinop(node, machine_->Int64Add(), RawIntKind::kInt64);
        } else if (IsMachineWord64Like(node->InputAt(0)) &&
                   IsMachineWord64Like(node->InputAt(1)) &&
                   IsTypeOrLiteralCompatible(node->InputAt(0),
                                             RawIntKind::kUint64) &&
                   IsTypeOrLiteralCompatible(node->InputAt(1),
                                             RawIntKind::kUint64)) {
          ReduceCheckedBinop(node, machine_->Int64Add(), RawIntKind::kUint64);
        }
        break;
      case IrOpcode::kCheckedInt64Sub:
        if (IsMachineWord64Like(node->InputAt(0)) &&
            IsMachineWord64Like(node->InputAt(1)) &&
            IsTypeOrLiteralCompatible(node->InputAt(0), RawIntKind::kInt64) &&
            IsTypeOrLiteralCompatible(node->InputAt(1), RawIntKind::kInt64)) {
          ReduceCheckedBinop(node, machine_->Int64Sub(), RawIntKind::kInt64);
        } else if (IsMachineWord64Like(node->InputAt(0)) &&
                   IsMachineWord64Like(node->InputAt(1)) &&
                   IsTypeOrLiteralCompatible(node->InputAt(0),
                                             RawIntKind::kUint64) &&
                   IsTypeOrLiteralCompatible(node->InputAt(1),
                                             RawIntKind::kUint64)) {
          ReduceCheckedBinop(node, machine_->Int64Sub(), RawIntKind::kUint64);
        }
        break;
      case IrOpcode::kCheckedInt64Mul:
        if (IsMachineWord64Like(node->InputAt(0)) &&
            IsMachineWord64Like(node->InputAt(1)) &&
            IsTypeOrLiteralCompatible(node->InputAt(0), RawIntKind::kInt64) &&
            IsTypeOrLiteralCompatible(node->InputAt(1), RawIntKind::kInt64)) {
          ReduceCheckedBinop(node, machine_->Int64Mul(), RawIntKind::kInt64);
        } else if (IsMachineWord64Like(node->InputAt(0)) &&
                   IsMachineWord64Like(node->InputAt(1)) &&
                   IsTypeOrLiteralCompatible(node->InputAt(0),
                                             RawIntKind::kUint64) &&
                   IsTypeOrLiteralCompatible(node->InputAt(1),
                                             RawIntKind::kUint64)) {
          ReduceCheckedBinop(node, machine_->Int64Mul(), RawIntKind::kUint64);
        }
        break;
      case IrOpcode::kCheckedInt64Div:
        ReduceCheckedInt64DivOrMod(node, true);
        break;
      case IrOpcode::kCheckedInt64Mod:
        ReduceCheckedInt64DivOrMod(node, false);
        break;
      default:
        break;
    }
  }

  for (Node* node : all.reachable) {
    switch (node->opcode()) {
      case IrOpcode::kFloat64Div:
        ReduceRawFloat64DivOrMod(node, true);
        break;
      case IrOpcode::kFloat64Mod:
        ReduceRawFloat64DivOrMod(node, false);
        break;
      default:
        break;
    }
  }
}

}  // namespace v8::internal::compiler
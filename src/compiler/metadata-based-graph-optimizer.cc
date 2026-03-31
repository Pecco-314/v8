#include "src/compiler/metadata-based-graph-optimizer.h"

#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/operator-properties.h"
#include "src/compiler/simplified-operator.h"
#include "src/compiler/turbofan-graph.h"
#include "src/common/message-template.h"
#include "src/objects/js-array.h"
#include "src/runtime/runtime.h"

#include <vector>

namespace v8::internal::compiler {

void MetadataBasedGraphOptimizer::Run() {
  EnsureMetadataLoaded();

  AllNodes all(graph_->zone(), graph_);

  for (Node* node : all.reachable) {
    OptimizeTupleLength(node);
  }

  for (Node* node : all.reachable) {
    OptimizeLoadElementBounds(node);
  }

  for (Node* node : all.reachable) {
    ProcessCheckMapsNode(node);
  }

  for (Node* node : all.reachable) {
    OptimizeRawIntDivModZeroGuard(node);
  }
}

bool MetadataBasedGraphOptimizer::IsRawInt32Like(
    const std::optional<TypeAST>& type_opt) const {
  if (!type_opt.has_value()) return false;
  return type_opt->kind == TypeAST::RawInt32 ||
         type_opt->kind == TypeAST::RawUint32;
}

bool MetadataBasedGraphOptimizer::IsRawInt32Node(Node* node) {
  while (node != nullptr) {
    switch (node->opcode()) {
      case IrOpcode::kJSToNumber:
      case IrOpcode::kJSToNumeric:
      case IrOpcode::kTypeGuard:
      case IrOpcode::kCheckNumber:
      case IrOpcode::kCheckedTaggedToFloat64:
      case IrOpcode::kChangeTaggedToFloat64:
      case IrOpcode::kCheckedTaggedSignedToInt32:
      case IrOpcode::kCheckedTaggedToInt32:
      case IrOpcode::kChangeFloat64ToInt32:
      case IrOpcode::kChangeFloat64ToUint32:
      case IrOpcode::kChangeInt32ToFloat64:
      case IrOpcode::kChangeUint32ToFloat64:
      case IrOpcode::kChangeFloat64ToTagged:
        if (node->InputCount() == 0) break;
        node = node->InputAt(0);
        continue;
      default:
        break;
    }
    break;
  }
  if (node == nullptr) return false;
  return IsRawInt32Like(GetNodeTypeAST(node));
}

void MetadataBasedGraphOptimizer::OptimizeRawIntDivModZeroGuard(Node* node) {
  if (node->opcode() != IrOpcode::kJSDivide &&
      node->opcode() != IrOpcode::kJSModulus &&
      node->opcode() != IrOpcode::kSpeculativeNumberDivide &&
      node->opcode() != IrOpcode::kSpeculativeNumberModulus) {
    return;
  }

  Node* left = NodeProperties::GetValueInput(node, 0);
  Node* right = NodeProperties::GetValueInput(node, 1);
  bool both_rawint = IsRawInt32Node(left) && IsRawInt32Node(right);
  if (!both_rawint) return;

  Node* effect = NodeProperties::GetEffectInput(node);
  Node* control = NodeProperties::GetControlInput(node);
  Node* frame_state = OperatorProperties::HasFrameStateInput(node->op())
                          ? NodeProperties::GetFrameStateInput(node)
                          : NodeProperties::FindFrameStateBefore(node, nullptr);
  if (frame_state == nullptr || frame_state->opcode() != IrOpcode::kFrameState) {
    return;
  }

  Node* context = OperatorProperties::HasContextInput(node->op())
                      ? NodeProperties::GetContextInput(node)
                      : FrameState(frame_state).context();
  if (context == nullptr) return;

  Node* zero = jsgraph_->SmiConstant(0);
  Node* is_zero = graph_->NewNode(simplified_->ReferenceEqual(), right, zero);
  Node* branch =
      graph_->NewNode(common_->Branch(BranchHint::kFalse), is_zero, control);
  Node* if_zero = graph_->NewNode(common_->IfTrue(), branch);
  Node* if_nonzero = graph_->NewNode(common_->IfFalse(), branch);

  Node* message =
      jsgraph_->SmiConstant(static_cast<int>(MessageTemplate::kBigIntDivZero));
  Node* effect_zero = effect;
  Node* control_zero = if_zero;
    effect_zero = control_zero = graph_->NewNode(
      javascript_->CallRuntime(Runtime::kThrowRangeError, 1), message, context,
      frame_state, effect_zero, control_zero);

  Node* throw_node = graph_->NewNode(common_->Throw(), effect_zero, control_zero);
  NodeProperties::MergeControlToEnd(graph_, common_, throw_node);

  std::vector<Node*> effect_users;
  for (Edge edge : node->use_edges()) {
    if (NodeProperties::IsEffectEdge(edge)) {
      effect_users.push_back(edge.from());
    }
  }

  for (Node* use : effect_users) {
    if (use->InputCount() == 0) continue;
    if (use->op()->ControlInputCount() > 0 &&
        NodeProperties::GetControlInput(use) == control) {
      NodeProperties::ReplaceControlInput(use, if_nonzero);
    }
  }

  NodeProperties::ReplaceControlInput(node, if_nonzero);
}

void MetadataBasedGraphOptimizer::OptimizeTupleLength(Node* node) {
  if (node->opcode() != IrOpcode::kLoadField) return;

  FieldAccess const& access = FieldAccessOf(node->op());
  if (access.offset != JSArray::kLengthOffset) return;

  Node* object_node = NodeProperties::GetValueInput(node, 0);
  auto object_type_opt = GetNodeTypeAST(object_node);
  if (!object_type_opt.has_value()) return;

  const TypeAST& object_type = object_type_opt.value();
  if (object_type.kind != TypeAST::Tuple) return;

  int tuple_length = static_cast<int>(object_type.children.size());
  ReplaceTupleLengthWithConstant(node, tuple_length);
}

void MetadataBasedGraphOptimizer::OptimizeLoadElementBounds(Node* node) {
  if (node->opcode() != IrOpcode::kLoadElement) return;

  Node* elements_node = node->InputAt(0);
  Node* array_node = elements_node;
  if (elements_node->opcode() == IrOpcode::kLoadField) {
    array_node = elements_node->InputAt(0);
  }

  auto container_type_opt = GetNodeTypeAST(array_node);
  if (!container_type_opt.has_value()) return;

  const TypeAST& container_type = container_type_opt.value();
  if (container_type.kind != TypeAST::Tuple &&
      container_type.kind != TypeAST::Arr) {
    return;
  }

  Node* index_node = NodeProperties::GetValueInput(node, 1);
  auto index_opt = TryGetConstantIndex(index_node);
  if (!index_opt.has_value()) return;

  int index = index_opt.value();
  if (container_type.kind != TypeAST::Tuple) return;

  int length = static_cast<int>(container_type.children.size());
  if (index < 0 || index >= length) return;

  if (index_node->opcode() == IrOpcode::kCheckBounds ||
      index_node->opcode() == IrOpcode::kCheckedUint32Bounds ||
      index_node->opcode() == IrOpcode::kCheckedUint64Bounds) {
    Node* index_constant = index_node->InputAt(0);
    while (index_constant != nullptr &&
           (index_constant->opcode() == IrOpcode::kChangeUint32ToUint64 ||
            index_constant->opcode() == IrOpcode::kChangeInt32ToInt64)) {
      index_constant = index_constant->InputAt(0);
    }

    if (index_constant != nullptr &&
        (index_constant->opcode() == IrOpcode::kInt32Constant ||
         index_constant->opcode() == IrOpcode::kInt64Constant ||
         index_constant->opcode() == IrOpcode::kNumberConstant)) {
      RemoveTupleBoundsCheck(node, index_node, index_constant);
    }
  }
}

void MetadataBasedGraphOptimizer::RemoveTupleBoundsCheck(
    Node* load_element_node, Node* check_bounds_node, Node* index_constant) {
  Node* index_input = NodeProperties::GetValueInput(load_element_node, 1);
  Node* effect_input = NodeProperties::GetEffectInput(load_element_node);

  if (index_input != check_bounds_node && effect_input != check_bounds_node) {
    return;
  }

  if (index_input == check_bounds_node) {
    int value_index = NodeProperties::FirstValueIndex(load_element_node) + 1;
    load_element_node->ReplaceInput(value_index, index_constant);
  }

  if (effect_input == check_bounds_node) {
    Node* check_bounds_effect_input =
        NodeProperties::GetEffectInput(check_bounds_node);
    int effect_index = NodeProperties::FirstEffectIndex(load_element_node);
    load_element_node->ReplaceInput(effect_index, check_bounds_effect_input);
  }
}

void MetadataBasedGraphOptimizer::ReplaceTupleLengthWithConstant(
    Node* load_field_node, int tuple_length) {
  Node* constant_node = graph_->NewNode(common_->NumberConstant(tuple_length));

  Node* load_field_effect = NodeProperties::GetEffectInput(load_field_node);

  ZoneVector<std::pair<Node*, int>> value_edges(graph_->zone());
  ZoneVector<std::pair<Node*, int>> effect_edges(graph_->zone());
  for (Edge edge : load_field_node->use_edges()) {
    Node* use = edge.from();
    int index = edge.index();
    if (index < NodeProperties::FirstEffectIndex(use)) {
      value_edges.push_back(std::make_pair(use, index));
    } else if (NodeProperties::IsEffectEdge(edge)) {
      effect_edges.push_back(std::make_pair(use, index));
    }
  }

  for (auto& pair : value_edges) {
    Node* use = pair.first;
    int index = pair.second;
    use->ReplaceInput(index, constant_node);
  }

  for (auto& pair : effect_edges) {
    Node* use = pair.first;
    int index = pair.second;
    use->ReplaceInput(index, load_field_effect);
  }
}

void MetadataBasedGraphOptimizer::ProcessCheckMapsNode(Node* node) {
  if (node->opcode() != IrOpcode::kCheckMaps) return;

  Node* value_input = NodeProperties::GetValueInput(node, 0);
  const TypeAST* type = GetNodeType(value_input);
  if (type != nullptr && type->kind == TypeAST::Symbol) {
    Node* effect_input = NodeProperties::GetEffectInput(node);
    Node* control_input = NodeProperties::GetControlInput(node);
    NodeProperties::ReplaceUses(node, nullptr, effect_input, control_input,
                                control_input);
    node->Kill();
  }
}

}  // namespace v8::internal::compiler
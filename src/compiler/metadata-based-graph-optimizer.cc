#include "src/compiler/metadata-based-graph-optimizer.h"

#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/simplified-operator.h"
#include "src/compiler/turbofan-graph.h"
#include "src/objects/js-array.h"

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
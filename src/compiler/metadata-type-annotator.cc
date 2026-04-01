#include "src/compiler/metadata-type-annotator.h"

#include "src/compiler/heap-refs.h"
#include "src/compiler/js-heap-broker.h"
#include "src/compiler/js-operator.h"
#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/simplified-operator.h"
#include "src/compiler/turbofan-graph.h"
#include "src/objects/casting-inl.h"
#include "src/objects/heap-object-inl.h"
#include "src/objects/js-array.h"
#include "src/objects/map-inl.h"
#include "src/objects/shared-function-info-inl.h"

namespace v8::internal::compiler {

#if V8_COMPILER_TYPE_INJECTOR_DEBUG
#include "src/base/logging.h"
#define TYPE_INJECTOR_DEBUG(...) \
  do { PrintF("[MetadataTypeAnnotator] " __VA_ARGS__); PrintF("\n"); } while (false)
#else
#define TYPE_INJECTOR_DEBUG(...) ((void)0)
#endif

void MetadataTypeAnnotator::Run() {
  EnsureMetadataLoaded();

  AllNodes all(graph_->zone(), graph_);
  RunTypeAnnotation(all);
}

void MetadataTypeAnnotator::RunTypeAnnotation(AllNodes& all) {
  auto& param_types = context_.param_types();
  for (Node* node : all.reachable) {
    if (node->opcode() == IrOpcode::kParameter) {
      int index = ParameterIndexOf(node->op());
      if (index >= 0 && index < static_cast<int>(param_types.size()) - 1) {
        const TypeAST& ast = param_types[index];
        AnnotateNode(node, &ast);
      }
    }
  }

  for (Node* node : all.reachable) {
    ProcessLoadFieldNode(node);
  }

  for (Node* node : all.reachable) {
    ProcessLoadElementNode(node);
  }

  for (Node* node : all.reachable) {
    ProcessJSKeyedPropertyNode(node);
  }

  for (Node* node : all.reachable) {
    ProcessJSCallNode(node);
  }
}

void MetadataTypeAnnotator::ProcessLoadFieldNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadField) return;

  FieldAccess const& access = FieldAccessOf(node->op());

  if (access.offset == JSArray::kLengthOffset) {
    Node* object_node = NodeProperties::GetValueInput(node, 0);
    auto object_type_opt = GetNodeTypeAST(object_node);
    if (object_type_opt.has_value()) {
      const TypeAST& object_type = object_type_opt.value();
      if (object_type.kind == TypeAST::Tuple) {
        const TypeAST* len_ptr = StoreOwnedTypeAST(object_type);
        SetNodeType(node, len_ptr);
      }
    }
  }

  std::string field_name;
  if (!access.name.is_null()) {
    Handle<Name> name_handle = access.name.ToHandleChecked();
    InstanceType type = name_handle->map()->instance_type();
    if (FIRST_STRING_TYPE <= type && type <= LAST_STRING_TYPE) {
      DirectHandle<String> str = Cast<String>(name_handle);
      field_name = str->ToCString().get();
    } else {
      return;
    }
  } else {
    return;
  }

  Node* object_node = NodeProperties::GetValueInput(node, 0);
  auto object_type_opt = GetNodeTypeAST(object_node);
  if (!object_type_opt.has_value()) {
    return;
  }

  const TypeAST& object_type = object_type_opt.value();
  if (object_type.kind != TypeAST::Obj) {
    return;
  }

  auto field_type = FindFieldInObj(object_type, field_name);
  if (field_type.has_value()) {
    const TypeAST* field_ptr = StoreOwnedTypeAST(field_type.value());
    if (field_type->kind == TypeAST::Num) {
      SetNodeType(node, field_ptr);
    } else {
      AnnotateNode(node, field_ptr);
    }
  }
}

void MetadataTypeAnnotator::ProcessLoadElementNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadElement) return;

  Node* elements_node = node->InputAt(0);
  Node* array_node = elements_node;
  if (elements_node->opcode() == IrOpcode::kLoadField) {
    array_node = elements_node->InputAt(0);
  }

  auto container_type_opt = GetNodeTypeAST(array_node);
  if (!container_type_opt.has_value()) {
    return;
  }

  const TypeAST& container_type = container_type_opt.value();
  std::optional<TypeAST> element_type;

  if (container_type.kind == TypeAST::Arr) {
    element_type = GetElementTypeInArray(container_type);
  } else if (container_type.kind == TypeAST::Tuple) {
    Node* index_node = NodeProperties::GetValueInput(node, 1);
    auto index_opt = TryGetConstantIndex(index_node);
    if (index_opt.has_value()) {
      int index = index_opt.value();
      int tuple_length = static_cast<int>(container_type.children.size());
      if (index >= 0 && index < tuple_length) {
        element_type = GetElementTypeInTuple(container_type, index);
      }
    }
  } else {
    return;
  }

  if (element_type.has_value()) {
    const TypeAST* elem_ptr = StoreOwnedTypeAST(element_type.value());
    AnnotateNode(node, elem_ptr);
  }
}

void MetadataTypeAnnotator::ProcessJSKeyedPropertyNode(Node* node) {
  bool is_load = node->opcode() == IrOpcode::kJSLoadProperty;
  bool is_store = node->opcode() == IrOpcode::kJSSetKeyedProperty;
  if (!is_load && !is_store) return;
  if (node->op()->ValueInputCount() < 2) return;

  Node* receiver = NodeProperties::GetValueInput(node, 0);
  Node* index = NodeProperties::GetValueInput(node, 1);

  auto receiver_type_opt = GetNodeTypeAST(receiver);
  if (!receiver_type_opt.has_value()) return;

  const TypeAST& receiver_type = receiver_type_opt.value();
  std::optional<TypeAST> element_type;

  if (receiver_type.kind == TypeAST::Arr) {
    element_type = GetElementTypeInArray(receiver_type);
  } else if (receiver_type.kind == TypeAST::Tuple) {
    auto index_opt = TryGetConstantIndex(index);
    if (index_opt.has_value()) {
      int tuple_length = static_cast<int>(receiver_type.children.size());
      if (index_opt.value() >= 0 && index_opt.value() < tuple_length) {
        element_type = GetElementTypeInTuple(receiver_type, index_opt.value());
      }
    }
  } else {
    return;
  }

  if (!element_type.has_value()) return;

  const TypeAST* elem_ptr = StoreOwnedTypeAST(element_type.value());

  if (is_load) {
    AnnotateNode(node, elem_ptr);
    return;
  }

  if (node->op()->ValueInputCount() < 3) return;
  Node* value = NodeProperties::GetValueInput(node, 2);
  AnnotateNode(value, elem_ptr);
}

void MetadataTypeAnnotator::ProcessJSCallNode(Node* node) {
  if (node->opcode() != IrOpcode::kJSCall) return;

  Node* target = NodeProperties::GetValueInput(node, 0);
  if (target->opcode() != IrOpcode::kHeapConstant) {
    return;
  }

  Type target_type = NodeProperties::GetType(target);
  if (!target_type.IsHeapConstant()) {
    return;
  }

  HeapObjectRef target_ref = target_type.AsHeapConstant()->Ref();
  if (!target_ref.IsJSFunction()) {
    return;
  }

  JSFunctionRef function = target_ref.AsJSFunction();
  OptionalSharedFunctionInfoRef shared_opt = function.shared(broker_);
  if (!shared_opt.has_value()) {
    return;
  }

  SharedFunctionInfoRef shared = shared_opt.value();

  if (v8_flags.turbo_builtin_type_table && shared.HasBuiltinId()) {
    Builtin builtin_id = shared.builtin_id();
    auto signature = storage_->GetBuiltinSignature(builtin_id);

    if (signature.has_value()) {
      AnnotateNode(node, signature->return_type);

      JSCallNode call_node(node);
      Node* receiver = call_node.receiver();

      const auto& param_types = signature->param_types;
      if (!param_types.empty() && receiver != nullptr) {
        AnnotateNode(receiver, param_types[0]);
      }

      size_t arg_count = call_node.ArgumentCount();
      for (size_t i = 0; i < arg_count && i + 1 < param_types.size(); ++i) {
        Node* arg = NodeProperties::GetValueInput(node, static_cast<int>(2 + i));
        if (arg != nullptr) {
          AnnotateNode(arg, param_types[i + 1]);
        }
      }

      return;
    }
  }

  int start_pos = shared.StartPosition();
  auto return_type_opt = GetFunctionReturnType(start_pos);
  if (!return_type_opt.has_value()) {
    return;
  }

  const TypeAST& return_type_ast = return_type_opt.value();
  const TypeAST* ret_ptr = StoreOwnedTypeAST(return_type_ast);
  AnnotateNode(node, ret_ptr);
}

}  // namespace v8::internal::compiler
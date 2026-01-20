#include "src/compiler/metadata-type-helper.h"

#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/simplified-operator.h"
#include "src/objects/casting-inl.h"
#include "src/objects/heap-object-inl.h"
#include "src/objects/map-inl.h"
#include "src/objects/name.h"

namespace v8::internal::compiler {

MetadataTypeHelper::MetadataTypeHelper(OptimizedCompilationInfo* compilation_info,
                                       TFGraph* graph,
                                       CommonOperatorBuilder* common,
                                       JSHeapBroker* broker,
                                       SimplifiedOperatorBuilder* simplified)
    : compilation_info_(compilation_info),
      graph_(graph),
      common_(common),
      broker_(broker),
      simplified_(simplified),
      storage_(TypeStorage::Get()),
      context_(MetadataTypeContextStore::Get(graph)) {}

void MetadataTypeHelper::EnsureMetadataLoaded() {
  context_.EnsureLoaded(compilation_info_, storage_);
}

const TypeAST* MetadataTypeHelper::StoreOwnedTypeAST(const TypeAST& ast) {
  return context_.StoreOwnedTypeAST(ast);
}

void MetadataTypeHelper::SetNodeType(Node* node, const TypeAST* type_ast) {
  context_.SetNodeType(node, type_ast);
}

const TypeAST* MetadataTypeHelper::GetNodeType(Node* node) const {
  return context_.GetNodeType(node);
}

Type MetadataTypeHelper::TypeASTToType(const TypeAST& ast) {
  switch (ast.kind) {
    case TypeAST::Num:
      return Type::Number();
    case TypeAST::Str:
      return Type::String();
    case TypeAST::Bool:
      return Type::Boolean();
    case TypeAST::Symbol:
      return Type::Symbol();
    case TypeAST::RawInt32:
      return Type::Any();
    case TypeAST::BigInt:
      return Type::BigInt();
    case TypeAST::Arr:
      return Type::Array();
    case TypeAST::Tuple:
      return Type::Array();
    case TypeAST::Obj:
      return Type::Object();
    default:
      return Type::Any();
  }
}

std::optional<TypeAST> MetadataTypeHelper::FindFieldInObj(
    const TypeAST& obj_ast, const std::string& field_name) {
  if (obj_ast.kind != TypeAST::Obj) {
    return std::nullopt;
  }
  for (const auto& child : obj_ast.children) {
    if (child.field_name == field_name) {
      return child;
    }
  }
  return std::nullopt;
}

std::optional<TypeAST> MetadataTypeHelper::GetElementTypeInArray(
    const TypeAST& array_ast) {
  if (array_ast.kind != TypeAST::Arr) {
    return std::nullopt;
  }
  if (!array_ast.children.empty()) {
    return array_ast.children[0];
  }
  return std::nullopt;
}

std::optional<TypeAST> MetadataTypeHelper::GetElementTypeInTuple(
    const TypeAST& tuple_ast, int index) {
  if (tuple_ast.kind != TypeAST::Tuple) {
    return std::nullopt;
  }
  if (index >= 0 && index < static_cast<int>(tuple_ast.children.size())) {
    return tuple_ast.children[index];
  }
  return std::nullopt;
}

std::optional<int> MetadataTypeHelper::TryGetConstantIndex(Node* node) {
  while (node != nullptr) {
    switch (node->opcode()) {
      case IrOpcode::kInt32Constant:
        return OpParameter<int32_t>(node->op());
      case IrOpcode::kInt64Constant:
        return static_cast<int>(OpParameter<int64_t>(node->op()));
      case IrOpcode::kNumberConstant: {
        double value = OpParameter<double>(node->op());
        return static_cast<int>(value);
      }
      case IrOpcode::kChangeUint32ToUint64:
      case IrOpcode::kChangeInt32ToInt64:
      case IrOpcode::kCheckedUint32Bounds:
      case IrOpcode::kCheckedUint64Bounds:
      case IrOpcode::kCheckBounds:
        node = node->InputAt(0);
        break;
      default:
        return std::nullopt;
    }
  }
  return std::nullopt;
}

std::optional<TypeAST> MetadataTypeHelper::GetNodeTypeAST(Node* node) {
  if (const TypeAST* cached = GetNodeType(node)) {
    return *cached;
  }
  if (node->opcode() == IrOpcode::kParameter) {
    int param_index = ParameterIndexOf(node->op());
    auto& param_types = context_.param_types();
    if (param_index >= 0 &&
        param_index < static_cast<int>(param_types.size())) {
      SetNodeType(node, &param_types[param_index]);
      return param_types[param_index];
    }
  } else if (node->opcode() == IrOpcode::kLoadField) {
    Node* object_node = node->InputAt(0);
    auto object_type_opt = GetNodeTypeAST(object_node);
    if (!object_type_opt.has_value()) {
      return std::nullopt;
    }

    const TypeAST& object_type = object_type_opt.value();
    if (object_type.kind != TypeAST::Obj) {
      return std::nullopt;
    }

    FieldAccess const& access = FieldAccessOf(node->op());
    if (access.name.is_null()) {
      return std::nullopt;
    }

    Handle<Name> name_handle = access.name.ToHandleChecked();
    InstanceType type = name_handle->map()->instance_type();
    if (FIRST_STRING_TYPE <= type && type <= LAST_STRING_TYPE) {
      DirectHandle<String> str = Cast<String>(name_handle);
      std::string field_name = str->ToCString().get();
      return FindFieldInObj(object_type, field_name);
    }
  } else if (node->opcode() == IrOpcode::kLoadElement) {
    Node* array_node = node->InputAt(0);
    auto array_type_opt = GetNodeTypeAST(array_node);
    if (!array_type_opt.has_value()) {
      return std::nullopt;
    }

    const TypeAST& array_type = array_type_opt.value();
    if (array_type.kind == TypeAST::Arr) {
      return GetElementTypeInArray(array_type);
    } else if (array_type.kind == TypeAST::Tuple) {
      Node* index_node = node->InputAt(1);
      auto index_opt = TryGetConstantIndex(index_node);
      if (index_opt.has_value()) {
        return GetElementTypeInTuple(array_type, index_opt.value());
      }
    }
  }

  return std::nullopt;
}

std::optional<TypeAST> MetadataTypeHelper::GetFunctionReturnType(
    int start_pos) {
  auto typemap = storage_->GetTypeMap(context_.script_hash());
  auto it = typemap.find(start_pos);
  if (it != typemap.end() && !it->second.empty()) {
    return it->second.back();
  }
  return std::nullopt;
}

}  // namespace v8::internal::compiler
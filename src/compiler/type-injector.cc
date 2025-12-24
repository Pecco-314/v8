#include "src/compiler/type-injector.h"

#include <iostream>

#include "src/compiler/all-nodes.h"
#include "src/compiler/common-operator.h"
#include "src/compiler/js-heap-broker.h"
#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/simplified-operator.h"
#include "src/compiler/turbofan-types.h"
#include "src/objects/casting-inl.h"
#include "src/objects/heap-object-inl.h"
#include "src/objects/map-inl.h"

namespace v8 {
namespace internal {
namespace compiler {

// 将 TypeAST 转换为 Turbofan Type
Type TypeInjector::TypeASTToType(const TypeAST& ast) {
  switch (ast.kind) {
    case TypeAST::Num:
      return Type::Number();
    case TypeAST::Str:
      return Type::String();
    case TypeAST::Bool:
      return Type::Boolean();
    case TypeAST::Arr:
      return Type::Array();
    case TypeAST::Interface:
      return Type::Object();
    default:
      return Type::Any();
  }
}

// 从 Interface 类型 AST 中查找字段
std::optional<TypeAST> TypeInjector::FindFieldInInterface(
    const TypeAST& interface_ast, const std::string& field_name) {
  if (interface_ast.kind != TypeAST::Interface) {
    return std::nullopt;
  }

  // Interface 的 children 中每一项都有 field_name
  for (const auto& child : interface_ast.children) {
    if (child.field_name == field_name) {
      return child;
    }
  }
  return std::nullopt;
}

// 从 Array 类型 AST 中获取元素类型
std::optional<TypeAST> TypeInjector::GetElementTypeInArray(
    const TypeAST& array_ast) {
  if (array_ast.kind != TypeAST::Arr) {
    return std::nullopt;
  }

  // Array 的元素类型存储在 children[0]
  if (array_ast.children.size() > 0) {
    return array_ast.children[0];
  }
  return std::nullopt;
}

// 递归获取一个 Node 对应的 TypeAST
// 如果 Node 是 Parameter，返回该参数的 TypeAST
// 如果 Node 是 LoadField，则递归获取其输入对象的类型，然后查找字段类型
// 如果 Node 是 LoadElement，则递归获取其输入数组的类型，然后查找元素类型
std::optional<TypeAST> TypeInjector::GetNodeTypeAST(Node* node) {
  if (node->opcode() == IrOpcode::kParameter) {
    int param_index = ParameterIndexOf(node->op());
    if (param_index >= 0 &&
        param_index < static_cast<int>(param_types_.size())) {
      return param_types_[param_index];
    }
  } else if (node->opcode() == IrOpcode::kLoadField) {
    // 递归获取 LoadField 的输入对象的类型
    Node* object_node = node->InputAt(0);
    auto object_type_opt = GetNodeTypeAST(object_node);
    if (!object_type_opt.has_value()) {
      return std::nullopt;
    }

    const TypeAST& object_type = object_type_opt.value();
    if (object_type.kind != TypeAST::Interface) {
      return std::nullopt;
    }

    // 获取该 LoadField 的字段名
    FieldAccess const& access = FieldAccessOf(node->op());
    if (access.name.is_null()) {
      return std::nullopt;
    }

    Handle<Name> name_handle = access.name.ToHandleChecked();
    InstanceType type = name_handle->map()->instance_type();
    if (FIRST_STRING_TYPE <= type && type <= LAST_STRING_TYPE) {
      DirectHandle<String> str = Cast<String>(name_handle);
      std::string field_name = str->ToCString().get();

      // 从 Interface 中查找该字段的类型
      return FindFieldInInterface(object_type, field_name);
    }
  } else if (node->opcode() == IrOpcode::kLoadElement) {
    // 递归获取 LoadElement 的输入数组的类型
    Node* array_node = node->InputAt(0);
    auto array_type_opt = GetNodeTypeAST(array_node);
    if (!array_type_opt.has_value()) {
      return std::nullopt;
    }

    const TypeAST& array_type = array_type_opt.value();
    if (array_type.kind != TypeAST::Arr) {
      return std::nullopt;
    }

    // 从 Array 中获取元素类型
    return GetElementTypeInArray(array_type);
  }

  return std::nullopt;
}

void TypeInjector::ProcessLoadFieldNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadField) return;

  // 获取操作符的参数
  FieldAccess const& access = FieldAccessOf(node->op());

  // 获取字段名
  std::string field_name;
  if (!access.name.is_null()) {
    Handle<Name> name_handle = access.name.ToHandleChecked();
    InstanceType type = name_handle->map()->instance_type();
    if (FIRST_STRING_TYPE <= type && type <= LAST_STRING_TYPE) {
      DirectHandle<String> str = Cast<String>(name_handle);
      field_name = str->ToCString().get();
    } else {
      return;  // 非 String 类型的字段名，跳过
    }
  } else {
    return;  // 没有字段名，跳过
  }
  std::cout << "[TypeInjector] Processing LoadField for field: " << field_name
            << std::endl;

  // 获取 LoadField 的输入节点（object）
  Node* object_node = node->InputAt(0);

  // 递归获取对象的类型
  auto object_type_opt = GetNodeTypeAST(object_node);
  if (!object_type_opt.has_value()) {
    return;
  }

  const TypeAST& object_type = object_type_opt.value();
  if (object_type.kind != TypeAST::Interface) {
    return;
  }

  // 从 Interface 中查找字段
  auto field_type = FindFieldInInterface(object_type, field_name);
  std::cout << "[TypeInjector] Field '" << field_name << "' lookup "
            << (field_type.has_value() ? "succeeded." : "failed.") << std::endl;
  if (field_type.has_value()) {
    Type field_turbofan_type = TypeASTToType(field_type.value());
    std::cout << "[TypeInjector] Setting type for Node #" << node->id()
              << " to: " << field_turbofan_type << std::endl;
    NodeProperties::SetType(node, field_turbofan_type);
  }
}

// 处理 LoadElement 节点的类型注入
void TypeInjector::ProcessLoadElementNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadElement) return;

  std::cout << "[TypeInjector] Processing LoadElement for Node #" << node->id()
            << std::endl;

  // 获取 LoadElement 的输入节点（array elements）
  // LoadElement 的输入通常是：[0] = elements buffer, [1] = index, [2] = length
  // 但在 IR 中，elements buffer 通常来自 LoadField 操作
  Node* elements_node = node->InputAt(0);
  std::cout << "[TypeInjector]   Input[0]: Node #" << elements_node->id()
            << " (" << elements_node->op()->mnemonic() << ")" << std::endl;

  // 如果 elements_node 是 LoadField，需要找到真正的数组对象
  Node* array_node = elements_node;
  if (elements_node->opcode() == IrOpcode::kLoadField) {
    // 这是 LoadField，其输入应该是数组对象
    array_node = elements_node->InputAt(0);
    std::cout << "[TypeInjector]   Following LoadField to Node #" << array_node->id()
              << " (" << array_node->op()->mnemonic() << ")" << std::endl;
  }

  // 递归获取数组的类型
  auto array_type_opt = GetNodeTypeAST(array_node);
  if (!array_type_opt.has_value()) {
    std::cout << "[TypeInjector]   Failed to get type for array node" << std::endl;
    return;
  }

  const TypeAST& array_type = array_type_opt.value();
  std::cout << "[TypeInjector]   Array type kind: " << array_type.kind << std::endl;
  if (array_type.kind != TypeAST::Arr) {
    std::cout << "[TypeInjector]   Array type is not Arr" << std::endl;
    return;
  }

  // 从 Array 中获取元素类型
  auto element_type = GetElementTypeInArray(array_type);
  std::cout << "[TypeInjector] Element type lookup "
            << (element_type.has_value() ? "succeeded." : "failed.")
            << std::endl;
  if (element_type.has_value()) {
    Type element_turbofan_type = TypeASTToType(element_type.value());
    std::cout << "[TypeInjector] Setting type for Node #" << node->id()
              << " to: " << element_turbofan_type << std::endl;
    NodeProperties::SetType(node, element_turbofan_type);
  }
}

// 调试输出 LoadField 节点信息
void TypeInjector::InspectLoadFieldNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadField) return;

  // 获取操作符的参数
  FieldAccess const& access = FieldAccessOf(node->op());

  std::cout << "[TypeInjector] Inspecting Node #" << node->id()
            << " (LoadField):" << std::endl;

  // 1. 获取偏移量
  std::cout << "  -> Offset: " << access.offset << std::endl;

  // 2. 获取字段名
  if (!access.name.is_null()) {
    Handle<Name> name_handle = access.name.ToHandleChecked();
    // Name 可能是 String 或 Symbol，这里尝试将其转为字符串形式
    // 使用 InstanceType 来区分
    InstanceType type = name_handle->map()->instance_type();
    if (FIRST_STRING_TYPE <= type && type <= LAST_STRING_TYPE) {
      // 是 String 类型
      DirectHandle<String> str = Cast<String>(name_handle);
      std::cout << "  -> Name: " << str->ToCString().get() << std::endl;
    } else {
      // 可能是 Symbol 或其他，先输出 <non-string>
      std::cout << "  -> Name: <non-string>" << std::endl;
    }
  } else {
    std::cout << "  -> Name: <null>" << std::endl;
  }

  // 3. 获取 Map (Field Owner)
  // access.map 是 OptionalMapRef
  if (access.map.has_value()) {
    std::cout << "  -> Map: <present>" << std::endl;
  } else {
    std::cout << "  -> Map: <null>" << std::endl;
  }

  // 4. 字段类型
  std::cout << "  -> Field Type: " << access.type << std::endl;
}

// 调试输出 LoadElement 节点信息
void TypeInjector::InspectLoadElementNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadElement) return;

  std::cout << "[TypeInjector] Inspecting Node #" << node->id()
            << " (LoadElement):" << std::endl;

  // 获取操作符的参数
  ElementAccess const& access = ElementAccessOf(node->op());

  // 1. 获取元素类型
  std::cout << "  -> Element Type: " << access.type << std::endl;

  // 2. 获取基数是否有标记
  std::cout << "  -> Base Is Tagged: "
            << (access.base_is_tagged == kTaggedBase ? "true" : "false")
            << std::endl;

  // 3. 获取 header size
  std::cout << "  -> Header Size: " << access.header_size << std::endl;

  // 4. 获取 machine type
  std::cout << "  -> Machine Type: " << access.machine_type << std::endl;
}

void TypeInjector::Run() {
  const std::string& script_hash = compilation_info_->cached_script_hash();
  std::cout << "[TypeInjector] script_hash=" << script_hash << std::endl;

  IndirectHandle<SharedFunctionInfo> shared = compilation_info_->shared_info();
  int start_pos = shared->StartPosition();

  auto* storage = TypeStorage::Get();
  auto typemap = storage->GetTypeMap(script_hash);

  auto it = typemap.find(start_pos);
  if (it != typemap.end()) {
    param_types_ = it->second;
  }

  AllNodes all(graph_->zone(), graph_);

  // 第一次遍历：设置参数节点的类型
  for (Node* node : all.reachable) {
    if (node->opcode() == IrOpcode::kParameter) {
      int index = ParameterIndexOf(node->op());
      if (index >= 0 && index < static_cast<int>(param_types_.size())) {
        const TypeAST& ast = param_types_[index];
        Type type = TypeASTToType(ast);
        std::cout << "[TypeInjector] Parameter #" << index << ": ";
        ast.Print();
        std::cout << std::endl;
        NodeProperties::SetType(node, type);
      }
    }
  }

  // 第二次遍历：处理 LoadField 节点，注入字段类型
  for (Node* node : all.reachable) {
    ProcessLoadFieldNode(node);
    this->InspectLoadFieldNode(node);
  }

  // 第三次遍历：处理 LoadElement 节点，注入元素类型
  for (Node* node : all.reachable) {
    ProcessLoadElementNode(node);
    this->InspectLoadElementNode(node);
  }
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
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
#include "src/objects/js-array.h"
#include "src/objects/map-inl.h"
#include "src/zone/zone-containers.h"

namespace v8 {
namespace internal {
namespace compiler {

#ifndef V8_COMPILER_TYPE_INJECTOR_DEBUG
#define V8_COMPILER_TYPE_INJECTOR_DEBUG 0
#endif

#if V8_COMPILER_TYPE_INJECTOR_DEBUG
#define TYPE_INJECTOR_DEBUG(...) \
  do { std::cout << "[TypeInjector] "; std::cout << __VA_ARGS__ << std::endl; } while (false)
#else
#define TYPE_INJECTOR_DEBUG(...) ((void)0)
#endif

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
    case TypeAST::Tuple:
      // Tuple 底层也是数组，但长度固定，每个位置类型不同
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

// 从 Tuple 类型 AST 中获取指定索引的元素类型
std::optional<TypeAST> TypeInjector::GetElementTypeInTuple(
    const TypeAST& tuple_ast, int index) {
  if (tuple_ast.kind != TypeAST::Tuple) {
    return std::nullopt;
  }

  // Tuple 的每个元素类型存储在 children 中，按索引访问
  if (index >= 0 && index < static_cast<int>(tuple_ast.children.size())) {
    return tuple_ast.children[index];
  }
  return std::nullopt;
}

// 尝试从节点中提取常量索引值
// 追踪 ChangeUint32ToUint64、CheckedUint32Bounds、CheckBounds 等转换，找到原始常量
std::optional<int> TypeInjector::TryGetConstantIndex(Node* node) {
  while (node != nullptr) {
    switch (node->opcode()) {
      case IrOpcode::kInt32Constant:
        return OpParameter<int32_t>(node->op());
      case IrOpcode::kInt64Constant:
        return static_cast<int>(OpParameter<int64_t>(node->op()));
      case IrOpcode::kNumberConstant: {
        // NumberConstant 存储 double 值，转换为 int
        double value = OpParameter<double>(node->op());
        return static_cast<int>(value);
      }
      case IrOpcode::kChangeUint32ToUint64:
      case IrOpcode::kChangeInt32ToInt64:
      case IrOpcode::kCheckedUint32Bounds:
      case IrOpcode::kCheckedUint64Bounds:
      case IrOpcode::kCheckBounds:
        // 继续追踪第一个输入
        node = node->InputAt(0);
        break;
      default:
        // 无法追踪，返回空
        return std::nullopt;
    }
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
    // 递归获取 LoadElement 的输入数组/元组的类型
    Node* array_node = node->InputAt(0);
    auto array_type_opt = GetNodeTypeAST(array_node);
    if (!array_type_opt.has_value()) {
      return std::nullopt;
    }

    const TypeAST& array_type = array_type_opt.value();
    
    if (array_type.kind == TypeAST::Arr) {
      // Array: 所有元素类型相同
      return GetElementTypeInArray(array_type);
    } else if (array_type.kind == TypeAST::Tuple) {
      // Tuple: 需要根据索引获取类型
      // 尝试从 LoadElement 的索引输入中提取常量索引
      Node* index_node = node->InputAt(1);
      auto index_opt = TryGetConstantIndex(index_node);
      if (index_opt.has_value()) {
        return GetElementTypeInTuple(array_type, index_opt.value());
      }
    }
  }

  return std::nullopt;
}

// 带索引版本的 GetNodeTypeAST，用于 Tuple 类型
std::optional<TypeAST> TypeInjector::GetNodeTypeASTWithIndex(Node* node,
                                                              int index) {
  auto type_opt = GetNodeTypeAST(node);
  if (!type_opt.has_value()) {
      return std::nullopt;
    }

  const TypeAST& type = type_opt.value();
  if (type.kind == TypeAST::Tuple) {
    return GetElementTypeInTuple(type, index);
  } else if (type.kind == TypeAST::Arr) {
    return GetElementTypeInArray(type);
  }

  return std::nullopt;
}

void TypeInjector::ProcessLoadFieldNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadField) return;

  // 获取操作符的参数
  FieldAccess const& access = FieldAccessOf(node->op());

  // 检查是否是 JSArrayLength（offset == 12，JSArray::kLengthOffset）
  // JSArrayLength 的字段名可能是 null 或 "length"，需要通过 offset 识别
  if (access.offset == JSArray::kLengthOffset) {
    // 获取 LoadField 的输入节点（object）
    Node* object_node = NodeProperties::GetValueInput(node, 0);

    // 递归获取对象的类型
    auto object_type_opt = GetNodeTypeAST(object_node);
    if (object_type_opt.has_value()) {
      const TypeAST& object_type = object_type_opt.value();
      if (object_type.kind == TypeAST::Tuple) {
        // Tuple 的长度是固定的，直接用常量替换
        int tuple_length = static_cast<int>(object_type.children.size());
        TYPE_INJECTOR_DEBUG("Replacing Tuple length LoadField #" << node->id()
                                                                  << " with constant: " << tuple_length);
        ReplaceTupleLengthWithConstant(node, tuple_length);
        return;
      }
    }
  }

  // 处理有字段名的 LoadField（Interface 字段访问）
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
    return;  // 没有字段名且不是 JSArrayLength，跳过
  }
  TYPE_INJECTOR_DEBUG("Processing LoadField for field: " << field_name);

  // 获取 LoadField 的输入节点（object）
  Node* object_node = NodeProperties::GetValueInput(node, 0);

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
  TYPE_INJECTOR_DEBUG("Field '" << field_name << "' lookup "
                                 << (field_type.has_value() ? "succeeded." : "failed."));
  if (field_type.has_value()) {
    Type field_turbofan_type = TypeASTToType(field_type.value());
    TYPE_INJECTOR_DEBUG("Setting type for Node #" << node->id()
                                                   << " to: " << field_turbofan_type);
    NodeProperties::SetType(node, field_turbofan_type);
  }
}

// 处理 LoadElement 节点的类型注入
// 支持 Array 和 Tuple 两种类型
void TypeInjector::ProcessLoadElementNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadElement) return;

  TYPE_INJECTOR_DEBUG("Processing LoadElement for Node #" << node->id());

  // 获取 LoadElement 的输入节点（array/tuple elements）
  // LoadElement 的输入通常是：[0] = elements buffer, [1] = index
  // 在 IR 中，elements buffer 通常来自 LoadField 操作
  Node* elements_node = node->InputAt(0);
  TYPE_INJECTOR_DEBUG("  Input[0]: Node #" << elements_node->id()
                                           << " (" << elements_node->op()->mnemonic() << ")");

  // 如果 elements_node 是 LoadField，需要找到真正的数组/元组对象
  Node* array_node = elements_node;
  if (elements_node->opcode() == IrOpcode::kLoadField) {
    // 这是 LoadField，其输入应该是数组/元组对象
    array_node = elements_node->InputAt(0);
    TYPE_INJECTOR_DEBUG("  Following LoadField to Node #"
                        << array_node->id() << " (" << array_node->op()->mnemonic() << ")");
  }

  // 递归获取数组/元组的类型
  auto container_type_opt = GetNodeTypeAST(array_node);
  if (!container_type_opt.has_value()) {
    TYPE_INJECTOR_DEBUG("  Failed to get type for container node");
    return;
  }

  const TypeAST& container_type = container_type_opt.value();
  TYPE_INJECTOR_DEBUG("  Container type: " << container_type.KindToString());

  std::optional<TypeAST> element_type;

  if (container_type.kind == TypeAST::Arr) {
    // Array: 所有元素类型相同
    element_type = GetElementTypeInArray(container_type);
    TYPE_INJECTOR_DEBUG("  Array element type lookup "
                        << (element_type.has_value() ? "succeeded." : "failed."));
  } else if (container_type.kind == TypeAST::Tuple) {
    // Tuple: 每个位置类型不同，需要获取索引
    Node* index_node = NodeProperties::GetValueInput(node, 1);
    auto index_opt = TryGetConstantIndex(index_node);

    if (index_opt.has_value()) {
      int index = index_opt.value();
      int tuple_length = static_cast<int>(container_type.children.size());
      TYPE_INJECTOR_DEBUG("  Tuple index: " << index << ", length: " << tuple_length);
      
      // 检查索引是否在 Tuple 长度范围内
      if (index >= 0 && index < tuple_length) {
        element_type = GetElementTypeInTuple(container_type, index);
        TYPE_INJECTOR_DEBUG("  Tuple element type lookup "
                            << (element_type.has_value() ? "succeeded." : "failed."));
        
        // 移除 bounds check：索引是常量且在范围内，bounds check 是冗余的
        if (index_node->opcode() == IrOpcode::kCheckBounds ||
            index_node->opcode() == IrOpcode::kCheckedUint32Bounds ||
            index_node->opcode() == IrOpcode::kCheckedUint64Bounds) {
          // 找到原始的索引常量节点
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
      } else {
        TYPE_INJECTOR_DEBUG("  Tuple index " << index << " out of bounds [0, " << tuple_length << ")");
      }
    } else {
      TYPE_INJECTOR_DEBUG("  Failed to get constant index for Tuple");
      return;
    }
  } else {
    TYPE_INJECTOR_DEBUG("  Container type is neither Arr nor Tuple");
    return;
  }

  if (element_type.has_value()) {
    Type element_turbofan_type = TypeASTToType(element_type.value());
    TYPE_INJECTOR_DEBUG("Setting type for Node #" << node->id()
                                                   << " to: " << element_turbofan_type);
    NodeProperties::SetType(node, element_turbofan_type);
  }
}

// 移除 Tuple 的冗余 bounds check
// 当索引是常量且在 Tuple 长度范围内时，bounds check 是冗余的
void TypeInjector::RemoveTupleBoundsCheck(Node* load_element_node,
                                           Node* check_bounds_node,
                                           Node* index_constant) {
  // LoadElement 的输入结构：
  // [0]=elements (value), [1]=index (value, CheckBounds), [2]=effect (CheckBounds), [3]=control
  // CheckBounds 节点有 value 输出（检查后的索引）和 effect 输出
  
  // 检查 check_bounds_node 是否确实是 LoadElement 的输入
  Node* index_input = NodeProperties::GetValueInput(load_element_node, 1);
  Node* effect_input = NodeProperties::GetEffectInput(load_element_node);
  
  if (index_input != check_bounds_node && effect_input != check_bounds_node) {
    return;
  }
  
  TYPE_INJECTOR_DEBUG("Removing redundant bounds check for Tuple LoadElement #"
                      << load_element_node->id() << ", CheckBounds #"
                      << check_bounds_node->id() << ", using constant #"
                      << index_constant->id());
  
  // 替换 LoadElement 的索引输入（value input），用 index_constant
  if (index_input == check_bounds_node) {
    int value_index = NodeProperties::FirstValueIndex(load_element_node) + 1;
    load_element_node->ReplaceInput(value_index, index_constant);
  }
  
  // 替换 LoadElement 的 effect 输入，用 CheckBounds 的 effect 输入
  // 这样我们移除了 CheckBounds，但保留了 effect chain
  if (effect_input == check_bounds_node) {
    Node* check_bounds_effect_input = NodeProperties::GetEffectInput(check_bounds_node);
    int effect_index = NodeProperties::FirstEffectIndex(load_element_node);
    load_element_node->ReplaceInput(effect_index, check_bounds_effect_input);
  }
  
  // 注意：我们不移除 check_bounds_node 本身，因为它可能被其他节点使用
  // 后续的 DeadCodeElimination phase 会自动清理未使用的节点
}

void TypeInjector::ReplaceTupleLengthWithConstant(Node* load_field_node,
                                                    int tuple_length) {
  // JSArrayLength 返回 TaggedSigned (Smi)
  // 使用 NumberConstant，SimplifiedLowering 会将其转换为 TaggedSigned
  Node* constant_node = graph_->NewNode(common_->NumberConstant(tuple_length));
  
  // NumberConstant 的类型会自动设置
  // SimplifiedLowering 会将其转换为 TaggedSigned (Smi)
  
  TYPE_INJECTOR_DEBUG("Created Int32Constant #" << constant_node->id()
                                                 << " with value: " << tuple_length);
  
  // 替换 LoadField 节点的所有 value 使用
  // LoadField 的输出是 value，我们需要替换所有使用 LoadField 的节点
  // 但是，LoadField 也可能有 effect 和 control 输出，我们需要保留这些
  
  // 获取 LoadField 的 effect 输入（用于替换 effect 使用）
  Node* load_field_effect = NodeProperties::GetEffectInput(load_field_node);
  
  // 先收集所有的 value edge 和 effect edge，避免迭代器失效
  ZoneVector<std::pair<Node*, int>> value_edges(graph_->zone());
  ZoneVector<std::pair<Node*, int>> effect_edges(graph_->zone());
  for (Edge edge : load_field_node->use_edges()) {
    Node* use = edge.from();
    int index = edge.index();
    // 检查这是否是 value input
    if (index < NodeProperties::FirstEffectIndex(use)) {
      value_edges.push_back(std::make_pair(use, index));
    } else if (NodeProperties::IsEffectEdge(edge)) {
      // 这是 effect input，需要替换为 LoadField 的 effect 输入
      effect_edges.push_back(std::make_pair(use, index));
    }
  }
  
  // 替换所有 value 使用
  for (auto& pair : value_edges) {
    Node* use = pair.first;
    int index = pair.second;
    use->ReplaceInput(index, constant_node);
    TYPE_INJECTOR_DEBUG("Replaced value input #" << index << " of node #"
                                                 << use->id());
  }
  
  // 替换所有 effect 使用（用 LoadField 的 effect 输入替换）
  for (auto& pair : effect_edges) {
    Node* use = pair.first;
    int index = pair.second;
    use->ReplaceInput(index, load_field_effect);
    TYPE_INJECTOR_DEBUG("Replaced effect input #" << index << " of node #"
                                                  << use->id());
  }
  
  // 注意：LoadField 节点本身不会被删除，因为它可能还有 control 使用
  // 后续的 DeadCodeElimination phase 会自动清理未使用的节点
}

#if V8_COMPILER_TYPE_INJECTOR_DEBUG
void TypeInjector::InspectLoadFieldNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadField) return;

  FieldAccess const& access = FieldAccessOf(node->op());

  std::cout << "[TypeInjector] Inspecting Node #" << node->id()
            << " (LoadField):" << std::endl;
  std::cout << "  -> Offset: " << access.offset << std::endl;

  if (!access.name.is_null()) {
    Handle<Name> name_handle = access.name.ToHandleChecked();
    InstanceType type = name_handle->map()->instance_type();
    if (FIRST_STRING_TYPE <= type && type <= LAST_STRING_TYPE) {
      DirectHandle<String> str = Cast<String>(name_handle);
      std::cout << "  -> Name: " << str->ToCString().get() << std::endl;
    } else {
      std::cout << "  -> Name: <non-string>" << std::endl;
    }
  } else {
    std::cout << "  -> Name: <null>" << std::endl;
  }

  if (access.map.has_value()) {
    std::cout << "  -> Map: <present>" << std::endl;
  } else {
    std::cout << "  -> Map: <null>" << std::endl;
  }

  std::cout << "  -> Field Type: " << access.type << std::endl;
}

void TypeInjector::InspectLoadElementNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadElement) return;

  std::cout << "[TypeInjector] Inspecting Node #" << node->id()
            << " (LoadElement):" << std::endl;

  ElementAccess const& access = ElementAccessOf(node->op());

  std::cout << "  -> Element Type: " << access.type << std::endl;
  std::cout << "  -> Base Is Tagged: "
            << (access.base_is_tagged == kTaggedBase ? "true" : "false")
            << std::endl;
  std::cout << "  -> Header Size: " << access.header_size << std::endl;
  std::cout << "  -> Machine Type: " << access.machine_type << std::endl;
}
#endif

void TypeInjector::Run() {
  const std::string& script_hash = compilation_info_->cached_script_hash();
  TYPE_INJECTOR_DEBUG("script_hash=" << script_hash);

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
        TYPE_INJECTOR_DEBUG("Parameter #" << index << ": ");
        if (V8_COMPILER_TYPE_INJECTOR_DEBUG) {
        ast.Print();
        std::cout << std::endl;
        }
        NodeProperties::SetType(node, type);
      }
    }
  }

  // 第二次遍历：处理 LoadField 节点，注入字段类型
  for (Node* node : all.reachable) {
    ProcessLoadFieldNode(node);
#if V8_COMPILER_TYPE_INJECTOR_DEBUG
    InspectLoadFieldNode(node);
#endif
  }

  // 第三次遍历：处理 LoadElement 节点，注入元素类型
  for (Node* node : all.reachable) {
    ProcessLoadElementNode(node);
#if V8_COMPILER_TYPE_INJECTOR_DEBUG
    InspectLoadElementNode(node);
#endif
  }
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
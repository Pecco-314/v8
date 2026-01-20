#include "src/compiler/type-injector.h"

#include "src/compiler/all-nodes.h"
#include "src/compiler/common-operator.h"
#include "src/compiler/heap-refs.h"
#include "src/compiler/js-heap-broker.h"
#include "src/compiler/js-operator.h"
#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/simplified-operator.h"
#include "src/compiler/turbofan-types.h"
#include "src/objects/casting-inl.h"
#include "src/objects/heap-object-inl.h"
#include "src/objects/js-array.h"
#include "src/objects/map-inl.h"
#include "src/objects/shared-function-info-inl.h"
#include "src/zone/zone-containers.h"

namespace v8 {
namespace internal {
namespace compiler {

#ifndef V8_COMPILER_TYPE_INJECTOR_DEBUG
#define V8_COMPILER_TYPE_INJECTOR_DEBUG 0
#endif

#if V8_COMPILER_TYPE_INJECTOR_DEBUG
#include "src/base/logging.h"
#define TYPE_INJECTOR_DEBUG(...) \
  do { PrintF("[TypeInjector] " __VA_ARGS__); PrintF("\n"); } while (false)
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
    case TypeAST::Symbol:
      return Type::Symbol();
    case TypeAST::RawInt32:
      return Type::Signed32();
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

const TypeAST* TypeInjector::StoreOwnedTypeAST(const TypeAST& ast) {
  owned_typeasts_.push_back(ast);
  return &owned_typeasts_.back();
}

void TypeInjector::SetNodeType(Node* node, const TypeAST* type_ast) {
  if (type_ast == nullptr) return;
  node_type_map_[node] = type_ast;
}

const TypeAST* TypeInjector::GetNodeType(Node* node) {
  auto it = node_type_map_.find(node);
  if (it == node_type_map_.end()) return nullptr;
  return it->second;
}

// 从 Obj 类型 AST 中查找字段
std::optional<TypeAST> TypeInjector::FindFieldInObj(
    const TypeAST& obj_ast, const std::string& field_name) {
  if (obj_ast.kind != TypeAST::Obj) {
    return std::nullopt;
  }

  // Obj 的 children 中每一项都有 field_name
  for (const auto& child : obj_ast.children) {
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
  if (const TypeAST* cached = GetNodeType(node)) {
    return *cached;
  }
  if (node->opcode() == IrOpcode::kParameter) {
    int param_index = ParameterIndexOf(node->op());
    if (param_index >= 0 &&
        param_index < static_cast<int>(param_types_.size())) {
      SetNodeType(node, &param_types_[param_index]);
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
    if (object_type.kind != TypeAST::Obj) {
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

      // 从 Obj 中查找该字段的类型
      return FindFieldInObj(object_type, field_name);
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
  } else if (node->opcode() == IrOpcode::kChangeTaggedToInt32) {
    // RawInt32 经过 ChangeTaggedToInt32 后保持类型标签，便于后续消除 Checked* 节点
    auto input_type_opt = GetNodeTypeAST(node->InputAt(0));
    if (input_type_opt.has_value() && input_type_opt->kind == TypeAST::RawInt32) {
      const TypeAST* input_ptr = StoreOwnedTypeAST(input_type_opt.value());
      SetNodeType(node, input_ptr);
      NodeProperties::SetType(node, Type::Signed32());
      return input_type_opt;
    }
  } else if (node->opcode() == IrOpcode::kNumberAdd ||
             node->opcode() == IrOpcode::kNumberSubtract ||
             node->opcode() == IrOpcode::kNumberMultiply ||
             node->opcode() == IrOpcode::kNumberDivide ||
             node->opcode() == IrOpcode::kNumberModulus) {
    // Number* 节点：若双输入都是 RawInt32，则将当前节点标记为 RawInt32，便于后续继续替换/消除
    auto left_type_opt = GetNodeTypeAST(node->InputAt(0));
    auto right_type_opt = GetNodeTypeAST(node->InputAt(1));
    if (left_type_opt.has_value() && right_type_opt.has_value() &&
        left_type_opt->kind == TypeAST::RawInt32 &&
        right_type_opt->kind == TypeAST::RawInt32) {
      const TypeAST* left_ptr = StoreOwnedTypeAST(left_type_opt.value());
      SetNodeType(node, left_ptr);
      NodeProperties::SetType(node, Type::Signed32());
      return left_type_opt;
    }
  } else if (node->opcode() == IrOpcode::kInt32Add ||
             node->opcode() == IrOpcode::kInt32Sub ||
             node->opcode() == IrOpcode::kInt32Mul ||
             node->opcode() == IrOpcode::kInt32Div ||
             node->opcode() == IrOpcode::kInt32Mod) {
    // 纯 Int32 算术节点：若输入都是 RawInt32，写入类型便于链式传播
    auto left_type_opt = GetNodeTypeAST(node->InputAt(0));
    auto right_type_opt = GetNodeTypeAST(node->InputAt(1));
    if (left_type_opt.has_value() && right_type_opt.has_value() &&
        left_type_opt->kind == TypeAST::RawInt32 &&
        right_type_opt->kind == TypeAST::RawInt32) {
      const TypeAST* left_ptr = StoreOwnedTypeAST(left_type_opt.value());
      SetNodeType(node, left_ptr);
      NodeProperties::SetType(node, Type::Signed32());
      return left_type_opt;
    }
  } else if (node->opcode() == IrOpcode::kPhi) {
    // 循环累加场景下，Phi 应该继承 RawInt32 标签，避免后续重新落回 Speculative*
    bool all_raw = true;
    const TypeAST* first_raw = nullptr;
    int value_count = node->op()->ValueInputCount();
    for (int i = 0; i < value_count; ++i) {
      Node* input = NodeProperties::GetValueInput(node, i);
      auto input_type_opt = GetNodeTypeAST(input);
      bool is_raw = input_type_opt.has_value() &&
                    input_type_opt->kind == TypeAST::RawInt32;
      if (!is_raw) {
        Type t = NodeProperties::GetType(input);
        is_raw = t.Is(Type::Signed32());
      }
      if (!is_raw) {
        all_raw = false;
        break;
      }
      if (first_raw == nullptr && input_type_opt.has_value()) {
        first_raw = StoreOwnedTypeAST(input_type_opt.value());
      }
    }

    if (all_raw) {
      if (first_raw == nullptr) {
        TypeAST fallback(TypeAST::RawInt32);
        first_raw = StoreOwnedTypeAST(fallback);
      }
      SetNodeType(node, first_raw);
      NodeProperties::SetType(node, Type::Signed32());
      return *first_raw;
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
      // 注入阶段仅记录类型，不做替换
      if (object_type.kind == TypeAST::Tuple) {
        const TypeAST* len_ptr = StoreOwnedTypeAST(object_type);  // reuse object type pointer
        SetNodeType(node, len_ptr);
      }
    }
  }

  // 处理有字段名的 LoadField（Obj 字段访问）
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
  TYPE_INJECTOR_DEBUG("Processing LoadField for field: %s", field_name.c_str());

  // 获取 LoadField 的输入节点（object）
  Node* object_node = NodeProperties::GetValueInput(node, 0);

  // 递归获取对象的类型
  auto object_type_opt = GetNodeTypeAST(object_node);
  if (!object_type_opt.has_value()) {
    return;
  }

  const TypeAST& object_type = object_type_opt.value();
  if (object_type.kind != TypeAST::Obj) {
    return;
  }

  // 从 Obj 中查找字段
  auto field_type = FindFieldInObj(object_type, field_name);
  TYPE_INJECTOR_DEBUG("Field '%s' lookup %s", 
                      field_name.c_str(),
                      field_type.has_value() ? "succeeded." : "failed.");
  if (field_type.has_value()) {
    const TypeAST* field_ptr = StoreOwnedTypeAST(field_type.value());
    Type field_turbofan_type = TypeASTToType(*field_ptr);
    TYPE_INJECTOR_DEBUG("Setting type for Node #%d to: %s", 
                        node->id(), 
                        field_turbofan_type.ToString().c_str());
    NodeProperties::SetType(node, field_turbofan_type);
    SetNodeType(node, field_ptr);
  }
}

// 处理 LoadElement 节点的类型注入
// 支持 Array 和 Tuple 两种类型
void TypeInjector::ProcessLoadElementNode(Node* node) {
  if (node->opcode() != IrOpcode::kLoadElement) return;

  TYPE_INJECTOR_DEBUG("Processing LoadElement for Node #%d", node->id());

  // 获取 LoadElement 的输入节点（array/tuple elements）
  // LoadElement 的输入通常是：[0] = elements buffer, [1] = index
  // 在 IR 中，elements buffer 通常来自 LoadField 操作
  Node* elements_node = node->InputAt(0);
  TYPE_INJECTOR_DEBUG("  Input[0]: Node #%d (%s)", 
                      elements_node->id(),
                      elements_node->op()->mnemonic());

  // 如果 elements_node 是 LoadField，需要找到真正的数组/元组对象
  Node* array_node = elements_node;
  if (elements_node->opcode() == IrOpcode::kLoadField) {
    // 这是 LoadField，其输入应该是数组/元组对象
    array_node = elements_node->InputAt(0);
    TYPE_INJECTOR_DEBUG("  Following LoadField to Node #%d (%s)",
                        array_node->id(), 
                        array_node->op()->mnemonic());
  }

  // 递归获取数组/元组的类型
  auto container_type_opt = GetNodeTypeAST(array_node);
  if (!container_type_opt.has_value()) {
    TYPE_INJECTOR_DEBUG("  Failed to get type for container node");
    return;
  }

  const TypeAST& container_type = container_type_opt.value();
  TYPE_INJECTOR_DEBUG("  Container type: %s", container_type.KindToString().c_str());

  std::optional<TypeAST> element_type;

  if (container_type.kind == TypeAST::Arr) {
    // Array: 所有元素类型相同
    element_type = GetElementTypeInArray(container_type);
    TYPE_INJECTOR_DEBUG("  Array element type lookup %s",
                        element_type.has_value() ? "succeeded." : "failed.");
  } else if (container_type.kind == TypeAST::Tuple) {
    // Tuple: 每个位置类型不同，需要获取索引
    Node* index_node = NodeProperties::GetValueInput(node, 1);
    auto index_opt = TryGetConstantIndex(index_node);

    if (index_opt.has_value()) {
      int index = index_opt.value();
      int tuple_length = static_cast<int>(container_type.children.size());
      TYPE_INJECTOR_DEBUG("  Tuple index: %d, length: %d", index, tuple_length);
      
      // 检查索引是否在 Tuple 长度范围内
      if (index >= 0 && index < tuple_length) {
        element_type = GetElementTypeInTuple(container_type, index);
        TYPE_INJECTOR_DEBUG("  Tuple element type lookup %s",
                            element_type.has_value() ? "succeeded." : "failed.");
      } else {
        TYPE_INJECTOR_DEBUG("  Tuple index %d out of bounds [0, %d)", index, tuple_length);
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
    const TypeAST* elem_ptr = StoreOwnedTypeAST(element_type.value());
    Type element_turbofan_type = TypeASTToType(*elem_ptr);
    TYPE_INJECTOR_DEBUG("Setting type for Node #%d to: %s",
                        node->id(),
                        element_turbofan_type.ToString().c_str());
    NodeProperties::SetType(node, element_turbofan_type);
    SetNodeType(node, elem_ptr);
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
  
  TYPE_INJECTOR_DEBUG("Removing redundant bounds check for Tuple LoadElement #%d, CheckBounds #%d, using constant #%d",
                      load_element_node->id(),
                      check_bounds_node->id(),
                      index_constant->id());

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
  
  TYPE_INJECTOR_DEBUG("Created Int32Constant #%d with value: %d",
                      constant_node->id(),
                      tuple_length);
  
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
    TYPE_INJECTOR_DEBUG("Replaced value input #%d of node #%d",
                        index,
                        use->id());
  }

  // 替换所有 effect 使用（用 LoadField 的 effect 输入替换）
  for (auto& pair : effect_edges) {
    Node* use = pair.first;
    int index = pair.second;
    use->ReplaceInput(index, load_field_effect);
    TYPE_INJECTOR_DEBUG("Replaced effect input #%d of node #%d",
                        index,
                        use->id());
}

  // 注意：LoadField 节点本身不会被删除，因为它可能还有 control 使用
  // 后续的 DeadCodeElimination phase 会自动清理未使用的节点
}

// 处理 JSCall 节点，注入返回值类型
void TypeInjector::ProcessJSCallNode(Node* node) {
  if (node->opcode() != IrOpcode::kJSCall) return;

  // 获取 target（被调用的函数）
  Node* target = NodeProperties::GetValueInput(node, 0);
  
  // 检查 target 是否是 HeapConstant (JSFunction)
  if (target->opcode() != IrOpcode::kHeapConstant) {
    return;
  }

  // 获取 target 的类型
  Type target_type = NodeProperties::GetType(target);
  if (!target_type.IsHeapConstant()) {
    return;
  }

  HeapObjectRef target_ref = target_type.AsHeapConstant()->Ref();
  if (!target_ref.IsJSFunction()) {
    return;
  }

  // 获取 SharedFunctionInfo
  JSFunctionRef function = target_ref.AsJSFunction();
  OptionalSharedFunctionInfoRef shared_opt = function.shared(broker_);
  if (!shared_opt.has_value()) {
    return;
  }

  SharedFunctionInfoRef shared = shared_opt.value();
  
  // 1. 尝试从内建函数类型表获取完整签名（基于 Builtin ID）
  if (v8_flags.turbo_builtin_type_table && shared.HasBuiltinId()) {
    Builtin builtin_id = shared.builtin_id();
    TYPE_INJECTOR_DEBUG("Found builtin: %s (id=%d)",
                        Builtins::name(builtin_id),
                        static_cast<int>(builtin_id));
    
    auto* storage = TypeStorage::Get();
    auto signature = storage->GetBuiltinSignature(builtin_id);
    
    if (signature.has_value()) {
      // 设置返回值类型
      Type return_turbofan_type = TypeASTToType(signature->return_type);
      NodeProperties::SetType(node, return_turbofan_type);
      TYPE_INJECTOR_DEBUG("Applied builtin return type for %s -> %s",
                          Builtins::name(builtin_id),
                          return_turbofan_type.ToString().c_str());
      
      // 设置接收者和参数类型
      JSCallNode call_node(node);
      Node* receiver = call_node.receiver();
      
      // 参数类型列表：第一个是接收者，后续是其他参数
      const auto& param_types = signature->param_types;
      
      // 设置接收者类型（如果有）
      if (!param_types.empty() && receiver != nullptr) {
        Type receiver_type = TypeASTToType(param_types[0]);
        NodeProperties::SetType(receiver, receiver_type);
        TYPE_INJECTOR_DEBUG("  Set receiver type: %s", receiver_type.ToString().c_str());
      }
      
      // 设置其他参数类型
      size_t arg_count = call_node.ArgumentCount();
      for (size_t i = 0; i < arg_count && i + 1 < param_types.size(); ++i) {
        Node* arg = NodeProperties::GetValueInput(node, static_cast<int>(2 + i));  // 跳过 target 和 receiver
        if (arg != nullptr) {
          Type arg_type = TypeASTToType(param_types[i + 1]);
          NodeProperties::SetType(arg, arg_type);
          TYPE_INJECTOR_DEBUG("  Set arg[%zu] type: %s", i, arg_type.ToString().c_str());
        }
      }
      
      return;
    } else {
      TYPE_INJECTOR_DEBUG("  No builtin signature found in metadata");
    }
  }
  
  // 2. 尝试从用户 metadata 获取返回值类型（基于 bytecode offset）
  int start_pos = shared.StartPosition();

  TYPE_INJECTOR_DEBUG("Processing JSCall #%d to function at position %d",
                      node->id(),
                      start_pos);

  // 获取函数的返回值类型
  auto return_type_opt = GetFunctionReturnType(start_pos);
  if (!return_type_opt.has_value()) {
    TYPE_INJECTOR_DEBUG("  No return type metadata found");
    return;
  }

  const TypeAST& return_type_ast = return_type_opt.value();
  const TypeAST* ret_ptr = StoreOwnedTypeAST(return_type_ast);
  Type return_type = TypeASTToType(*ret_ptr);

  TYPE_INJECTOR_DEBUG("  Setting return type: ");
  if (V8_COMPILER_TYPE_INJECTOR_DEBUG) {
    return_type_ast.PrintLn();
  }

  // 设置 JSCall 节点的返回值类型
  NodeProperties::SetType(node, return_type);
  SetNodeType(node, ret_ptr);
}


// 获取函数的返回值类型
std::optional<TypeAST> TypeInjector::GetFunctionReturnType(int start_pos) {
  auto typemap = storage_->GetTypeMap(script_hash_);
  auto it = typemap.find(start_pos);
  if (it != typemap.end() && !it->second.empty()) {
    // 返回值类型存储在参数列表的最后一个位置
    return it->second.back();
  }
  return std::nullopt;
}

// 处理 RawInt32 类型的二元操作（加/减/乘/除/模）
// 基于注入的 TypeAST 决策，不再写 Type
bool TypeInjector::ProcessRawInt32BinaryOp(Node* node) {
  // 如果没有 metadata（param_types_ 为空），直接跳过，避免误优化
  if (param_types_.empty()) return false;

    // 只处理 SpeculativeSmallIntegerAdd/Subtract、SpeculativeNumberAdd/Subtract
    // 以及 SpeculativeNumberMultiply/Divide/Modulus
  IrOpcode::Value opcode = node->opcode();
  
  if (opcode != IrOpcode::kSpeculativeSmallIntegerAdd &&
      opcode != IrOpcode::kSpeculativeSmallIntegerSubtract &&
      opcode != IrOpcode::kSpeculativeNumberAdd &&
      opcode != IrOpcode::kSpeculativeNumberSubtract &&
      opcode != IrOpcode::kSpeculativeNumberMultiply &&
      opcode != IrOpcode::kSpeculativeNumberDivide &&
      opcode != IrOpcode::kSpeculativeNumberModulus &&
      opcode != IrOpcode::kInt32Add &&
      opcode != IrOpcode::kInt32Sub &&
      opcode != IrOpcode::kInt32Mul &&
      opcode != IrOpcode::kInt32Div &&
      opcode != IrOpcode::kInt32Mod &&
      opcode != IrOpcode::kCheckedInt32Add &&
      opcode != IrOpcode::kCheckedInt32Sub &&
      opcode != IrOpcode::kCheckedInt32Mul &&
      opcode != IrOpcode::kCheckedInt32Div &&
      opcode != IrOpcode::kCheckedInt32Mod) {
    return false;
  }

  TYPE_INJECTOR_DEBUG("rawint32 bin op id=%d opcode=%d (%s) start_pos=%d",
                       node->id(), static_cast<int>(opcode),
                       node->op()->mnemonic(), current_start_pos_);

  // 若本函数的参数全部为 RawInt32，则允许无条件地视为 RawInt32 运算，避免遗漏链式传播
  bool fn_all_rawint32 = true;
  if (param_types_.empty()) {
    fn_all_rawint32 = false;
  } else {
    // 最后一位是返回类型
    for (size_t i = 0; i + 1 < param_types_.size(); ++i) {
      if (param_types_[i].kind != TypeAST::RawInt32) {
        fn_all_rawint32 = false;
        break;
      }
    }
  }

  // 检查两个输入是否都是 RawInt32 类型（映射到 Signed32）
  Node* left = NodeProperties::GetValueInput(node, 0);
  Node* right = NodeProperties::GetValueInput(node, 1);
  auto left_ast_opt = GetNodeTypeAST(left);
  auto right_ast_opt = GetNodeTypeAST(right);

  // 允许通过 Turbofan Type 来兜底识别 RawInt32（参数已被标为 Signed32）
  bool left_is_raw = left_ast_opt.has_value() && left_ast_opt->kind == TypeAST::RawInt32;
  bool right_is_raw = right_ast_opt.has_value() && right_ast_opt->kind == TypeAST::RawInt32;
  if (!left_is_raw) {
    Type left_type = NodeProperties::GetType(left);
    left_is_raw = left_type.Is(Type::Signed32());
  }
  if (!right_is_raw) {
    Type right_type = NodeProperties::GetType(right);
    right_is_raw = right_type.Is(Type::Signed32());
  }

  if (!left_is_raw || !right_is_raw) {
    if (!fn_all_rawint32) return false;
    // 函数级 RawInt32，缺少标记也继续替换
    left_is_raw = right_is_raw = true;
  }

  // 确保我们给新节点留下 RawInt32 的 TypeAST 标签
  const TypeAST* left_ast = nullptr;
  if (left_ast_opt.has_value()) {
    left_ast = StoreOwnedTypeAST(left_ast_opt.value());
  } else if (right_ast_opt.has_value()) {
    // 尽量复用右侧的 RawInt32 标签以保持结构一致
    left_ast = StoreOwnedTypeAST(right_ast_opt.value());
  } else {
    TypeAST fallback(TypeAST::RawInt32);
    left_ast = StoreOwnedTypeAST(fallback);
  }

  // 纯 Int32* 运算：仅写入类型标签以便后续节点继续识别为 RawInt32
  if (opcode == IrOpcode::kInt32Add || opcode == IrOpcode::kInt32Sub ||
      opcode == IrOpcode::kInt32Mul || opcode == IrOpcode::kInt32Div ||
      opcode == IrOpcode::kInt32Mod) {
    NodeProperties::SetType(node, Type::Signed32());
    SetNodeType(node, left_ast);
    return false;
  }

  TYPE_INJECTOR_DEBUG("Found RawInt32 binary operation, replacing with Number op");

  // SpeculativeSmallInteger/Number* 和 CheckedInt32* 有 effect/control，
  // Number* 是纯操作，只需要 value 输入。需要新节点来移除 effect/control。
  
  // 根据操作类型创建对应的 Number 操作节点
  const Operator* number_op = nullptr;
  if (opcode == IrOpcode::kSpeculativeSmallIntegerAdd ||
      opcode == IrOpcode::kSpeculativeNumberAdd ||
      opcode == IrOpcode::kCheckedInt32Add ||
      opcode == IrOpcode::kInt32Add) {
    number_op = simplified_->NumberAdd();
  } else if (opcode == IrOpcode::kSpeculativeSmallIntegerSubtract ||
             opcode == IrOpcode::kSpeculativeNumberSubtract ||
             opcode == IrOpcode::kCheckedInt32Sub ||
             opcode == IrOpcode::kInt32Sub) {
    number_op = simplified_->NumberSubtract();
  } else if (opcode == IrOpcode::kSpeculativeNumberMultiply ||
             opcode == IrOpcode::kCheckedInt32Mul ||
             opcode == IrOpcode::kInt32Mul) {
    number_op = simplified_->NumberMultiply();
  } else if (opcode == IrOpcode::kSpeculativeNumberDivide ||
             opcode == IrOpcode::kCheckedInt32Div ||
             opcode == IrOpcode::kInt32Div) {
    number_op = simplified_->NumberDivide();
  } else {
    number_op = simplified_->NumberModulus();
  }

  Node* number_node = graph_->NewNode(number_op, left, right);
  
  // 设置结果类型为 Signed32，确保 SimplifiedLowering 生成 Int32Add/Sub
  NodeProperties::SetType(number_node, Type::Signed32());
  // 保持 RawInt32 的类型标签，便于后续链式运算继续走 RawInt32 快路径
  SetNodeType(number_node, left_ast);
  
  // 获取原节点的 effect 和 control 输入（用于替换）
  Node* effect_input = NodeProperties::GetEffectInput(node);
  Node* control_input = NodeProperties::GetControlInput(node);
  
  // 替换所有使用：
  // - value 输出 -> number_node
  // - effect 输出 -> effect_input（直通）
  // - control 输出 -> control_input（直通）
  NodeProperties::ReplaceUses(node, number_node, effect_input, control_input, control_input);
  
  // 删除原节点
    node->Kill();
    TYPE_INJECTOR_DEBUG("replaced node id=%d (%s) with Number* (id=%d) start_pos=%d",
                         node->id(), node->op()->mnemonic(), number_node->id(),
                         current_start_pos_);
  TYPE_INJECTOR_DEBUG("Changed to Number* successfully");
  return true;
}

// Tuple length 常量化（使用已注入的 TypeAST 信息）
void TypeInjector::OptimizeTupleLength(Node* node) {
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

// LoadElement bounds 消除，依赖注入的 TypeAST 信息
void TypeInjector::OptimizeLoadElementBounds(Node* node) {
  if (node->opcode() != IrOpcode::kLoadElement) return;

  Node* elements_node = node->InputAt(0);
  Node* array_node = elements_node;
  if (elements_node->opcode() == IrOpcode::kLoadField) {
    array_node = elements_node->InputAt(0);
  }

  auto container_type_opt = GetNodeTypeAST(array_node);
  if (!container_type_opt.has_value()) return;

  const TypeAST& container_type = container_type_opt.value();
  if (container_type.kind != TypeAST::Tuple && container_type.kind != TypeAST::Arr) return;

  Node* index_node = NodeProperties::GetValueInput(node, 1);
  auto index_opt = TryGetConstantIndex(index_node);
  if (!index_opt.has_value()) return;

  int index = index_opt.value();
  int length = 0;
  if (container_type.kind == TypeAST::Tuple) {
    length = static_cast<int>(container_type.children.size());
  } else {
    // Array 不确定长度，只有 Tuple 时能确定
    return;
  }

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

void TypeInjector::ProcessCheckMapsNode(Node* node) {
  if (node->opcode() != IrOpcode::kCheckMaps) return;

  Node* value_input = NodeProperties::GetValueInput(node, 0);
  const TypeAST* type = GetNodeType(value_input);
  if (type != nullptr && type->kind == TypeAST::Symbol) {
    TYPE_INJECTOR_DEBUG("Removing CheckMaps for Symbol input");

    Node* effect_input = NodeProperties::GetEffectInput(node);
    Node* control_input = NodeProperties::GetControlInput(node);
    
    NodeProperties::ReplaceUses(node, nullptr, effect_input, control_input, control_input);
    node->Kill();
  }
}

void TypeInjector::Run() {
  script_hash_ = compilation_info_->cached_script_hash();

  IndirectHandle<SharedFunctionInfo> shared = compilation_info_->shared_info();
  int start_pos = shared->StartPosition();
  current_start_pos_ = start_pos;

  TYPE_INJECTOR_DEBUG("Run() called - script_hash: %s, start_pos: %d", 
                      script_hash_.c_str(), start_pos);

  storage_ = TypeStorage::Get();
  auto typemap = storage_->GetTypeMap(script_hash_);

  TYPE_INJECTOR_DEBUG("Found %zu entries in typemap", typemap.size());

  auto it = typemap.find(start_pos);
  if (it != typemap.end()) {
    param_types_ = it->second;
  } else {
    // 仅打印轻量日志，避免依赖 Type::ToString
    TYPE_INJECTOR_DEBUG("no metadata for script_hash=%s start_pos=%d",
                         script_hash_.c_str(), start_pos);
  }
  if (!param_types_.empty()) {
    TYPE_INJECTOR_DEBUG("metadata ok: start_pos=%d params=%zu", start_pos,
                         param_types_.size());
  }

  AllNodes all(graph_->zone(), graph_);
  // Phase A: 类型注入（写 Type + Tag，不做替换）
  RunTypeAnnotation(all);

  // Phase B: 定制消除/替换（只读 Tag，不写 Type）
  RunCustomElimination(all);
}

// Phase A: 类型注入
void TypeInjector::RunTypeAnnotation(AllNodes& all) {
  // 参数类型注入
  for (Node* node : all.reachable) {
    if (node->opcode() == IrOpcode::kParameter) {
      int index = ParameterIndexOf(node->op());
      if (index >= 0 && index < static_cast<int>(param_types_.size()) - 1) {
        const TypeAST& ast = param_types_[index];
        SetNodeType(node, &ast);
        Type type = TypeASTToType(ast);
        NodeProperties::SetType(node, type);
      }
    }
  }

  // LoadField 注入
  for (Node* node : all.reachable) {
    ProcessLoadFieldNode(node);
  }

  // LoadElement 注入
  for (Node* node : all.reachable) {
    ProcessLoadElementNode(node);
  }

  // JSCall 返回值注入
  for (Node* node : all.reachable) {
    ProcessJSCallNode(node);
  }
}

// Phase B: 定制消除/替换
void TypeInjector::RunCustomElimination(AllNodes& all) {
  // Tuple length 常量化
  for (Node* node : all.reachable) {
    OptimizeTupleLength(node);
  }

  // LoadElement bounds 消除
  for (Node* node : all.reachable) {
    OptimizeLoadElementBounds(node);
  }

  // RawInt32 算术替换：迭代直到无新替换，确保链式累加完全转换
  int iteration = 0;
  bool changed = false;
  do {
    changed = false;
    AllNodes fresh(graph_->zone(), graph_);
    TYPE_INJECTOR_DEBUG("RawInt32 pass #%d start_pos=%d nodes=%zu", iteration,
                         current_start_pos_, fresh.reachable.size());
    for (Node* node : fresh.reachable) {
      changed |= ProcessRawInt32BinaryOp(node);
    }
    iteration++;
  } while (changed && iteration < 32);

  // CheckMaps 消除
  for (Node* node : all.reachable) {
    ProcessCheckMapsNode(node);
  }
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
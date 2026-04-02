#include "src/compiler/metadata-type-helper.h"

#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/opcodes.h"
#include "src/compiler/simplified-operator.h"
#include "src/objects/casting-inl.h"
#include "src/objects/heap-object-inl.h"
#include "src/objects/map-inl.h"
#include "src/objects/name.h"

#include <unordered_set>

namespace v8::internal::compiler {

#if V8_COMPILER_TYPE_INJECTOR_DEBUG
#include "src/base/logging.h"
#define TYPE_HELPER_DEBUG(...) \
  do { PrintF("[MetadataTypeHelper] " __VA_ARGS__); PrintF("\n"); } while (false)
#else
#define TYPE_HELPER_DEBUG(...) ((void)0)
#endif

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

bool MetadataTypeHelper::IsRawProofType(const TypeAST& ast) const {
  return ast.kind == TypeAST::RawInt32 || ast.kind == TypeAST::RawUint32 ||
         ast.kind == TypeAST::RawInt64 || ast.kind == TypeAST::RawUint64;
}

bool MetadataTypeHelper::ShouldSetTurboFanType(const TypeAST& ast) const {
  return !IsRawProofType(ast);
}

void MetadataTypeHelper::AnnotateNode(Node* node, const TypeAST* type_ast) {
  if (node == nullptr || type_ast == nullptr) return;
  SetNodeType(node, type_ast);
  if (ShouldSetTurboFanType(*type_ast)) {
    NodeProperties::SetType(node, TypeASTToType(*type_ast));
  }
}

void MetadataTypeHelper::AnnotateNode(Node* node, const TypeAST& ast) {
  const TypeAST* stored = StoreOwnedTypeAST(ast);
  AnnotateNode(node, stored);
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
    case TypeAST::RawUint32:
      return Type::Any();
    case TypeAST::RawInt64:
      return Type::Any();
    case TypeAST::RawUint64:
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
  if (node == nullptr) {
    return std::nullopt;
  }

  static thread_local std::unordered_set<Node*>* visiting_nodes =
      new std::unordered_set<Node*>();
  if (visiting_nodes->find(node) != visiting_nodes->end()) {
    return std::nullopt;
  }
  struct VisitingGuard {
    explicit VisitingGuard(std::unordered_set<Node*>* set, Node* current)
        : set_ref(set), current_node(current) {
      set_ref->insert(current_node);
    }
    ~VisitingGuard() { set_ref->erase(current_node); }

    std::unordered_set<Node*>* set_ref;
    Node* current_node;
  } guard(visiting_nodes, node);

  if (const TypeAST* cached = GetNodeType(node)) {
    return *cached;
  }

  auto cache_and_return = [&](const TypeAST& ast) -> std::optional<TypeAST> {
    const TypeAST* stored = StoreOwnedTypeAST(ast);
    SetNodeType(node, stored);
    return *stored;
  };

  auto make_primitive_ast = [](TypeAST::TypeKind kind) {
    TypeAST ast;
    ast.kind = kind;
    return ast;
  };

  auto is_raw_kind = [](TypeAST::TypeKind kind) {
    return kind == TypeAST::RawInt32 || kind == TypeAST::RawUint32 ||
           kind == TypeAST::RawInt64 || kind == TypeAST::RawUint64;
  };

  if (node->opcode() == IrOpcode::kParameter) {
    int param_index = ParameterIndexOf(node->op());
    auto& param_types = context_.param_types();
    if (param_index >= 0 &&
        param_index < static_cast<int>(param_types.size())) {
      SetNodeType(node, &param_types[param_index]);
      return param_types[param_index];
    }
  } else if (node->opcode() == IrOpcode::kInt32Constant ||
             node->opcode() == IrOpcode::kInt64Constant ||
             node->opcode() == IrOpcode::kFloat32Constant ||
             node->opcode() == IrOpcode::kFloat64Constant ||
             node->opcode() == IrOpcode::kNumberConstant) {
    return cache_and_return(make_primitive_ast(TypeAST::Num));
  } else if (node->opcode() == IrOpcode::kHeapConstant) {
    Type constant_type = NodeProperties::GetType(node);
    if (constant_type.Is(Type::String())) {
      return cache_and_return(make_primitive_ast(TypeAST::Str));
    }
    if (constant_type.Is(Type::Boolean())) {
      return cache_and_return(make_primitive_ast(TypeAST::Bool));
    }
    if (constant_type.Is(Type::Symbol())) {
      return cache_and_return(make_primitive_ast(TypeAST::Symbol));
    }
    if (constant_type.Is(Type::BigInt())) {
      return cache_and_return(make_primitive_ast(TypeAST::BigInt));
    }
    return cache_and_return(make_primitive_ast(TypeAST::Any));
  } else if (node->opcode() == IrOpcode::kTypeGuard ||
             node->opcode() == IrOpcode::kMapGuard ||
             node->opcode() == IrOpcode::kCheckHeapObject ||
             node->opcode() == IrOpcode::kCheckMaps ||
             node->opcode() == IrOpcode::kCheckInternalizedString ||
             node->opcode() == IrOpcode::kCheckNotTaggedHole ||
             node->opcode() == IrOpcode::kCheckNumber ||
             node->opcode() == IrOpcode::kCheckReceiver ||
             node->opcode() == IrOpcode::kCheckReceiverOrNullOrUndefined ||
             node->opcode() == IrOpcode::kCheckSmi ||
             node->opcode() == IrOpcode::kCheckString ||
             node->opcode() == IrOpcode::kCheckStringOrStringWrapper ||
             node->opcode() == IrOpcode::kCheckSymbol ||
             node->opcode() == IrOpcode::kCheckBigInt ||
             node->opcode() == IrOpcode::kCheckedTaggedToTaggedPointer ||
             node->opcode() == IrOpcode::kConvertTaggedHoleToUndefined) {
    if (node->op()->ValueInputCount() == 0) {
      return std::nullopt;
    }
    auto input_type_opt = GetNodeTypeAST(NodeProperties::GetValueInput(node, 0));
    if (!input_type_opt.has_value()) {
      return std::nullopt;
    }
    return cache_and_return(input_type_opt.value());
  } else if (node->opcode() == IrOpcode::kCheckedTaggedSignedToInt32 ||
             node->opcode() == IrOpcode::kCheckedTaggedToInt32 ||
             node->opcode() == IrOpcode::kChangeInt32ToTagged ||
             node->opcode() == IrOpcode::kChangeUint32ToTagged ||
             node->opcode() == IrOpcode::kChangeInt31ToTaggedSigned) {
    if (node->op()->ValueInputCount() == 0) {
      return std::nullopt;
    }
    auto input_type_opt = GetNodeTypeAST(NodeProperties::GetValueInput(node, 0));
    if (!input_type_opt.has_value()) {
      return std::nullopt;
    }
    if (!is_raw_kind(input_type_opt->kind)) {
      return std::nullopt;
    }
    return cache_and_return(input_type_opt.value());
  } else if (node->opcode() == IrOpcode::kInt32Add ||
             node->opcode() == IrOpcode::kInt32Sub ||
             node->opcode() == IrOpcode::kInt32Mul ||
             node->opcode() == IrOpcode::kCheckedInt32Add ||
             node->opcode() == IrOpcode::kCheckedInt32Sub ||
             node->opcode() == IrOpcode::kCheckedInt32Mul ||
             node->opcode() == IrOpcode::kSpeculativeSmallIntegerAdd ||
             node->opcode() == IrOpcode::kSpeculativeSmallIntegerSubtract) {
    if (node->op()->ValueInputCount() < 2) {
      return std::nullopt;
    }

    Node* left = NodeProperties::GetValueInput(node, 0);
    Node* right = NodeProperties::GetValueInput(node, 1);
    auto left_type_opt = GetNodeTypeAST(left);
    auto right_type_opt = GetNodeTypeAST(right);

    auto left_is_raw = left_type_opt.has_value() && is_raw_kind(left_type_opt->kind);
    auto right_is_raw = right_type_opt.has_value() && is_raw_kind(right_type_opt->kind);

    if (left_is_raw && right_is_raw && left_type_opt->kind == right_type_opt->kind) {
      TYPE_HELPER_DEBUG("start_pos=%d node#%d op=%s inferred raw-raw kind=%d",
                        context_.current_start_pos(), node->id(),
                        IrOpcode::Mnemonic(node->opcode()),
                        static_cast<int>(left_type_opt->kind));
      return cache_and_return(left_type_opt.value());
    }

    bool right_is_const = TryGetConstantIndex(right).has_value();
    bool left_is_const = TryGetConstantIndex(left).has_value();
    if (left_is_raw && right_is_const) {
      TYPE_HELPER_DEBUG("start_pos=%d node#%d op=%s inferred raw+const kind=%d",
                        context_.current_start_pos(), node->id(),
                        IrOpcode::Mnemonic(node->opcode()),
                        static_cast<int>(left_type_opt->kind));
      return cache_and_return(left_type_opt.value());
    }
    if (right_is_raw && left_is_const) {
      TYPE_HELPER_DEBUG("start_pos=%d node#%d op=%s inferred const+raw kind=%d",
                        context_.current_start_pos(), node->id(),
                        IrOpcode::Mnemonic(node->opcode()),
                        static_cast<int>(right_type_opt->kind));
      return cache_and_return(right_type_opt.value());
    }

    TYPE_HELPER_DEBUG(
        "start_pos=%d node#%d op=%s no-raw-proof left=%d right=%d left_const=%d right_const=%d",
        context_.current_start_pos(), node->id(), IrOpcode::Mnemonic(node->opcode()),
        left_type_opt.has_value() ? static_cast<int>(left_type_opt->kind) : -1,
        right_type_opt.has_value() ? static_cast<int>(right_type_opt->kind) : -1,
        left_is_const ? 1 : 0, right_is_const ? 1 : 0);

    return std::nullopt;
  } else if (node->opcode() == IrOpcode::kPhi) {
    int value_input_count = node->op()->ValueInputCount();
    if (value_input_count <= 0) {
      return std::nullopt;
    }

    std::optional<TypeAST::TypeKind> candidate_raw_kind;
    for (int i = 0; i < value_input_count; ++i) {
      Node* input = NodeProperties::GetValueInput(node, i);
      auto input_type_opt = GetNodeTypeAST(input);
      if (!input_type_opt.has_value()) {
        TYPE_HELPER_DEBUG("start_pos=%d phi#%d input#%d node#%d type=missing",
                          context_.current_start_pos(), node->id(), i,
                          input ? input->id() : -1);
        continue;
      }

      TypeAST::TypeKind input_kind = input_type_opt->kind;
      if (is_raw_kind(input_kind)) {
        if (!candidate_raw_kind.has_value()) {
          candidate_raw_kind = input_kind;
          continue;
        }
        if (candidate_raw_kind.value() != input_kind) {
          TYPE_HELPER_DEBUG(
              "start_pos=%d phi#%d raw-kind-mismatch input#%d kind=%d candidate=%d",
              context_.current_start_pos(), node->id(), i,
              static_cast<int>(input_kind),
              static_cast<int>(candidate_raw_kind.value()));
          return std::nullopt;
        }
        continue;
      }

      if (input_kind == TypeAST::Num && TryGetConstantIndex(input).has_value()) {
        continue;
      }

      TYPE_HELPER_DEBUG(
          "start_pos=%d phi#%d rejected input#%d node#%d kind=%d (not raw/const-num)",
          context_.current_start_pos(), node->id(), i,
          input ? input->id() : -1, static_cast<int>(input_kind));

      return std::nullopt;
    }

    if (!candidate_raw_kind.has_value()) {
      if (value_input_count == 2) {
        Node* first = NodeProperties::GetValueInput(node, 0);
        Node* second = NodeProperties::GetValueInput(node, 1);
        Node* const_input = nullptr;
        Node* recur_input = nullptr;

        if (first && TryGetConstantIndex(first).has_value()) {
          const_input = first;
          recur_input = second;
        } else if (second && TryGetConstantIndex(second).has_value()) {
          const_input = second;
          recur_input = first;
        }

        if (const_input && recur_input) {
          IrOpcode::Value recur_opcode = recur_input->opcode();
          bool recur_is_addsub =
              recur_opcode == IrOpcode::kCheckedInt32Add ||
              recur_opcode == IrOpcode::kInt32Add ||
              recur_opcode == IrOpcode::kSpeculativeSmallIntegerAdd ||
              recur_opcode == IrOpcode::kCheckedInt32Sub ||
              recur_opcode == IrOpcode::kInt32Sub ||
              recur_opcode == IrOpcode::kSpeculativeSmallIntegerSubtract;

          if (recur_is_addsub && recur_input->op()->ValueInputCount() >= 2) {
            Node* recur_left = NodeProperties::GetValueInput(recur_input, 0);
            Node* recur_right = NodeProperties::GetValueInput(recur_input, 1);
            bool touches_self = recur_left == node || recur_right == node;
            Node* other = recur_left == node ? recur_right : recur_left;
            bool other_is_const = other && TryGetConstantIndex(other).has_value();

            if (touches_self && other_is_const) {
              TYPE_HELPER_DEBUG(
                  "start_pos=%d phi#%d inferred induction rawint32 via recur node#%d",
                  context_.current_start_pos(), node->id(), recur_input->id());
              TypeAST phi_type;
              phi_type.kind = TypeAST::RawInt32;
              return cache_and_return(phi_type);
            }
          }
        }
      }

      TYPE_HELPER_DEBUG("start_pos=%d phi#%d no-raw-candidate", context_.current_start_pos(),
                        node->id());
      return std::nullopt;
    }

    TypeAST phi_type;
    phi_type.kind = candidate_raw_kind.value();
    return cache_and_return(phi_type);
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
      auto field_type_opt = FindFieldInObj(object_type, field_name);
      if (!field_type_opt.has_value()) {
        return std::nullopt;
      }
      return cache_and_return(field_type_opt.value());
    }
  } else if (node->opcode() == IrOpcode::kLoadElement) {
    Node* array_node = node->InputAt(0);
    auto array_type_opt = GetNodeTypeAST(array_node);
    if (!array_type_opt.has_value()) {
      return std::nullopt;
    }

    const TypeAST& array_type = array_type_opt.value();
    if (array_type.kind == TypeAST::Arr) {
      auto element_type_opt = GetElementTypeInArray(array_type);
      if (!element_type_opt.has_value()) {
        return std::nullopt;
      }
      return cache_and_return(element_type_opt.value());
    } else if (array_type.kind == TypeAST::Tuple) {
      Node* index_node = node->InputAt(1);
      auto index_opt = TryGetConstantIndex(index_node);
      if (index_opt.has_value()) {
        auto element_type_opt =
            GetElementTypeInTuple(array_type, index_opt.value());
        if (!element_type_opt.has_value()) {
          return std::nullopt;
        }
        return cache_and_return(element_type_opt.value());
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
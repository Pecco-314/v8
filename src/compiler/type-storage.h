#ifndef V8_COMPILER_TYPE_STORAGE_H_
#define V8_COMPILER_TYPE_STORAGE_H_

#include "src/builtins/builtins.h"
#include "src/compiler/turbofan-types.h"

namespace v8 {
namespace internal {
namespace compiler {

struct TypeAST {
  enum TypeKind : int {
    Any = 0,
    Void,
    Bool,
    Num,
    Str,
    Sym,
    I32,
    U32,
    I64,
    U64,
    F32,
    F64,
    Arr = 100, // 以下为复合类型
    Tuple,
    Interface,
    Class,
  } kind;
  std::string field_name;  // 仅用于 obj 的字段名
  std::vector<TypeAST> children;
  const char* KindToString() const {
    switch (kind) {
      case Any:
        return "Any";
      case Void:
        return "Void";
      case Bool:
        return "Bool";
      case Num:
        return "Num";
      case Str:
        return "Str";
      case Sym:
        return "Sym";
      case I32:
        return "I32";
      case U32:
        return "U32";
      case I64:
        return "I64";
      case U64:
        return "U64";
      case F32:
        return "F32";
      case F64:
        return "F64";
      case Arr:
        return "Arr";
      case Tuple:
        return "Tuple";
      case Interface:
        return "Interface";
      case Class:
        return "Class";
      default:
        return "Unknown";
    }
  }
  bool IsPrimitive() const { return kind < 100; }
  void Print() const {
    if (!field_name.empty()) {
      std::cout << field_name << ": ";
    }
    if (IsPrimitive()) {
      std::cout << KindToString();
    } else {
      std::cout << KindToString() << "[";
      for (size_t i = 0; i < children.size(); ++i) {
        children[i].Print();
        if (i + 1 < children.size()) {
          std::cout << ", ";
        }
      }
      std::cout << "]";
    }
  }
  void PrintLn() const {
    Print();
    std::cout << std::endl;
  }
};

class TypeStorage {
 private:
  std::map<std::string, std::map<int, std::vector<TypeAST>>> storage_;
  std::map<std::string, std::map<int, TypeAST>> return_types_;
  
  // 内建函数签名存储
  struct BuiltinSignature {
    std::vector<TypeAST> param_types;  // 参数类型列表（第一个是接收者）
    TypeAST return_type;               // 返回值类型
  };
  std::map<int, BuiltinSignature> builtin_signatures_;
  bool builtin_types_loaded_ = false;
  
  void ReadBuiltinFile();

 public:
  static TypeStorage* Get();
  TypeStorage() = default;
  void ReadFile(std::string hash);
  std::map<int, std::vector<TypeAST>> GetTypeMap(std::string hash);
  std::optional<TypeAST> GetReturnType(std::string hash, int bytecode_offset);

  // 内建函数类型表（基于 Builtin ID）
  std::optional<TypeAST> GetBuiltinReturnType(Builtin builtin_id);
  std::optional<BuiltinSignature> GetBuiltinSignature(Builtin builtin_id);
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_TYPE_STORAGE_H_
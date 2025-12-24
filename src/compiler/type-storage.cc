#include "src/compiler/type-storage.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "src/base/lazy-instance.h"
#include "src/compiler/type-cache.h"
#include "src/flags/flags.h"

namespace v8 {
namespace internal {
namespace compiler {

DEFINE_LAZY_LEAKY_OBJECT_GETTER(TypeStorage, TypeStorage::Get)

class TypeParser {
 public:
  explicit TypeParser(const std::string& input) : input_(input), pos_(0) {}

  TypeAST Parse() { return ParseNext(); }

 private:
  const std::string& input_;
  size_t pos_;

  // 辅助：字符串映射到 Enum
  static TypeAST::TypeKind StringToKind(const std::string& s) {
    if (s == "any") return TypeAST::Any;
    if (s == "void") return TypeAST::Void;
    if (s == "bool") return TypeAST::Bool;
    if (s == "num") return TypeAST::Num;
    if (s == "str") return TypeAST::Str;

    if (s == "i32") return TypeAST::I32;
    if (s == "u32") return TypeAST::U32;
    if (s == "i64") return TypeAST::I64;
    if (s == "u64") return TypeAST::U64;
    if (s == "f32") return TypeAST::F32;
    if (s == "f64") return TypeAST::F64;

    if (s == "arr") return TypeAST::Arr;
    if (s == "tuple") return TypeAST::Tuple;
    if (s == "interface") return TypeAST::Interface;
    if (s == "class") return TypeAST::Class;

    return TypeAST::Any;  // 默认或未知
  }

  char Peek() const {
    if (pos_ >= input_.size()) return 0;
    return input_[pos_];
  }

  char Advance() {
    if (pos_ >= input_.size()) return 0;
    return input_[pos_++];
  }

  bool Match(char expected) {
    if (Peek() == expected) {
      pos_++;
      return true;
    }
    return false;
  }

  std::string ParseIdentifier() {
    size_t start = pos_;
    while (pos_ < input_.size()) {
      char c = input_[pos_];
      // 允许字母、数字、下划线、# (用于readonly标记)
      if (isalnum(c) || c == '_' || c == '#') {
        pos_++;
      } else {
        break;
      }
    }
    return input_.substr(start, pos_ - start);
  }

  // 核心递归函数
  TypeAST ParseNext() {
    TypeAST node;

    // 1. 读取类型字符串并转换为 Enum
    std::string token = ParseIdentifier();
    node.kind = StringToKind(token);

    // 情况 A: 泛型 <T>
    if (Match('<')) {
      // 递归解析内部类型
      node.children.push_back(ParseNext());

      // 注意：如果将来支持类似于 Map<K,V>，这里需要循环处理逗号

      if (!Match('>')) {
        std::cerr << "[Metadata Parse Error] Expected '>' at pos " << pos_
                  << std::endl;
      }
    }
    // 情况 B: 元组 [T1, T2]
    else if (Match('[')) {
      while (Peek() != ']' && Peek() != 0) {
        node.children.push_back(ParseNext());
        if (Peek() == ',') {
          Advance();
        } else {
          break;  // 期待 ']'
        }
      }
      if (!Match(']')) {
        std::cerr << "[Metadata Parse Error] Expected ']' at pos " << pos_
                  << std::endl;
      }
    }
    // 情况 C: 接口/类 {key:Val, #key:Val}
    else if (Match('{')) {
      while (Peek() != '}' && Peek() != 0) {
        // 1. 解析 Key (字段名，可能包含 # 前缀)
        std::string key = ParseIdentifier();

        if (!Match(':')) {
          std::cerr << "[Metadata Parse Error] Expected ':' after key " << key
                    << std::endl;
        }

        // 2. 递归解析 Value 类型
        TypeAST value_node = ParseNext();

        // 3. 将 Key 附着在子节点上
        value_node.field_name = key;
        node.children.push_back(value_node);

        if (Peek() == ',') {
          Advance();
        } else {
          break;  // 期待 '}'
        }
      }
      if (!Match('}')) {
        std::cerr << "[Metadata Parse Error] Expected '}' at pos " << pos_
                  << std::endl;
      }
    }

    return node;
  }
};

void TypeStorage::ReadFile(std::string hash) {
  if (storage_.find(hash) != storage_.end()) {
    return;
  }
  // 使用 --turbo_metadata_path 传入的路径，如果没有设置则不加载 metadata
  const char* metadata_path = v8_flags.turbo_metadata_path;
  if (metadata_path == nullptr) {
    return;
  }
  std::string dir(metadata_path);
  if (!dir.empty() && dir.back() != '/') {
    dir += '/';
  }
  auto path = dir + hash + ".metadata";
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return;
  }

  auto& st = storage_[hash];
  int pos, num;
  std::string type_token;

  // 文件格式解析: [BytecodeOffset] [NumCount] [TypeString...]
  // NumCount 包含所有参数：this (索引 0) + 其他参数 (索引 1, 2, ...)
  // 注意：文本格式中的复合类型内部不允许包含空格，例如必须写为 "parr<i32>" 而非
  // "parr< i32 >"
  while (ifs >> pos >> num) {
    std::vector<TypeAST> type_list;
    for (int i = 0; i < num; ++i) {
      ifs >> type_token;
      TypeParser parser(type_token);
      TypeAST ast = parser.Parse();
      type_list.push_back(ast);
    }
    st[pos] = type_list;
  }
}

std::map<int, std::vector<TypeAST>> TypeStorage::GetTypeMap(std::string hash) {
  ReadFile(hash);
  if (storage_.find(hash) != storage_.end()) {
    return storage_[hash];
  } else {
    return {};
  }
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
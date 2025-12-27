#include "src/compiler/type-storage.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
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
    if (s == "symbol") return TypeAST::Symbol;
    if (s == "bigint") return TypeAST::BigInt;
    if (s == "rawint32") return TypeAST::RawInt32;

    if (s == "arr") return TypeAST::Arr;
    if (s == "tuple") return TypeAST::Tuple;
    if (s == "obj") return TypeAST::Obj;

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

    // 情况 A: 泛型 <T, U, ...> 
    if (Match('<')) {
      while (Peek() != '>' && Peek() != 0) {
        node.children.push_back(ParseNext());
        if (Peek() == ',') {
          Advance();
        } else {
          break;  // 期待 '>'
        }
      }
      if (!Match('>')) {
        std::cerr << "[Metadata Parse Error] Expected '>' at pos " << pos_
                  << std::endl;
      }
    }
    // 情况 B: 接口/类 {key:Val, #key:Val}
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
  std::string line;
  
  // 新文件格式解析: [BytecodeOffset] @params [param1 param2 ...] @ret [return_type]
  // 例如: 280 @params any str @ret str
  //      308 @params any @ret str
  while (std::getline(ifs, line)) {
    if (line.empty() || line[0] == '#') continue;  // 跳过空行和注释
    
    std::istringstream iss(line);
    int pos;
    std::string token;
    
    if (!(iss >> pos)) continue;  // 读取 bytecode offset
    
    std::vector<TypeAST> type_list;
    TypeAST return_type;
    return_type.kind = TypeAST::Any;  // 默认返回值为 Any
    
    bool in_params = false;
    bool in_ret = false;
    
    while (iss >> token) {
      if (token == "@params") {
        in_params = true;
        in_ret = false;
      } else if (token == "@ret") {
        in_params = false;
        in_ret = true;
      } else if (in_params) {
        // 解析参数类型
        TypeParser parser(token);
        TypeAST ast = parser.Parse();
        type_list.push_back(ast);
      } else if (in_ret) {
        // 解析返回值类型
        TypeParser parser(token);
        return_type = parser.Parse();
        break;  // 返回值只有一个
      }
    }
    
    // 将返回值类型追加到参数列表末尾
    // 这样可以通过索引 param_types_.size() - 1 访问返回值类型
    type_list.push_back(return_type);
    
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

// 读取内建函数类型文件
void TypeStorage::ReadBuiltinFile() {
  if (builtin_types_loaded_) {
    return;
  }
  builtin_types_loaded_ = true;
  
  // 尝试从多个位置查找 builtin.metadata
  std::vector<std::string> search_paths;
  
  // 1. 优先使用 --turbo_metadata_path 指定的目录
  const char* metadata_path = v8_flags.turbo_metadata_path;
  if (metadata_path != nullptr) {
    std::string dir(metadata_path);
    if (!dir.empty() && dir.back() != '/') {
      dir += '/';
    }
    search_paths.push_back(dir + "builtin.metadata");
  }
  
  // 2. 项目根目录的 metadata/ 目录（编译时路径）
  search_paths.push_back("metadata/builtin.metadata");
  
  // 3. 相对于可执行文件的 metadata/ 目录
  search_paths.push_back("../metadata/builtin.metadata");
  
  std::ifstream ifs;
  for (const auto& path : search_paths) {
    ifs.open(path);
    if (ifs.is_open()) {
      break;
    }
  }
  
  if (!ifs.is_open()) {
    return;
  }
  
  std::string line;
  // 格式: [BuiltinId] @params [param1 param2 ...] @ret [return_type]
  // 例如: 123 @params any @ret str  # NumberPrototypeToString
  //      456 @params any @ret str  # ObjectPrototypeToString
  while (std::getline(ifs, line)) {
    // 移除注释
    size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }
    
    // 跳过空行
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
      continue;
    }
    
    std::istringstream iss(line);
    int builtin_id;
    std::string token;
    
    if (!(iss >> builtin_id)) continue;  // 读取 builtin ID
    
    BuiltinSignature signature;
    signature.return_type.kind = TypeAST::Any;  // 默认返回值为 Any
    
    bool in_params = false;
    bool in_ret = false;
    
    while (iss >> token) {
      if (token == "@params") {
        in_params = true;
        in_ret = false;
      } else if (token == "@ret") {
        in_params = false;
        in_ret = true;
      } else if (in_params) {
        // 解析参数类型（第一个是接收者）
        TypeParser parser(token);
        signature.param_types.push_back(parser.Parse());
      } else if (in_ret) {
        // 解析返回值类型
        TypeParser parser(token);
        signature.return_type = parser.Parse();
        break;  // 返回值只有一个
      }
    }
    
    builtin_signatures_[builtin_id] = signature;
  }
}

// 基于 Builtin ID 获取返回值类型
std::optional<TypeAST> TypeStorage::GetBuiltinReturnType(Builtin builtin_id) {
  auto sig = GetBuiltinSignature(builtin_id);
  if (sig.has_value()) {
    return sig->return_type;
  }
  return std::nullopt;
}

// 基于 Builtin ID 获取完整的函数签名
std::optional<TypeStorage::BuiltinSignature> TypeStorage::GetBuiltinSignature(Builtin builtin_id) {
  // 首次调用时加载内建函数类型文件
  ReadBuiltinFile();
  
  int id = static_cast<int>(builtin_id);
  auto it = builtin_signatures_.find(id);
  if (it != builtin_signatures_.end()) {
    return it->second;
  }
  
  return std::nullopt;
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
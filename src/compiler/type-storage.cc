#include "src/compiler/type-storage.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include "src/base/lazy-instance.h"
#include "src/compiler/type-cache.h"
#include "src/flags/flags.h"
#include "src/utils/hex-format.h"
#include "src/utils/sha-256.h"

namespace v8 {
namespace internal {
namespace compiler {

namespace {

std::string Trim(const std::string& s) {
  size_t begin = s.find_first_not_of(" \t\r\n");
  if (begin == std::string::npos) return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(begin, end - begin + 1);
}

std::string Sha256Hex(const std::string& input) {
  uint8_t hash[kSizeOfSha256Digest];
  SHA256_hash(input.data(), input.size(), hash);
  char formatted_hash[kSizeOfFormattedSha256Digest];
  FormatBytesToHex(formatted_hash, kSizeOfFormattedSha256Digest, hash,
                   kSizeOfSha256Digest);
  formatted_hash[kSizeOfSha256Digest * 2] = '\0';
  return std::string(formatted_hash);
}

std::string ToLower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

}  // namespace

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
    if (s == "rawuint32") return TypeAST::RawUint32;
    if (s == "rawint64") return TypeAST::RawInt64;
    if (s == "rawuint64") return TypeAST::RawUint64;

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
  std::map<std::string, std::string> headers;
  std::vector<std::string> entry_lines;
  
  // 新文件格式解析: [BytecodeOffset] @params [param1 param2 ...] @ret [return_type]
  // 例如: 280 @params any str @ret str
  //      308 @params any @ret str
  while (std::getline(ifs, line)) {
    std::string trimmed = Trim(line);
    if (trimmed.empty()) continue;

    if (trimmed[0] == '#') {
      // Header: # Key: Value
      const std::string body = Trim(trimmed.substr(1));
      size_t colon = body.find(':');
      if (colon != std::string::npos) {
        std::string key = Trim(body.substr(0, colon));
        std::string value = Trim(body.substr(colon + 1));
        headers[key] = value;
      }
      continue;
    }

    entry_lines.push_back(trimmed);

    std::string parse_line = trimmed;
    size_t comment_pos = parse_line.find('#');
    if (comment_pos != std::string::npos) {
      parse_line = Trim(parse_line.substr(0, comment_pos));
    }
    if (parse_line.empty()) continue;

    std::istringstream iss(parse_line);
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

  const bool require_v2_provenance =
      headers.find("Metadata-Version") != headers.end() &&
      headers["Metadata-Version"] == "2";
  if (!require_v2_provenance) {
    return;
  }

  const std::array<const char*, 10> required_headers = {
      "Provenance-Generator",       "Provenance-Generator-SHA256",
      "Provenance-TS-Source",       "Provenance-TS-SHA256",
      "Provenance-TSC-Version",     "Source",
      "JS SHA256",                  "Entries SHA256",
      "Provenance-Stamp",           "Provenance-Generated-At",
  };

  for (const char* key : required_headers) {
    auto it = headers.find(key);
    if (it == headers.end() || it->second.empty()) {
      storage_.erase(hash);
      return;
    }
  }

  if (headers["Provenance-Generator"] != "ts_to_metadata.js") {
    storage_.erase(hash);
    return;
  }

  if (ToLower(headers["JS SHA256"]) != ToLower(hash)) {
    storage_.erase(hash);
    return;
  }

  const std::string computed_entries_sha = Sha256Hex([&entry_lines]() {
    std::ostringstream os;
    for (size_t i = 0; i < entry_lines.size(); ++i) {
      os << entry_lines[i];
      if (i + 1 < entry_lines.size()) os << "\n";
    }
    return os.str();
  }());

  if (ToLower(headers["Entries SHA256"]) != ToLower(computed_entries_sha)) {
    storage_.erase(hash);
    return;
  }

  std::ostringstream stamp_input;
  stamp_input << "v1\n" << headers["Provenance-Generator"] << "\n"
              << headers["Provenance-Generator-SHA256"] << "\n"
              << headers["Provenance-TS-Source"] << "\n"
              << headers["Provenance-TS-SHA256"] << "\n"
              << headers["Source"] << "\n"
              << headers["JS SHA256"] << "\n"
              << headers["Provenance-TSC-Version"] << "\n"
              << headers["Entries SHA256"];
  const std::string computed_stamp = Sha256Hex(stamp_input.str());

  if (ToLower(headers["Provenance-Stamp"]) != ToLower(computed_stamp)) {
    storage_.erase(hash);
    return;
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
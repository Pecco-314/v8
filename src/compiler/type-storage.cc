#include "src/compiler/type-storage.h"
#include "src/compiler/type-cache.h"
#include "src/base/lazy-instance.h"
#include <fstream>

// TODO: not hardcode the path
static const char* DIR = "metadata/";

namespace v8 {
namespace internal {
namespace compiler {

DEFINE_LAZY_LEAKY_OBJECT_GETTER(TypeStorage, TypeStorage::Get)

static Type GetTypeFromString(std::string type_str) {
  if (type_str == "s") {
    return Type::String();
  } else {
    return Type::Any();
  }
}

void TypeStorage::ReadFile(std::string hash) {
  if (storage_.find(hash) != storage_.end()) {
    return;
  }
  auto path = DIR + hash + ".metadata";
  std::ifstream ifs(path);
  if (!ifs.is_open()) {
    return;
  }
  auto &st = storage_[hash];
  std::string type_str;
  int pos, num;
  while (ifs >> pos >> num) {
    std::vector<Type> type_list;
    for (int i = 0; i < num; ++i) {
      ifs >> type_str;
      type_list.push_back(GetTypeFromString(type_str));
    }
    st[pos] = type_list;
  }
}

std::map<int, std::vector<Type>> TypeStorage::GetTypeMap(std::string hash) {
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

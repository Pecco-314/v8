#ifndef V8_COMPILER_TYPE_STORAGE_H_
#define V8_COMPILER_TYPE_STORAGE_H_


#include "src/compiler/turbofan-types.h"

namespace v8 {
namespace internal {
namespace compiler {

class TypeStorage {
  private:
    std::map<std::string, std::map<int, std::vector<Type>>> storage_;

  public:
    static TypeStorage* Get();
    TypeStorage() = default;
    void ReadFile(std::string hash);
    std::map<int, std::vector<Type>> GetTypeMap(std::string hash);
};

}  // namespace compiler
}  // namespace internal
}  // namespace v8

#endif  // V8_COMPILER_TYPE_STORAGE_H_
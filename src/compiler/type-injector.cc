#include "src/compiler/type-injector.h"

#include "src/compiler/all-nodes.h"
#include "src/compiler/common-operator.h"
#include "src/compiler/node-properties.h"
#include "src/compiler/node.h"
#include "src/compiler/turbofan-types.h"
#include "src/compiler/type-storage.h"

namespace v8 {
namespace internal {
namespace compiler {

void TypeInjector::Run() {
  const std::string& script_hash = compilation_info_->cached_script_hash();
  IndirectHandle<SharedFunctionInfo> shared = compilation_info_->shared_info();
  int start_pos = shared->StartPosition();

  auto* storage = TypeStorage::Get();
  auto typemap = storage->GetTypeMap(script_hash);

  std::vector<Type> param_types;
  auto it = typemap.find(start_pos);
  if (it != typemap.end()) param_types = it->second;

  // Iterate over all nodes reachable from end and assign types to Parameter
  // nodes when we have metadata for them.
  AllNodes all(graph_->zone(), graph_);
  for (Node* node : all.reachable) {
    if (node->opcode() == IrOpcode::kParameter) {
      int index = ParameterIndexOf(node->op());
      if (index > 0 && size_t(index) < param_types.size()) {
        NodeProperties::SetType(node, param_types[index]);
      }
    }
    // TODO: give types to JSCall nodes
  }
}

}  // namespace compiler
}  // namespace internal
}  // namespace v8
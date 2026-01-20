#include "src/compiler/metadata-type-context.h"

#include <memory>
#include <utility>

#include "src/base/lazy-instance.h"
#include "src/codegen/optimized-compilation-info.h"
#include "src/objects/shared-function-info-inl.h"

namespace v8::internal::compiler {

const TypeAST* MetadataTypeContext::StoreOwnedTypeAST(const TypeAST& ast) {
  owned_typeasts_.push_back(ast);
  return &owned_typeasts_.back();
}

void MetadataTypeContext::SetNodeType(Node* node, const TypeAST* type_ast) {
  if (type_ast == nullptr) return;
  node_type_map_[node] = type_ast;
}

const TypeAST* MetadataTypeContext::GetNodeType(Node* node) const {
  auto it = node_type_map_.find(node);
  if (it == node_type_map_.end()) return nullptr;
  return it->second;
}

void MetadataTypeContext::EnsureLoaded(OptimizedCompilationInfo* info,
                                       TypeStorage* storage) {
  std::string new_hash = info->cached_script_hash();
  IndirectHandle<SharedFunctionInfo> shared = info->shared_info();
  int new_start_pos = shared->StartPosition();

  if (loaded_ && script_hash_ == new_hash && current_start_pos_ == new_start_pos) {
    return;
  }

  loaded_ = true;
  script_hash_ = std::move(new_hash);
  current_start_pos_ = new_start_pos;

  node_type_map_.clear();
  owned_typeasts_.clear();
  param_types_.clear();

  auto typemap = storage->GetTypeMap(script_hash_);
  auto it = typemap.find(current_start_pos_);
  if (it != typemap.end()) {
    param_types_ = it->second;
  }
}

class MetadataTypeContextStoreImpl {
 public:
  MetadataTypeContext& Get(TFGraph* graph) const {
    auto it = contexts_.find(graph);
    if (it != contexts_.end()) return *it->second;
    auto context = std::make_unique<MetadataTypeContext>(graph);
    MetadataTypeContext& ref = *context;
    contexts_.emplace(graph, std::move(context));
    return ref;
  }

 private:
  mutable std::unordered_map<TFGraph*, std::unique_ptr<MetadataTypeContext>>
      contexts_;
};

MetadataTypeContext& MetadataTypeContextStore::Get(TFGraph* graph) {
  static base::LazyInstance<MetadataTypeContextStoreImpl>::type store =
      LAZY_INSTANCE_INITIALIZER;
  return store.Get().Get(graph);
}

}  // namespace v8::internal::compiler
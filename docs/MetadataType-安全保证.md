# MetadataType 安全保证（来源可证明）

本文档描述当前已落地保障：

1. **来源可证明（Provenance）**

---

## 1. 来源可证明

### 1.1 metadata 头部新增字段

`ts_to_metadata.js` 现在会写入以下头部（`Metadata-Version: 2`）：

- `Provenance-Generator`: 生成器名称（固定 `ts_to_metadata.js`）
- `Provenance-Generator-SHA256`: 生成器脚本 SHA256
- `Provenance-TS-Source`: TS 源路径（相对仓库）
- `Provenance-TS-SHA256`: TS 源内容 SHA256
- `Provenance-TSC-Version`: `tsc --version` 输出
- `Provenance-Generated-At`: 生成时间（ISO8601）
- `Source`: JS 路径（相对仓库）
- `JS SHA256`: JS 内容 SHA256（同时作为 metadata 文件名）
- `Entries SHA256`: 所有函数条目行的 SHA256
- `Provenance-Stamp`: 对关键字段的二次摘要（用于一致性校验）

### 1.2 加载侧校验规则（`TypeStorage::ReadFile`）

当 `Metadata-Version == 2` 时，加载器会执行以下强校验：

1. 必须存在全部必需头字段；否则拒绝加载。
2. `Provenance-Generator` 必须为 `ts_to_metadata.js`；否则拒绝加载。
3. `JS SHA256` 必须与当前读取文件名 hash 一致；否则拒绝加载。
4. 重新计算 `Entries SHA256` 与文件声明值比对，不一致则拒绝加载。
5. 重新计算 `Provenance-Stamp` 与文件声明值比对，不一致则拒绝加载。

### 1.3 失败行为

任一校验失败会直接丢弃该 metadata（不注入类型），编译流程回退为保守路径。

---

## 2. 已暂时回滚能力

`绑定不可错配（Binding Integrity）` 已暂时回滚，当前不再写入/校验 `@len`、`@nameh`。

当前 metadata 条目格式保持为：

```text
<start_pos> @params ... @ret ... # <name>
```

后续若重新启用该能力，会再补充兼容与迁移说明。

---

## 3. 不安全行为默认保守策略（AST 识别版）

### 3.1 识别范围（当前）

当前仅识别两类不安全行为：

- `as` / `<Type>expr` 类型断言
- `@ts-ignore` 注释

### 3.2 默认行为（保守）

一旦在任意函数中识别到上述不安全行为，默认策略为：

- **不做局部保守化**（避免类型系统信任被局部污染）
- **整份文件不注入任何函数 metadata**（`Entries-Selected` 变为 `0/N`）

这样可以确保该文件不会因部分“看似安全”的条目而继续传播不可信类型信息。

### 3.3 强制开启（仅专家使用）

如确需在含不安全行为的文件上继续注入 metadata，可显式使用：

```bash
node scripts/ts_to_metadata.js <ts_file> --forceUnsafeMetadata
```

即使强制开启，脚本仍会打印明显警告，提醒用户该行为会降低类型可信性。

### 3.4 metadata 头部新增策略字段

- `Unsafe-Policy`: 当前策略（`conservative-default`）
- `Unsafe-Detection`: 当前识别范围（`as-expression,ts-ignore`）
- `Unsafe-Detected-Count`: 检测到不安全函数数量
- `Unsafe-Forced`: 是否强制开启（`true/false`）
- `Unsafe-Functions`: 命中的函数名列表
- `Unsafe-As-Functions`: 含 `as` 的函数名列表
- `Unsafe-TsIgnore-Functions`: 含 `@ts-ignore` 的函数名列表
- `Entries-Selected`: 最终写入条目数 / 函数总数

---

## 4. 关键实现位置

- 生成端：`scripts/ts_to_metadata.js`
- 存储与校验：`src/compiler/type-storage.{h,cc}`
- 编译期上下文读取：`src/compiler/metadata-type-context.cc`

---

## 5. 使用建议

1. 在 CI 中统一使用 `scripts/ts_to_metadata.js` 生成 metadata。
2. 禁止手改 `metadata/*.metadata`，改动应来自 TS 源再生成。
3. 回归时优先检查：`Entries SHA256` / `Provenance-Stamp` 失败次数。
4. 默认不要使用 `--forceUnsafeMetadata`；仅在明确知晓风险时启用。

---

## 6. 当前边界

当前“来源可证明”是**强一致性证明**（字段与内容自洽、来源链可追踪），不是基于私钥的不可伪造签名。

如需进一步提升到“不可伪造”，可在后续增加：

- CI 私钥签名（Ed25519）
- V8 侧内置公钥验签
- key rotation / 吊销机制

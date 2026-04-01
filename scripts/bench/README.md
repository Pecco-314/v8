# 基准测试框架（micro-bench）

当前工作流以 TypeScript 为主：

- 把 benchmark 文件放到 `scripts/bench/benchmarks/`（推荐 `.ts`）
- `run_pipeline.py` 默认先执行 `ts_locals_to_wrappers.js` 预处理，再编译 TS → JS 并生成 metadata
- pipeline 自动对比 **without metadata** 和 **with metadata**
- nightly 统一由 `run_nightly.py` 编排三类指标：运行时间、编译时延、二进制字节数

## 目录约定

- `scripts/bench/benchmarks/`：基准文件目录（推荐 `.ts`，也支持 `.js`）
- `scripts/bench/metadata/`：metadata 目录（文件名为 benchmark 文件 SHA256）
- `scripts/bench/config/defaults.json`：全局默认配置
- `scripts/bench/config/overrides.json`：可选特例覆盖（按文件名）

## Benchmark 文件约定

每个 benchmark 文件必须定义：

- `bench()`：必需，单次被测操作
- `setup()`：可选
- `teardown()`：可选

示例：`scripts/bench/benchmarks/matrix-multiply.ts`

## 运行方式

### 1) 跑所有 benchmark（推荐）

```bash
python3 scripts/bench/run_pipeline.py
```

### 1.1) 跑 nightly 统一任务（推荐）

```bash
python3 scripts/bench/run_nightly.py
```

默认会执行三个维度：

- `performance`（运行时间）
- `compile_latency`（编译时延）
- `binary_codegen`（二进制字节数）

并且对 `binary_codegen` 默认启用：

- `--stats-scope all-business`
- `--compile-coverage`
- `--coverage-all-business-functions`
- `--no-inline`
- `--verify-rawint32-add-reduction`

即：保持预热、尽可能覆盖并编译业务函数、禁用内联，统计 **所有已编译业务函数** 的总字节数。

### 2) 只跑单个 benchmark

```bash
python3 scripts/bench/run_pipeline.py \
  --bench scripts/bench/benchmarks/matrix-multiply.ts
```

nightly 单基准：

```bash
python3 scripts/bench/run_nightly.py \
  --bench-dir scripts/bench/benchmarks \
  --max-benches 1
```

### 3) 指定 d8 / 配置文件

```bash
python3 scripts/bench/run_pipeline.py \
  --d8 out.gn/x64.release/d8 \
  --config scripts/bench/config/defaults.json \
  --overrides scripts/bench/config/overrides.json
```

### 4) 关闭 wrappers 预处理（A/B 对照）

```bash
python3 scripts/bench/run_pipeline.py \
  --bench scripts/bench/benchmarks/matrix-multiply.ts \
  --no-wrapper-preprocess
```

## 默认配置与特例覆盖

### 全局默认

在 `scripts/bench/config/defaults.json` 统一设置所有 benchmark 默认参数。

### 特例覆盖

在 `scripts/bench/config/overrides.json` 按文件名覆盖，示例：

```json
{
  "typeinjector-primitive.js": {
    "perf_events": "cycles,instructions"
  }
}
```

未列出的 benchmark 全部走默认配置。

## Metadata 校验

`run_pipeline.py` 默认严格要求 metadata：

1. 计算 benchmark 文件 SHA256
2. 查找 `metadata_dir/<sha>.metadata`（TS 基准会在运行前自动生成）
3. 缺失即报错退出（避免误测）

## 输出

- 每次 `run_bench.py` 输出：`tmp/bench/results-*.json`
- pipeline 汇总输出：`tmp/bench/pipeline-report-*.json`
- 逐项追加记录：`tmp/bench/bench-results.jsonl`
- nightly 汇总输出：`tmp/bench/nightly-results.jsonl`
- nightly 兼容入口：
  - `run_full_nightly.py` → 转发到 `run_nightly.py --dimensions performance,compile_latency,binary_codegen`
  - `run_nightly_codegen_only.py` → 转发到 `run_nightly.py --dimensions binary_codegen`

## 测试方法与设计

本套件用于验证 **元数据驱动优化** 的效果与代价。测试分三条线：

### 1) 测试点结构

每个 benchmark 均包含：

- **热点内核**：`bench()` 内尽量保持纯计算或可控的对象/数组访问。
- **数据准备**：`setup()` 构造确定性输入，避免随机波动。
- **可验证输出**：`bench()` 返回值用于防止被 DCE 消除。

为了偏袒我们的优化，优先覆盖：

- `rawint32/rawuint32` 算术路径
- 多维数组访问、紧密循环
- 对象/字典访问与小型数据结构维护

### 2) 测量方式

- **对比维度**：同一基准，`without metadata` vs `with metadata`。
- **主指标**：`perf stat` 的 `cycles`（可在 overrides 中追加 `instructions`）。
- **稳定性**：使用 `cv` 衡量波动；若 `cv` 过高可提升 `repeats/iterations`。
- **记录方式**：每完成一个基准追加一条 `bench-results.jsonl` 记录，便于增量追踪。

### 3) 评估方法

#### 3.1 机器码体积评估

目标：验证 **图剪枝与算术优化** 是否能减少最终机器码体积。

思路：

- 使用 `generate_assembly.py` 生成 `with/without metadata` 的汇编
- 对 `all-business` 范围统计 **所有已编译业务函数** 的总字节数（不仅是 paired 函数）
- 默认禁用内联，降低噪声并避免字节统计被内联折叠影响
- 观察是否出现 **类型守卫/边界检查/带检算术** 的剥离

操作建议：

```bash
python3 scripts/generate_assembly.py
```

针对正式基准（benchmarks）生成汇编：

```bash
python3 scripts/bench/generate_bench_assembly.py
```

单基准示例：

```bash
python3 scripts/bench/generate_bench_assembly.py \
  --bench scripts/bench/benchmarks/matrix-multiply.ts
```

如果 `bench()` 本身只是包装函数，建议直接对热点函数导出（否则 with/without 可能看起来一致）：

```bash
python3 scripts/bench/generate_bench_assembly.py \
  --bench scripts/bench/benchmarks/matrix-multiply.ts \
  --function matmul \
  --invoke-expr 'matmul(left, right, result)' \
  --trace-ir
```

上面命令会同时输出：

- `matmul_without.asm` / `matmul_with.asm`
- `matmul_without.ir.log` / `matmul_with.ir.log`
- `comparison.md` / `comparison.json`

常用参数：

- `--function/--setup/--teardown`：自定义入口函数名（默认 `bench/setup/teardown`）
- `--invoke-expr`：指定优化调用表达式（适用于目标函数需要参数）
- `--metadata-mode strict|off`：`strict` 对比 with/without metadata，`off` 仅生成无 metadata 版本
- `--trace-ir`：输出 TurboFan IR trace 日志
- `--verify-rawint32-add-reduction`：校验 `V8.TFRawInt32StrengthReduction` 中 `CheckedInt32Add` 在 with metadata 下是否减少
- `--no-wrapper-preprocess`：关闭 `ts_locals_to_wrappers.js` 预处理
- `--out-dir`：自定义输出目录（默认 `docs/assembly/benchmarks`）

输出目录：`docs/assembly/benchmarks/<bench_name>/`

统计脚本（汇编生成后执行）：

```bash
python3 scripts/bench/measure_codegen_size.py
```

输出：

- 汇总：`tmp/bench/codegen-size-<timestamp>.json`
- 逐项：`tmp/bench/codegen-size.jsonl`

输出在 `docs/assembly/`，每个测试函数包含：

- `*_without.asm` / `*_with.asm`
- `comparison.md`（自动统计变化）
- `comparison.json`（结构化统计，便于脚本消费，包含 `rawint32_strength_reduction` 与 `verification.rawint32_add_reduction`）

失败日志输出到：`tmp/bench/bench-asm-logs/<run_id>/`

> 注意：强度缩减可能引入额外指令，体积可能增加或减少，先以数据为准。

#### 3.2 编译开销分析

目标：量化 **元数据传递 + 解析 + 新优化阶段** 的额外时间成本。

建议做法：

1. 使用 `time` 或 `perf stat` 统计 `with metadata` 与 `without metadata` 的编译阶段耗时。
2. 对同一基准多次测量，取中位数或平均值。
3. 记录额外耗时占比，评估是否可接受。

示例命令（仅作为模板）：

```bash
time python3 scripts/bench/run_pipeline.py --bench scripts/bench/benchmarks/matrix-multiply.ts
```

自动统计脚本：

```bash
python3 scripts/bench/measure_compile_cost.py
```

该脚本使用轻量优化 harness（只触发一次优化）来估计编译开销，
避免把大量运行时间混入编译成本。

输出：

- 汇总：`tmp/bench/compile-cost-<timestamp>.json`
- 逐项：`tmp/bench/compile-cost.jsonl`

后续可在 `overrides.json` 为单个基准提高 `repeats`，以便稳定统计编译成本。

## 稳定性建议

- 锁核：`cpu_core`
- 长预热：`warmup >= 5000`
- 重复运行：`repeats >= 30`
- 丢弃前几次：`drop_first >= 5`
- 异常值过滤：`mad_z = 3.0`
- 主指标优先看 `cycles`
- 关注 `cv`（`stdev / mean`），一般 `cv <= 0.03` 认为较稳定

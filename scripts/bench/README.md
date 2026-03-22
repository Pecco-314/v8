# 基准测试框架（micro-bench）

当前工作流只保留一条主线：

- 把 benchmark 文件放到 `scripts/bench/benchmarks/`
- metadata 放到 `scripts/bench/metadata/<SHA256(benchmark文件)>.metadata`
- 运行 `run_pipeline.py` 自动对比 **without metadata** 和 **with metadata**

## 目录约定

- `scripts/bench/benchmarks/`：基准文件目录（直接投放 `.js`）
- `scripts/bench/metadata/`：metadata 目录（文件名为 benchmark 文件 SHA256）
- `scripts/bench/config/defaults.json`：全局默认配置
- `scripts/bench/config/overrides.json`：可选特例覆盖（按文件名）

## Benchmark 文件约定

每个 benchmark 文件必须定义：

- `bench()`：必需，单次被测操作
- `setup()`：可选
- `teardown()`：可选

示例：`scripts/bench/benchmarks/example.js`

## 运行方式

### 1) 跑所有 benchmark（推荐）

```bash
python3 scripts/bench/run_pipeline.py
```

### 2) 只跑单个 benchmark

```bash
python3 scripts/bench/run_pipeline.py \
  --bench scripts/bench/benchmarks/example.js
```

### 3) 指定 d8 / 配置文件

```bash
python3 scripts/bench/run_pipeline.py \
  --d8 out.gn/x64.release/d8 \
  --config scripts/bench/config/defaults.json \
  --overrides scripts/bench/config/overrides.json
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
2. 查找 `metadata_dir/<sha>.metadata`
3. 缺失即报错退出（避免误测）

## 输出

- 每次 `run_bench.py` 输出：`tmp/bench/results-*.json`
- pipeline 汇总输出：`tmp/bench/pipeline-report-*.json`

## 稳定性建议

- 锁核：`cpu_core`
- 长预热：`warmup >= 5000`
- 重复运行：`repeats >= 30`
- 丢弃前几次：`drop_first >= 5`
- 异常值过滤：`mad_z = 3.0`
- 主指标优先看 `cycles`
- 关注 `cv`（`stdev / mean`），一般 `cv <= 0.03` 认为较稳定

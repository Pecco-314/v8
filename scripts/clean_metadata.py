#!/usr/bin/env python3
"""
清理多余的 metadata 文件
只保留 test_config.py 中配置的测试文件对应的 metadata
"""

import os
import hashlib
from pathlib import Path
from test_config import TEST_CASES, V8_CONFIG

def get_file_hash(file_path):
    """计算文件的 SHA256 哈希"""
    with open(file_path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()

def get_required_metadata_files():
    """获取所有需要的 metadata 文件哈希"""
    required = set()
    v8_root = Path.cwd()  # 假设从 v8 根目录运行
    
    for test_case in TEST_CASES:
        for func in test_case["functions"]:
            test_file = v8_root / func["file"]
            if test_file.exists():
                file_hash = get_file_hash(test_file)
                required.add(file_hash)
                print(f"✓ {func['file']}")
                print(f"  哈希: {file_hash}")
    
    return required

def clean_metadata():
    """清理多余的 metadata 文件"""
    v8_root = Path.cwd()
    metadata_dir = v8_root / "test/mjsunit/compiler/type-injector/metadata"
    
    if not metadata_dir.exists():
        print(f"❌ Metadata 目录不存在: {metadata_dir}")
        return
    
    print("=" * 60)
    print("获取需要的 metadata 文件...")
    print("=" * 60)
    required_hashes = get_required_metadata_files()
    
    print("\n" + "=" * 60)
    print("扫描现有 metadata 文件...")
    print("=" * 60)
    
    all_files = list(metadata_dir.glob("*.metadata"))
    kept = []
    removed = []
    
    for metadata_file in all_files:
        file_hash = metadata_file.stem  # 去掉 .metadata 后缀
        
        if file_hash in required_hashes:
            kept.append(metadata_file.name)
            print(f"✓ 保留: {metadata_file.name}")
        else:
            removed.append(metadata_file.name)
            print(f"✗ 删除: {metadata_file.name}")
            metadata_file.unlink()  # 删除文件
    
    print("\n" + "=" * 60)
    print("清理完成")
    print("=" * 60)
    print(f"总文件数: {len(all_files)}")
    print(f"保留: {len(kept)}")
    print(f"删除: {len(removed)}")
    
    if removed:
        print("\n已删除的文件:")
        for name in sorted(removed):
            print(f"  - {name}")
    
    # 检查是否有缺失的 metadata 文件
    existing_hashes = {f.stem for f in metadata_dir.glob("*.metadata")}
    missing = required_hashes - existing_hashes
    
    if missing:
        print("\n" + "=" * 60)
        print("⚠️  缺失的 metadata 文件:")
        print("=" * 60)
        for file_hash in sorted(missing):
            print(f"  {file_hash}.metadata")
        print("\n请使用以下命令生成缺失的 metadata 文件:")
        
        for test_case in TEST_CASES:
            for func in test_case["functions"]:
                test_file = v8_root / func["file"]
                if test_file.exists():
                    file_hash = get_file_hash(test_file)
                    if file_hash in missing:
                        print(f"  python3 scripts/gen_metadata_info.py {func['file']}")

if __name__ == "__main__":
    clean_metadata()

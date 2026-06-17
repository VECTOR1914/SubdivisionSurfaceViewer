#!/usr/bin/env python3
"""
embed_models.py — 将 OBJ 文件嵌入为 C++ 字符串常量

用法:
  python embed_models.py <OBJ目录> <输出头文件>

示例:
  python embed_models.py ../../OBJ_Sample EmbeddedModels.h

输出 EmbeddedModels.h 包含:
  - EmbeddedModel 结构体（名称、数据、大小、是否大模型）
  - g_embeddedModels[] 数组
  - g_embeddedModelCount 计数

分类规则: < 500KB → 小模型, >= 500KB → 大模型
"""

import os
import sys
import glob

# 分类阈值 (字节)
LARGE_MODEL_THRESHOLD = 500 * 1024  # 500 KB

def escape_c_string(text: str) -> str:
    """转义 C++ 原始字符串中可能的冲突序列"""
    # 处理原始字符串中可能出现的 )" 序列
    return text.replace(')"', ')"')

def main():
    if len(sys.argv) < 3:
        print(f"用法: {sys.argv[0]} <OBJ目录> <输出头文件>")
        sys.exit(1)

    obj_dir = sys.argv[1]
    output_path = sys.argv[2]

    # 查找所有 .obj 文件
    obj_files = sorted(glob.glob(os.path.join(obj_dir, "*.obj")))
    if not obj_files:
        print(f"错误: 在 {obj_dir} 中未找到 .obj 文件")
        sys.exit(1)

    models = []
    for filepath in obj_files:
        filename = os.path.basename(filepath)
        name = os.path.splitext(filename)[0]  # 去掉 .obj 后缀
        size = os.path.getsize(filepath)
        is_large = size >= LARGE_MODEL_THRESHOLD

        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            data = f.read()

        models.append({
            'name': name,
            'data': data,
            'size_bytes': size,
            'size_kb': (size + 1023) // 1024,  # 向上取整
            'is_large': is_large,
        })
        print(f"  {'[大]' if is_large else '[小]'} {name}.obj ({models[-1]['size_kb']} KB)")

    # 生成 C++ 头文件
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('// ============================================================\n')
        f.write('// EmbeddedModels.h — 自动生成，请勿手动编辑\n')
        f.write(f'// 来源: {obj_dir}\n')
        f.write(f'// 模型数: {len(models)}\n')
        f.write(f'// 生成时间: {__import__("datetime").datetime.now().strftime("%Y-%m-%d %H:%M:%S")}\n')
        f.write('// ============================================================\n')
        f.write('#pragma once\n\n')
        f.write('#include <cstddef>\n\n')

        f.write('// ============================================================\n')
        f.write('// EmbeddedModel — 嵌入的 OBJ 模型\n')
        f.write('// ============================================================\n')
        f.write('struct EmbeddedModel {\n')
        f.write('    const wchar_t* name;     // 显示名称\n')
        f.write('    const char*    data;     // OBJ 文件原始内容\n')
        f.write('    unsigned int   sizeKB;   // 文件大小 (KB)\n')
        f.write('    bool           isLarge;  // true=大模型, false=小模型\n')
        f.write('};\n\n')

        # 每个模型的 C++ 字符串常量
        for i, m in enumerate(models):
            f.write(f'// ---- {m["name"]} ({m["size_kb"]} KB) ----\n')
            # 使用原始字符串字面量 R"obj(...)obj"
            # 处理数据中可能含有的 )" 序列
            safe_data = escape_c_string(m['data'])
            f.write(f'static const char* EMBED_DATA_{i} = R"obj({safe_data})obj";\n\n')

        # 模型数组
        f.write('// ============================================================\n')
        f.write('// g_embeddedModels — 所有嵌入模型\n')
        f.write('// ============================================================\n')
        f.write('static const EmbeddedModel g_embeddedModels[] = {\n')
        for i, m in enumerate(models):
            is_large_str = 'true' if m['is_large'] else 'false'
            f.write(f'    {{ L"{m["name"]}", EMBED_DATA_{i}, {m["size_kb"]}, {is_large_str} }},\n')
        f.write('};\n\n')
        f.write(f'static const int g_embeddedModelCount = {len(models)};\n')

    print(f"\n已生成: {output_path} ({len(models)} 个模型)")

if __name__ == '__main__':
    main()

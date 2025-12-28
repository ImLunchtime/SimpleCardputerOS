#!/usr/bin/env python3
"""
PNG图片转C++数组工具
将PNG文件转换为C++中可用的const uint8_t数组格式

使用方法:
python png_to_array.py input.png [output.h] [array_name]

参数:
- input.png: 输入的PNG文件路径
- output.h: 输出的头文件路径（可选，默认为input_data.h）
- array_name: 数组变量名（可选，默认为png_data）

示例:
python png_to_array.py icon.png icon_data.h icon_png
"""

import sys
import os
from pathlib import Path

def png_to_cpp_array(input_file, output_file=None, array_name=None):
    """
    将PNG文件转换为C++数组格式
    
    Args:
        input_file (str): 输入PNG文件路径
        output_file (str): 输出头文件路径
        array_name (str): 数组变量名
    """
    
    # 检查输入文件是否存在
    if not os.path.exists(input_file):
        print(f"错误: 文件 '{input_file}' 不存在")
        return False
    
    # 设置默认输出文件名
    if output_file is None:
        base_name = Path(input_file).stem
        output_file = f"{base_name}_data.h"
    
    # 设置默认数组名
    if array_name is None:
        base_name = Path(input_file).stem
        array_name = f"{base_name}_png"
    
    try:
        # 读取PNG文件的二进制数据
        with open(input_file, 'rb') as f:
            png_data = f.read()
        
        # 获取文件大小
        file_size = len(png_data)
        
        # 生成C++头文件内容
        header_content = f"""#pragma once

// 自动生成的PNG图片数据
// 源文件: {input_file}
// 文件大小: {file_size} 字节

#include <stdint.h>

// PNG图片数据数组
constexpr size_t {array_name}_size = {file_size};
constexpr uint8_t {array_name}[] = {{
"""
        
        # 将字节数据转换为十六进制格式
        bytes_per_line = 16  # 每行显示16个字节
        for i in range(0, file_size, bytes_per_line):
            line_bytes = png_data[i:i + bytes_per_line]
            hex_values = [f"0x{byte:02X}" for byte in line_bytes]
            
            if i + bytes_per_line < file_size:
                # 不是最后一行，添加逗号
                header_content += "    " + ", ".join(hex_values) + ",\n"
            else:
                # 最后一行，不添加逗号
                header_content += "    " + ", ".join(hex_values) + "\n"
        
        header_content += "};\n"
        
        # 写入输出文件
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(header_content)
        
        print(f"转换成功!")
        print(f"输入文件: {input_file}")
        print(f"输出文件: {output_file}")
        print(f"数组名称: {array_name}")
        print(f"数组大小: {array_name}_size ({file_size} 字节)")
        print(f"\n使用示例:")
        print(f"#include \"{output_file}\"")
        print(f"UIImage* image = new UIImage(1, 10, 10, 64, 64, {array_name}, {array_name}_size);")
        
        return True
        
    except Exception as e:
        print(f"转换失败: {e}")
        return False

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("PNG图片转C++数组工具")
        print("\n使用方法:")
        print("python png_to_array.py input.png [output.h] [array_name]")
        print("\n参数:")
        print("- input.png: 输入的PNG文件路径")
        print("- output.h: 输出的头文件路径（可选）")
        print("- array_name: 数组变量名（可选）")
        print("\n示例:")
        print("python png_to_array.py icon.png")
        print("python png_to_array.py icon.png icon_data.h")
        print("python png_to_array.py icon.png icon_data.h icon_png")
        return
    
    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else None
    array_name = sys.argv[3] if len(sys.argv) > 3 else None
    
    # 验证输入文件扩展名
    if not input_file.lower().endswith('.png'):
        print("警告: 输入文件不是PNG格式，但仍会尝试转换")
    
    # 执行转换
    success = png_to_cpp_array(input_file, output_file, array_name)
    
    if success:
        sys.exit(0)
    else:
        sys.exit(1)

if __name__ == "__main__":
    main()
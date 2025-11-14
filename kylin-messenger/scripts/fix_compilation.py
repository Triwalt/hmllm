#!/usr/bin/env python3
"""
快速修复编译错误的脚本
适用于src/network/lightweight_discovery.cpp
"""

import re
import sys

def fix_lightweight_discovery(filename):
    """修复lightweight_discovery.cpp中的UserInfo命名空间问题"""
    with open(filename, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. 将函数返回类型中的UserInfo改为Core::UserInfo
    content = re.sub(r'QList<UserInfo>', 'QList<Core::UserInfo>', content)
    content = re.sub(r'std::optional<UserInfo>', 'std::optional<Core::UserInfo>', content)

    # 2. 将lambda参数中的UserInfo改为Core::UserInfo
    content = re.sub(r'\(const UserInfo& a, const UserInfo& b\)',
                     r'(const Core::UserInfo& a, const Core::UserInfo& b)', content)

    # 3. 将函数参数中的UserInfo改为Core::UserInfo
    content = re.sub(r'const UserInfo& user\)', 'const Core::UserInfo& user)', content)

    # 4. 修复UserEntry的info字段声明
    content = re.sub(r'struct UserEntry \{\s*UserInfo info;',
                     r'struct UserEntry {\n        Core::UserInfo info;',
                     content, flags=re.MULTILINE)

    # 5. 修复onlineUsers_.remove为erase
    content = re.sub(r'onlineUsers_\.remove\(userId\);',
                     r'onlineUsers_.erase(userId);',
                     content)

    with open(filename, 'w', encoding='utf-8') as f:
        f.write(content)

    print(f"✓ 已修复 {filename}")

if __name__ == '__main__':
    if len(sys.argv) > 1:
        fix_lightweight_discovery(sys.argv[1])
    else:
        fix_lightweight_discovery('src/network/lightweight_discovery.cpp')

#!/bin/bash
# build.sh - 构建脚本

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}麒麟信使 构建脚本${NC}"
echo -e "${GREEN}========================================${NC}"

# 默认选项
BUILD_TYPE="Release"
ENABLE_AI="ON"
BUILD_TESTS="ON"
CLEAN_BUILD="no"
INSTALL="no"
PACKAGE="no"

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --no-ai)
            ENABLE_AI="OFF"
            shift
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --clean)
            CLEAN_BUILD="yes"
            shift
            ;;
        --install)
            INSTALL="yes"
            shift
            ;;
        --package)
            PACKAGE="yes"
            shift
            ;;
        --help)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --debug      Debug构建（默认: Release）"
            echo "  --no-ai      禁用AI功能"
            echo "  --no-tests   不构建测试"
            echo "  --clean      清理后重新构建"
            echo "  --install    构建后安装"
            echo "  --package    创建DEB安装包"
            echo "  --help       显示此帮助信息"
            exit 0
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            echo "使用 --help 查看可用选项"
            exit 1
            ;;
    esac
done

# 显示配置
echo -e "${YELLOW}构建配置:${NC}"
echo "  构建类型: $BUILD_TYPE"
echo "  AI功能: $ENABLE_AI"
echo "  构建测试: $BUILD_TESTS"
echo ""

# 清理
if [ "$CLEAN_BUILD" = "yes" ]; then
    echo -e "${YELLOW}清理构建目录...${NC}"
    rm -rf build
fi

# 创建构建目录
mkdir -p build
cd build

# CMake配置
echo -e "${YELLOW}配置CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DENABLE_AI_FEATURES=$ENABLE_AI \
    -DBUILD_TESTS=$BUILD_TESTS \
    -DCMAKE_INSTALL_PREFIX=/usr

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake配置失败${NC}"
    exit 1
fi

# 编译
echo -e "${YELLOW}开始编译...${NC}"
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo -e "${RED}编译失败${NC}"
    exit 1
fi

echo -e "${GREEN}编译成功！${NC}"

# 运行测试
if [ "$BUILD_TESTS" = "ON" ]; then
    echo -e "${YELLOW}运行测试...${NC}"
    ctest --output-on-failure
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}测试失败${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}所有测试通过！${NC}"
fi

# 安装
if [ "$INSTALL" = "yes" ]; then
    echo -e "${YELLOW}安装...${NC}"
    sudo make install
    sudo ldconfig
    echo -e "${GREEN}安装完成！${NC}"
fi

# 打包
if [ "$PACKAGE" = "yes" ]; then
    echo -e "${YELLOW}创建DEB包...${NC}"
    make package
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}DEB包创建成功！${NC}"
        ls -lh *.deb
    else
        echo -e "${RED}DEB包创建失败${NC}"
        exit 1
    fi
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}构建完成！${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "可执行文件位于: build/kylin-messenger"
echo ""
echo "运行程序:"
echo "  cd build && ./kylin-messenger"
echo ""
echo "安装:"
echo "  ./build.sh --install"
echo ""
echo "创建DEB包:"
echo "  ./build.sh --package"

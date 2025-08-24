#!/bin/bash

# XUtils 现代化构建脚本

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 显示帮助信息
show_help() {
    cat << EOF
XUtils 现代化构建脚本

用法: $0 [选项]

选项:
    -h, --help              显示此帮助信息
    -c, --clean             清理构建目录
    -d, --debug             构建 Debug 版本
    -r, --release           构建 Release 版本
    -m, --multi             同时构建 Debug 和 Release 版本
    -i, --install           构建并安装
    -t, --test              运行测试
    -v, --verbose           详细输出
    --shared                构建共享库
    --static                构建静态库
    --both                  同时构建共享库和静态库
    --disable-qt            禁用 Qt 依赖
    --enable-qt             启用 Qt 依赖
    --disable-boost         禁用 Boost 依赖
    --enable-boost          启用 Boost 依赖

示例:
    $0 --debug --shared     构建 Debug 版本的共享库
    $0 --release --static   构建 Release 版本的静态库
    $0 --multi --both       同时构建所有版本
    $0 --install            构建并安装
    $0 --test               运行测试
    $0 --multi --disable-qt --enable-boost 多配置构建，禁用Qt，启用Boost

EOF
}

# 默认参数
BUILD_TYPE="Release"
BUILD_SHARED=true
BUILD_STATIC=false
CLEAN_BUILD=false
INSTALL_BUILD=false
RUN_TESTS=false
VERBOSE=false
DISABLE_QT=false
ENABLE_QT=false
DISABLE_BOOST=false
ENABLE_BOOST=false

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -m|--multi)
            BUILD_TYPE="Multi"
            shift
            ;;
        -i|--install)
            INSTALL_BUILD=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        --shared)
            BUILD_SHARED=true
            BUILD_STATIC=false
            shift
            ;;
        --static)
            BUILD_SHARED=false
            BUILD_STATIC=true
            shift
            ;;
        --both)
            BUILD_SHARED=true
            BUILD_STATIC=true
            shift
            ;;
        --disable-qt)
            DISABLE_QT=true
            shift
            ;;
        --enable-qt)
            ENABLE_QT=true
            shift
            ;;
        --disable-boost)
            DISABLE_BOOST=true
            shift
            ;;
        --enable-boost)
            ENABLE_BOOST=true
            shift
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
done

# 检查构建类型
if [[ "$BUILD_TYPE" == "Multi" ]]; then
    print_info "多配置构建模式 - 将同时构建 Debug 和 Release 版本"
    CMAKE_CONFIG_TYPES="-DCMAKE_CONFIGURATION_TYPES=Debug;Release"
    CMAKE_BUILD_TYPE=""
else
    print_info "构建类型: $BUILD_TYPE"
    CMAKE_BUILD_TYPE="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
    CMAKE_CONFIG_TYPES=""
fi

# 设置库构建选项
LIB_OPTIONS=""
if [[ "$BUILD_SHARED" == true ]]; then
    LIB_OPTIONS="$LIB_OPTIONS -DBUILD_SHARED_LIBS=ON"
fi
if [[ "$BUILD_STATIC" == true ]]; then
    LIB_OPTIONS="$LIB_OPTIONS -DBUILD_STATIC_LIBS=ON"
fi

# 设置依赖选项
DEPENDENCY_OPTIONS=""
if [[ "$DISABLE_QT" == true ]]; then
    DEPENDENCY_OPTIONS="$DEPENDENCY_OPTIONS -DDISABLE_QT=ON"
fi
if [[ "$ENABLE_QT" == true ]]; then
    DEPENDENCY_OPTIONS="$DEPENDENCY_OPTIONS -DDISABLE_QT=OFF"
fi
if [[ "$DISABLE_BOOST" == true ]]; then
    DEPENDENCY_OPTIONS="$DEPENDENCY_OPTIONS -DDISABLE_BOOST=ON"
fi
if [[ "$ENABLE_BOOST" == true ]]; then
    DEPENDENCY_OPTIONS="$DEPENDENCY_OPTIONS -DDISABLE_BOOST=OFF"
fi

# 设置其他选项
OTHER_OPTIONS=""
if [[ "$VERBOSE" == true ]]; then
    OTHER_OPTIONS="$OTHER_OPTIONS -DVERBOSE_OUTPUT=ON"
fi

# 清理构建目录
if [[ "$CLEAN_BUILD" == true ]]; then
    print_info "清理构建目录..."
    rm -rf build
    print_success "清理完成"
fi

# 创建构建目录
BUILD_DIR="build"
if [[ "$BUILD_TYPE" == "Multi" ]]; then
    BUILD_DIR="build/multi"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 确定 CMake 源目录
CMAKE_SOURCE_DIR=".."
if [[ "$BUILD_TYPE" == "Multi" ]]; then
    CMAKE_SOURCE_DIR="../.."
fi

# 配置项目
print_info "配置项目..."
CMAKE_ARGS=("cmake" "$CMAKE_SOURCE_DIR")

if [[ -n "$CMAKE_BUILD_TYPE" ]]; then
    CMAKE_ARGS+=("$CMAKE_BUILD_TYPE")
fi

if [[ -n "$CMAKE_CONFIG_TYPES" ]]; then
    CMAKE_ARGS+=("$CMAKE_CONFIG_TYPES")
fi

if [[ -n "$LIB_OPTIONS" ]]; then
    # 将 LIB_OPTIONS 分割成数组以处理多个参数
    read -r -a LIB_OPTS_ARRAY <<< "$LIB_OPTIONS"
    CMAKE_ARGS+=("${LIB_OPTS_ARRAY[@]}")
fi

if [[ -n "$DEPENDENCY_OPTIONS" ]]; then
    # 将 DEPENDENCY_OPTIONS 分割成数组以处理多个参数
    read -r -a DEP_OPTS_ARRAY <<< "$DEPENDENCY_OPTIONS"
    CMAKE_ARGS+=("${DEP_OPTS_ARRAY[@]}")
fi

if [[ -n "$OTHER_OPTIONS" ]]; then
    # 将 OTHER_OPTIONS 分割成数组以处理多个参数
    read -r -a OTHER_OPTS_ARRAY <<< "$OTHER_OPTIONS"
    CMAKE_ARGS+=("${OTHER_OPTS_ARRAY[@]}")
fi

if [[ "$VERBOSE" == true ]]; then
    print_info "执行命令: ${CMAKE_ARGS[*]}"
fi

if "${CMAKE_ARGS[@]}"; then
    print_success "配置成功"
else
    print_error "配置失败"
    exit 1
fi

# 构建项目
print_info "构建项目..."
if [[ "$BUILD_TYPE" == "Multi" ]]; then
    # 多配置构建 - 同时构建 Debug 和 Release
    print_info "构建 Debug 版本..."
    if ! cmake --build . --config Debug; then
        print_error "Debug 版本构建失败"
        exit 1
    fi
    
    print_info "构建 Release 版本..."
    if ! cmake --build . --config Release; then
        print_error "Release 版本构建失败"
        exit 1
    fi
    
    print_success "多配置构建成功"
else
    # 单配置构建
    if ! cmake --build .; then
        print_error "构建失败"
        exit 1
    fi
    print_success "构建成功"
fi

# 运行测试
if [[ "$RUN_TESTS" == true ]]; then
    print_info "运行测试..."
    if [[ "$BUILD_TYPE" == "Multi" ]]; then
        print_info "运行 Debug 版本测试..."
        if ! ctest --output-on-failure -C Debug; then
            print_error "Debug 版本测试失败"
            exit 1
        fi
        
        print_info "运行 Release 版本测试..."
        if ! ctest --output-on-failure -C Release; then
            print_error "Release 版本测试失败"
            exit 1
        fi
        
        print_success "所有测试通过"
    else
        if ! ctest --output-on-failure; then
            print_error "测试失败"
            exit 1
        fi
        print_success "测试通过"
    fi
fi

# 安装
if [[ "$INSTALL_BUILD" == true ]]; then
    print_info "安装项目..."
    if [[ "$BUILD_TYPE" == "Multi" ]]; then
        print_info "安装 Debug 版本..."
        if ! cmake --install . --config Debug; then
            print_error "Debug 版本安装失败"
            exit 1
        fi
        
        print_info "安装 Release 版本..."
        if ! cmake --install . --config Release; then
            print_error "Release 版本安装失败"
            exit 1
        fi
        
        print_success "所有版本安装成功"
    else
        if ! cmake --install .; then
            print_error "安装失败"
            exit 1
        fi
        print_success "安装成功"
    fi
fi

print_success "构建完成！"
print_info "构建目录: $BUILD_DIR"

# 显示生成的文件
print_info "生成的文件:"
if [[ "$BUILD_TYPE" == "Multi" ]]; then
    echo "Debug 版本:"
    find . -path "*/Debug/*" \( -name "*.a" -o -name "*.so" -o -name "*.dll" -o -name "*.exe" \) | sort
    echo "Release 版本:"
    find . -path "*/Release/*" \( -name "*.a" -o -name "*.so" -o -name "*.dll" -o -name "*.exe" \) | sort
else
    find . -name "*.a" -o -name "*.so" -o -name "*.dll" -o -name "*.exe" | sort
fi 
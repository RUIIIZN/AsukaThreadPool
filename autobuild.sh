#!/bin/bash

# autobuild.sh - 自动构建脚本

# 设置颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查是否在项目根目录
check_project_root() {
    if [[ ! -f "CMakeLists.txt" ]]; then
        print_error "未找到 CMakeLists.txt 文件，请确保在项目根目录运行此脚本"
        exit 1
    fi
}

# 创建构建目录
create_build_dir() {
    if [[ ! -d "build" ]]; then
        print_info "创建构建目录..."
        mkdir build
    else
        print_info "构建目录已存在"
    fi
}

# 构建项目
build_project() {
    print_info "进入构建目录..."
    cd build
    
    print_info "运行 CMake 配置..."
    cmake .. || {
        print_error "CMake 配置失败"
        exit 1
    }
    
    print_info "编译项目..."
    make || {
        print_error "编译失败"
        exit 1
    }
    
    cd ..
}

# 清理构建文件
clean_build() {
    if [[ -d "build" ]]; then
        print_info "清理构建目录..."
        rm -rf build/*
    fi
    
    if [[ -d "bin" ]]; then
        print_info "清理 bin 目录..."
        rm -rf bin/*
    fi
}

# 显示帮助信息
show_help() {
    echo "用法: ./autobuild.sh [选项]"
    echo "选项:"
    echo "  build     构建项目 (默认)"
    echo "  clean     清理构建文件"
    echo "  rebuild   重新构建项目"
    echo "  help      显示此帮助信息"
}

# 主逻辑
main() {
    check_project_root
    
    case "${1:-build}" in
        build)
            create_build_dir
            build_project
            ;;
        clean)
            clean_build
            ;;
        rebuild)
            clean_build
            create_build_dir
            build_project
            ;;
        help)
            show_help
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
    
    print_info "操作完成!"
}

# 执行主函数
main "$@"
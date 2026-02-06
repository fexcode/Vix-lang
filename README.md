![1770378110202](images/README/1770378110202.png)
# Vix 编程语言详细介绍

- Vix是一种轻量级的编译型脚本语言，旨在提供接近原生 C++ 的执行速度，同时保持脚本语言的简洁性和易用性。

## Why Vix?

- 🚀 接近C++的性能：通过编译为高效本地代码实现

- 🔧 多后端支持：可灵活选择C++、LLVM IR、QBE IR作为编译目标

- 📱 跨平台运行：支持Windows、Linux、macOS三大系统

- 📦 轻量级：仅需一个可执行文件

- 📝多架构支持：支持64位ARM、x86、RISC-V 、AMD 等架构
## 快速开始

1. 安装依赖

```shell
apt install gcc g++ flex bison ## ubuntu
yum install gcc gcc-c++ flex bison ## centos
brew install flex bison ## macos
pacman -S flex bison g++ gcc ## archlinux
```

### 编译

```shell
make
```

### 运行

```shell
vixc -v
```

### 第一个vix程序

```vix
fn main() ->i32
{
    print("Hello, Vix!")
}
```

```shell
vixc hello.vix -o hello
```

### 📖 更多示例
（展示更丰富的语法特性：函数、控制流、类型系统等）

### 📚 文档目录
[目录](Docs/README.md)

### 🤝 参与贡献
我们欢迎各种形式的贡献！请阅读：

### 📄 许可证
本项目基于MIT许可证开源 - 查看LICENSE文件了解详情。

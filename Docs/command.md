# Vix 编译器命令行参数文档

## 概述

Vix编译器（vixc）提供了丰富的命令行选项，用于控制编译过程、选择后端和优化级别。本文档详细介绍了所有可用的命令行参数及其用法。

## 基本用法

```shell
vixc [选项] <源文件>
```

## 命令行参数

### 编译为可执行文件

```shell
vixc test.vix -o test
```

将`test.vix`源文件编译为名为`test`的可执行文件。

- `-o <文件名>`：指定输出文件名

### 编译为QBE IR

```shell
vixc test.vix -q qbe-ir
```

将`test.vix`源文件编译为QBE中间表示（IR）代码。

- `-q <文件名>`：输出QBE IR代码到指定文件

### 初始化项目

```shell
vixc init
```

初始化一个新的Vix项目，创建基本的项目结构和配置文件。

### 编译为LLVM IR

```shell
vixc test.vix -ll test
```

将`test.vix`源文件编译为LLVM IR代码。

- `-ll <文件名>`：输出LLVM IR代码到指定文件

### 保留C++文件

```shell
vixc test.vix -kt
```

在编译过程中保留生成的C++文件，便于调试或进一步处理。

- `-kt`：保留临时C++文件

### 指定后端

```shell
vixc test.vix -o test --backend=qbe
vixc test.vix -o test --backend=cpp
vixc test.vix -o test --backend=llvm
```

指定编译器使用的后端：

- `--backend=qbe`：使用QBE后端
- `--backend=cpp`：使用C++后端
- `--backend=llvm`：使用LLVM后端

### 优化代码

```shell
vixc test.vix -q test -opt
```

对生成的SSA（静态单赋值）代码进行优化。

- `-opt`：启用代码优化

## 参数组合使用

### 编译为优化后的QBE IR

```shell
vixc test.vix -q test -opt
```

将`test.vix`编译为优化后的QBE IR代码并输出到`test`文件。

### 使用特定后端编译为可执行文件

```shell
vixc test.vix -o test --backend=qbe
vixc test.vix -o test --backend=cpp
vixc test.vix -o test --backend=llvm
```

使用指定的后端将`test.vix`编译为名为`test`的可执行文件。

### 保留中间文件并编译

```shell
vixc test.vix -o test -kt --backend=cpp
```

使用C++后端编译`test.vix`为可执行文件，并保留生成的C++文件。

## 常见用例

### 快速编译运行

```shell
vixc main.vix -o main
./main
```

### 调试编译过程

```shell
vixc test.vix -q test_ir -opt
# 查看 test_ir 文件以分析中间代码
```

### 生成LLVM IR用于分析

```shell
vixc test.vix -ll test_ir
# 使用opt或lli工具进一步处理test_ir
```

### 使用不同后端比较性能

```shell
vixc test.vix -o test_qbe --backend=qbe
vixc test.vix -o test_llvm --backend=llvm
# 比较两个可执行文件的性能
```

## 注意事项

1. 源文件必须使用`.vix`扩展名
2. 输出文件名不应包含扩展名，编译器会根据后端自动添加
3. 使用`-kt`选项时，生成的C++文件通常与源文件同名，但扩展名为`.cpp`
4. 不同后端可能支持不同的优化级别和特性
5. 对于大型项目，建议使用`vixc init`初始化项目结构

## 故障排除

### 编译错误

如果遇到编译错误，请检查：
- 源文件语法是否正确
- 是否使用了支持的语言特性
- 是否选择了正确的后端

### 后端相关问题

如果特定后端出现问题，可以尝试：
- 切换到其他后端
- 检查后端是否正确安装
- 使用`-kt`选项查看生成的中间代码

### 性能问题

如果生成的代码性能不佳，可以尝试：
- 使用`-opt`选项启用优化
- 尝试不同的后端
- 检查源代码是否使用了高效的算法和数据结构

## 相关文档

- [README.md](README.md) - Vix语言语法指南
- [ABOUTBACKEND.md](ABOUTBACKEND.md) - 后端实现说明
- [CONTRIBUTING.md](CONTRIBUTING.md) - 贡献指南
# 贡献指南

感谢您有兴趣为 Vix 语言项目做贡献！本文档提供了有关如何参与项目开发的指导。


## 入门

以下是为 Vix 语言做贡献的几种方式：

- 报告错误或提出功能建议
- 编写或改进文档
- 修复错误
- 添加新功能
- 编写测试用例
- 优化性能

## 开发环境设置

### 系统要求

- 支持 Unix/Linux/macOS/Windows 系统
- GCC/G++ 编译器
- Flex/Bison 工具
- GNU Make

### 安装依赖

Ubuntu/Debian:
```bash
sudo apt install gcc g++ flex bison make
```

CentOS/RHEL/Fedora:
```bash
sudo yum install gcc gcc-c++ flex bison make
# 或者 Fedora 上使用
sudo dnf install gcc gcc-c++ flex bison make
```

macOS:
```bash
brew install flex bison make
```

Windows (WSL/MSYS2):
```bash
pacman -S flex bison g++ gcc make
```

### 克隆和构建

```bash
# 克隆仓库
git clone https://github.com/Daweidie/vix-lang.git ##或 git clone -b windows https://gitee.com/Mulang_zty/Vix-lang.git
cd vix-lang

# 构建项目
make

# 验证安装
./vixc -v
```

## 项目结构

Vix 语言的源代码按照功能模块组织如下：

```
.
├── CHANGELOG.md
├── Docs #文档
├── LICENSE
├── README.md
├── example #测试文件
├── include #头文件部分
│   ├── ast.h
│   ├── bytecode.h
│   ├── compiler.h
│   ├── llvm_emit.h
│   ├── opt.h
│   ├── parser.h
│   ├── qbe-ir
│   │   ├── ir.h
│   │   └── opt.h
│   ├── semantic.h
│   ├── struct.h
│   ├── type_inference.h
│   └── vic-ir
│       └── mir.h
└── src
    ├── Makefile #构建脚本2
    ├── ast
    │   ├── ast.c
    │   └── type_inference.c
    ├── bootstrap #自举相关
    ├── build.sh # 编译脚本1 
    ├── bytecode
    │   └── bytecode.c
    ├── compiler
    │   ├── backend-cpp 
    │   │   └── atc.c #cpp代码生成
    │   ├── backend-llvm
    │   │   └── emit.c
    │   └── backend-qbe #qbe相关文件
    ├── lib #运行时库
    │   ├── std
    │   │   └── print.c #输出
    │   ├── vconvert.hpp #类型转换
    │   ├── vcore.hpp #核心
    │   └── vtypes.hpp #类型定义
    ├── main.c
    ├── parser
    │   ├── lexer.l #词法分析lex文件
    │   └── parser.y #语义分析bison文件
    ├── qbe
    ├── qbe-ir
    │   ├── ir.c # qbe IR
    │   ├── opt
    │   │   └── opt.c # 优化
    │   └── struct.c # 结构体
    ├── semantic
    │   └── semantic.c # 语义分析
    ├── test
    ├── test.vix # 测试文件
    ├── utils
    │   └── error.c # 错误处理
    ├── vic-ir
    │   └── mir.c # 中间表示
    └── vm
        └── vm.c #虚拟机

```

## 编码规范

### C 代码规范

1. **命名约定**：
   - 函数名和变量名：使用小写字母和下划线（snake_case）
   - 类型定义：使用大写字母开头（CamelCase）
   - 常量：全大写字母和下划线（UPPER_CASE）

2. **缩进**：
   - 使用 4 个空格进行缩进
   - 不要使用 Tab 字符

3. **注释**：
   - 为复杂逻辑添加解释性注释
   - 为公共 API 添加文档注释
   - 使用 `//` 进行单行注释，`/* */` 进行多行注释

### 提交信息规范

提交信息应遵循以下格式：

```
<type>(<scope>): <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

类型包括：
- `feat`: 新功能
- `fix`: 错误修复
- `docs`: 文档更新
- `style`: 代码样式修改（不影响代码含义）
- `refactor`: 重构（既不修复bug也不添加功能）
- `perf`: 性能改进
- `test`: 测试相关
- `chore`: 构建过程或辅助工具的变动

示例：
```
feat(parser): 添加对指针解引用的支持

- 添加 @ 操作符的语法解析
- 实现解引用的 AST 节点
- 添加相关的语义检查
创建拉取请求
``` 

## 报告问题

### 错误报告

当报告错误时，请包含以下信息：

- Vix 版本
- 操作系统
- 复现步骤
- 期望行为
- 实际行为
- 相关代码片段（如果有的话）

### 功能建议

功能建议应包括：

- 清晰的问题描述（或为什么需要此功能）
- 建议的解决方案
- 替代方案（如果有的话）
- 附加背景信息

## 社区

- 如果有疑问，请在 Issues 区提问
- 保持友好和专业的态度
- 尊重不同意见和观点

## 致谢

感谢所有为 Vix 语言项目做出贡献的人！

---

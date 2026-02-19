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
git clone https://github.com/Daweidie/vix-lang.git ## 或 git clone -b windows https://gitee.com/Mulang_zty/Vix-lang.git
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
```

## 创建拉取请求

### Fork 仓库

1. 访问 Vix 语言的 GitHub 仓库：https://github.com/Daweidie/vix-lang.git
   或 Gitee 仓库：https://gitee.com/Mulang_zty/Vix-lang.git

2. 点击页面右上角的 "Fork" 按钮，将仓库 fork 到你自己的 GitHub/Gitee 账号下

### 克隆仓库

```bash
# 克隆你 fork 的仓库
# GitHub
git clone https://github.com/你的用户名/vix-lang.git
cd vix-lang

# 或 Gitee
git clone https://gitee.com/你的用户名/Vix-lang.git
cd Vix-lang
```

### 添加上游仓库

```bash
# 添加原始仓库为上游仓库
# GitHub
git remote add upstream https://github.com/Daweidie/vix-lang.git

# 或 Gitee
git remote add upstream https://gitee.com/Mulang_zty/Vix-lang.git

# 验证远程仓库
git remote -v
```

### 创建新分支

```bash
# 确保你在主分支上
git checkout main

# 拉取最新的上游代码
git pull upstream main

# 创建并切换到新分支
git checkout -b feature/你的功能名称
# 或
git checkout -b fix/你要修复的问题
```

### 进行更改

```bash
# 进行你的代码更改
# ...

# 查看更改状态
git status

# 添加更改的文件
git add .

# 提交更改
git commit -m "feat(模块): 简短描述"
```

### 推送到你的仓库

```bash
# 推送你的分支到你的 fork 仓库
git push origin feature/你的功能名称
```

### 创建 Pull Request

1. 访问你 fork 的仓库页面
2. 点击 "New Pull Request" 按钮
3. 确保你的分支与上游仓库的主分支进行比较
4. 填写 PR 标题和描述：
   - 标题应简明扼要，遵循提交信息格式
   - 描述应详细说明你的更改，包括：
     - 更改的目的
     - 实现的方法
     - 相关的 issue（如果有）
     - 测试情况
     - 截图（如果适用）
5. 提交 Pull Request

### 保持分支更新

```bash
# 切换到主分支
git checkout main

# 拉取上游最新代码
git pull upstream main

# 切换回你的功能分支
git checkout feature/你的功能名称

# 合并主分支的更改到你的功能分支
git merge main

# 解决可能的冲突
# ...

# 推送更新后的分支
git push origin feature/你的功能名称
```

### 代码审查

1. 关注 PR 的评论和反馈
2. 根据维护者的建议进行修改
3. 推送更新后的代码
4. 等待 PR 被合并

### PR 合并后

```bash
# 切换到主分支
git checkout main

# 拉取最新的上游代码
git pull upstream main

# 删除已合并的功能分支
git branch -d feature/你的功能名称

# 删除远程分支
git push origin --delete feature/你的功能名称
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

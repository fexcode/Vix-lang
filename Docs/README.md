# Vix 编程语言语法指南

## 简介

Vix是一种静态类型、编译型编程语言，结合了函数式编程和面向对象编程的特性。它支持多种后端（LLVM、QBE等），具有简洁的语法和强大的类型系统。

## 目录

- [基本语法](#基本语法)
- [变量与类型](#变量与类型)
- [函数](#函数)
- [控制流](#控制流)
- [数组与列表](#数组与列表)
- [结构体](#结构体)
- [面向对象编程](#面向对象编程)
- [指针](#指针)
- [错误处理](#错误处理)
- [外部函数接口](#外部函数接口)
- [全局变量](#全局变量)

## 基本语法

### Hello World

```vix
print("Hello,world!")
```

### 注释

```vix
// 单行注释

/*
多行注释
*/
```

## 变量与类型

### 变量声明

```vix
a = "Hello"
b = 123
c = 3.14
```

### 类型标注

```vix
a: i32 = 10
b: f64 = 3.14
c: string = "Hello"
```

### 基本类型

- `i32` - 32位整数
- `i64` - 64位整数
- `f32` - 32位浮点数
- `f64` - 64位浮点数
- `string` - 字符串类型
- `bool` - 布尔类型
- `ptr` - 指针类型

### 可变变量

使用`mut`关键字声明可变变量：

```vix
mut x = 10
x = 20  // 可以修改
```

## 函数

### 函数定义

```vix
fn add(a: i32, b: i32) -> i32 {
    return a + b
}
```

### 主函数

```vix
fn main() -> i32 {
    print("Hello, World!")
    return 0
}
```

### 带命令行参数的主函数

```vix
fn main(argc: i32, argv: ptr) -> i32 {
    for (i in 0 .. argc) {
        print(argv[i])
    }
    return 0
}
```

## 控制流

### if语句

```vix
a = input(">")
if (a == "10") {
    print("a=10")
} elif (a == "20") {
    print("a=20")
} else {
    print("a not is 10 or 20")
}
```

### while循环

```vix
a = 0
while (a <= 10) {
    print(a)
    a += 1
}
```

### for循环

```vix
// 范围循环
for (i in 1 .. 5) {
    print(i)
}

// 嵌套循环
for (x in 1 .. 3) {
    for (y in 1 .. 3) {
        print("x:", x, "y:", y)
    }
}
```

### 逻辑运算符

```vix
a = 10
b = 20
if (a == 10 or b == 20) {
    print("a is 10 or b is 20")
}
if (a == 10 and b == 20) {
    print("a is 10 and b is 20")
}
```

## 数组与列表

### 数组

```vix
// 数组声明
a: [i32 * 5] = [1, 2, 3, 4, 5]

// 访问数组元素
print(a[0])  // 1
print(a[1])  // 2

// 修改数组元素
a[1] = 10
print(a[1])  // 10

// 数组长度
print(a.length)  // 5
```

### 数组作为函数参数

```vix
fn arr_loop(arr: [i32], len: i32) {
    for (i in 0 .. len) {
        print(arr[i])
    }
}

fn main() -> i32 {
    arr = [5, 2, 8, 1, 9, 10]
    arr_loop(arr, arr.length)
    return 0
}
```

### 列表

```vix
// 列表声明
a = ["python", "js", "vix"]

// 访问列表元素
print(a[0])  // "python"

// 列表操作
a.add!(0, "first")  // 在索引0处添加元素
b = a.remove(0)     // 移除并返回指定索引的元素
a.push!("end")      // 在末尾添加元素
c = a.pop()         // 弹出并返回最后一个元素
a.replace!(1, "new")  // 替换指定位置的元素

// 列表长度
print(a.length)
```

### 列表操作示例

```vix
a = [1, 2, 3]
print(a)       // [1, 2, 3]
a.add!(0, 4)   // [4, 1, 2, 3]
b = a.remove(0) // b = 1, a = [4, 2, 3]
a.push!(5)     // a = [4, 2, 3, 5]
c = a.pop()    // c = 5, a = [4, 2, 3]
a.replace!(1, 6) // a = [4, 6, 3]
```

## 结构体

### 结构体定义

```vix
struct Person {
    name: string,
    age: i32
}
```

### 结构体使用

```vix
fn main() -> i32 {
    // 使用初始化列表
    p1: Person {name: "John", age: 20}
    print(p1.name)
    print(p1.age)
    
    // 逐字段赋值
    p2: Person {}
    p2.name = "Jane"
    p2.age = 25
    print(p2.name)
    print(p2.age)
    
    return 0
}
```

## 面向对象编程

### 对象定义

```vix
obj PersonA(age: i32, name: string) {
    field age = age
    field name = name
    
    hello(self) {
        print("ciallo ! my name is " + self.name)
    }
    
    out_age(self) {
        print("I am " + self.age + " years old")
    }
}
```

### 接口实现

```vix
impl PersonA -> public {
    hello as greeting,
    out_age as age
}
```

### 对象使用

```vix
fn main() -> i32 {
    obj p = PersonA(20, "john")
    p.greeting()
    p.age()
    return 0
}
```

## 指针

### 取地址与解引用

```vix
// 取地址：&
x = 10
mut ptr = &x  // 获取 x 的地址，赋值需要加上 mut 修饰符

// 解引用：@
value = @ptr  // 获取 ptr 指向的值
@ptr = 20     // 修改 ptr 指向的值
```

### 指针运算

```vix
arr = [1, 2, 3, 4, 5]
p = &arr[0]
second = @(p + 1)  // 获取 arr[1] = 2
print(second)
```

### 空指针

```vix
fn main() {
    p: &i32 = nil
    if (p == nil) {
        print("nil is working...")
    }
}
```

## 错误处理

### 错误定义

```vix
maybe {
    error."No Found" {
        code: 404,
        message: "The requested resource was not found."
    }
}
```

### 错误捕获

```vix
catch error."No Found" as e {
    print("Error " + e.code + ": " + e.message)
}
```

## 外部函数接口

### 外部函数声明

```vix
extern "C" {
    fn printf(format: ptr, ...) -> i32
    fn puts(s: ptr) -> i32
}
```

### 使用外部函数

```vix
fn main(argc: i32, argv: ptr) -> i32 {
    printf("Hello World! argc = %d\n", argc)
    for (i in 1 .. argc) {
        puts(argv[i])
        printf("argv[%d] = %s\n", i, argv[i])
    }
    return 0
}
```

## 全局变量

### 全局变量声明

```vix
global counter = 42
global message: str = "Hello World"
global pi: f64 = 3.14159
```

### 使用全局变量

```vix
print(counter)
print(message)
print(pi)
```

## 类型转换

### 整数转换

```vix
a = toint("123")  // 字符串转整数
```

### 浮点数转换

```vix
b = tofloat("3.14")  // 字符串转浮点数
```

## 输入输出

### 输入

```vix
a = input(">")
```

### 输出

```vix
print("Hello, World!")
print("Value:", 42)
```

## 实用示例

### 斐波那契数列

```vix
fn fib(n: i32) -> i64 {
    a = 0
    b = 1
    for (i in 1 .. n) {
        c = a + b
        a = b
        b = c
    }
    return b
}

fn main() -> i32 {
    print(fib(40))
}
```

### 交换两个值

```vix
fn swap(a: i32, b: i32) -> i32 {
    temp = a
    a = b
    b = temp
    print("a:", a, "b:", b)
    return 0
}
```

## 更多示例

更多示例代码可以在项目的`examples`目录中找到，包括：
- 基本语法示例
- 数据结构示例
- 控制流示例
- 函数示例
- 面向对象示例
- 指针操作示例
- 错误处理示例

## 贡献

欢迎贡献代码和文档！请查看[CONTRIBUTING.md](CONTRIBUTING.md)了解如何参与项目开发。

## 许可证

本项目采用开源许可证，具体信息请查看[LICENSE](LICENSE)文件。

## 版本历史

详细的版本变更历史请查看[CHANGELOG.md](CHANGELOG.md)。

## 相关文档

- [CONTRIBUTING.md](CONTRIBUTING.md) - 贡献指南
- [ABOUTBACKEND.md](ABOUTBACKEND.md) - 后端实现说明
- [DIFF.md](DIFF.md) - 与不同后端的差异
- [command.md](command.md) - 命令行工具使用说明
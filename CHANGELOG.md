### 26/1/24 更新日志

1. 更新了错误提示
2. 更新了qbe ir的生成，从vic ir到qbe ir ，而不是从bytcode到qbe ir
3. 更新const关键字（包括错误提示）
4. 修复了toint/tofloat函数的cpp代码生成

- will do
  修复input的seg bug

### 26/1/25 更新日志

1. 更新列表

- will do
  指针

### 26/1/26 更新日志

1. 更新列表操作(add,remove,pop,push)

- will do
  指针和结构体

### 26/1/27 更新日志

1. 修复对于pop操作的bug
2. 更新结构体语法
3. 更新函数参数类型后置语法
4. 更新结构体的错误提示

- will do
  指针

### 26/2/5 更新日志
1. 添加指针语法
2. 添加指针错误提示
3. 基本支持qbe后端
4. qbe后端的opt未完善，可能有优化问题

- will do
  完善qbe后端

### 26/2/6 更新日志
1. 实现嵌套结构体的cpp代码生成

- will do
  完善qbe后端

### 26/2/7 更新日志
1. 修改struct的初始化语法 ' : '变为  ' = '
2. 更新for循环的语法 可以 for(i ; 1 .. 10)或for(i in 1 .. 10)

- will do
  完善qbe后端

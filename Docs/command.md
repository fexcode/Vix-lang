目前实现的命令行参数：
```shell
vixc test.vix -o test   #编译为可执行文件
vixc test.vix -q qbe-ir     #编译为qbe ir
vixc init   #初始化项目
vixc test.vix -ll test  #编译为llvm ir
vixc test.vix -kt   #保留cpp文件
vixc test.vix -o test --backend=qbe/cpp/llvm 
vixc test.vix -q test -opt   #优化生成了ssa代码
```
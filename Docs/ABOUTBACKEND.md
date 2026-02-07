qbe 后端 ：速度约为gcc o2的0.9倍
当你使用for循环时:
```vix
for (i in 1 .. 10)
{
    print(i)
}
```
从1到9的数字会打印出来

但是cpp后端:
```vix
for (i in 1 .. 10)
{
    print(i)
}
```
从1到10的数字会打印出来
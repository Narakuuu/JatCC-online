# JatCC-online
基于C语言并通过自举实现的在线编译系统，编译器前后端合一采用单趟(one-pass)解析，生成的目标代码基于自定义的虚拟机。该C语言在线编译系统经过webbench压力测试可以实现近万的QPS。

## 功能
* 一个能够自举在线的C编译器，受[c4](https://github.com/rswier/c4)的启发；
* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；

## 编译器特性
* 前后端合一，无中间优化；
* 目标代码基于自定义的虚拟机（VM），采用单趟（One-Pass）解析过程：源代码经过解析和代码生成过程直接生成虚拟机指令，然后由虚拟机来执行
* 可以使用GCC或者MSVC来编译，支持32位或者64位

`jatcc.c`（仅仅是c4的重构）支持的C语言特性：
- 基本类型：`char`，`int`，指针，匿名`enum`
- 控制流：`if-else`，`while`
- 运算符：`, = ?: || && | ^ & == != < > <= >= << >> + - * / % ++ -- [] ()`
- 函数定义
    - 支持全局变量和局部变量声明。
    - 支持使用指针进行的一切操作。
    - 支持前向声明。
    - 按照 `sizeof(int)` 字节对齐。

`jatccex.c`添加的扩展：
- 控制流：完美支持 `for` `do while` `break` `continue` `goto`
- 命名`enum`支持，简单等同于`int`
- 有限地支持了`union`和`struct`的核心特性，添加运算符`. ->`

jatcc.c作为基础版本不再改动。

## 实现细节
- [jatcc.c的实现细节与学习笔记](./code/compiler/jatcc.md)



## 环境要求
* Linux
* C++14
* 服务器程序运行端口默认使用1316，运行前请查看是否占用该端口


## 项目启动
```bash
make
./bin/server
```
> * __样例1-hello.c
```bash
#include <stdio.h>

int main () {
    printf("Hello World\n");
    return 0;
}
```
![image](https://github.com/Narakuuu/IMG/blob/main/hello.gif)

> * __样例2-fibonacci.c
```bash
#include <stdio.h>

int fibonacci(int i) {
    if (i <= 1) {
        return 1;
    }
    return fibonacci(i-1) + fibonacci(i-2);
}

int main() {
    int i;
    i = 0;
    while (i <= 10) {
        printf("fibonacci(%2d) = %d\n", i, fibonacci(i));
        i = i + 1;
    }
    return 0;
}
```
![image](https://github.com/Narakuuu/IMG/blob/main/fibonacci.gif)

## Webbench测试结果
```bash
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://192.168.220.128:1316/
7000 clients, running 10 sec.

Speed=611427 pages/min, 30136884 bytes/sec.
Requests: 100208 susceed, 0 failed.
```

## 致谢
《Linux高性能服务器编程》，游双著.
  
[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)：TinyWebServer开源项目
  
[@Robert Swierczek](https://github.com/rswier/c4)：c4开源项目

[手把手教你构建 C 语言编译器](https://lotabout.me/2015/write-a-C-interpreter-0/)系列，[lotabout/write-a-C-interpreter](https://github.com/lotabout/write-a-C-interpreter)
  
更多关于C4以及实现一个C编译器相关的细节：[知乎-RednaxelaFX关于C4的文章](https://www.zhihu.com/question/28249756/answer/84307453)

## TODO
- float & double ? 虚拟机没有实现浮点指令，需要扩展指令集，现有框架能否完整且良好地支持还不好说，暂不实现。




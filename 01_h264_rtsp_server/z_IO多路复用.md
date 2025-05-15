# IO 零 copy：writev() 和 readv()
*  可以先看看 [linux IO 零 copy](https://strikefreedom.top/archives/linux-io-stack-and-zero-copy)

* writev() 和 readv() 系统调用
> writev() 和 readv() 是 UNIX/Linux 系统中用于分散/聚集 I/O (scatter/gather I/O) 的系统调用，它们允许程序在一次系统调用中从多个缓冲区读取或写入数据。
### 1. writev() - 聚集写入
   * 功能
     * writev() 将多个缓冲区的数据聚集起来，然后一次性写入文件描述符。

* 函数原型
c
```c
#include <sys/uio.h>

ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

```
* 参数说明
  * fd: 文件描述符
  * iov: 指向 iovec 结构体数组的指针
  * iovcnt: iov 数组中的元素个数

* iovec 结构体
c
```c
struct iovec {
void  *iov_base;  /* 缓冲区起始地址 */
size_t iov_len;   /* 缓冲区长度 */
};
```
* 返回值
  * 成功时返回写入的总字节数
  * 失败时返回 -1 并设置 errno

* 示例

```c
struct iovec iov[3];
char *part1 = "Header: ";
char *part2 = "Body content";
char *part3 = "\nFooter";

iov[0].iov_base = part1;
iov[0].iov_len = strlen(part1);
iov[1].iov_base = part2;
iov[1].iov_len = strlen(part2);
iov[2].iov_base = part3;
iov[2].iov_len = strlen(part3);

ssize_t nwritten = writev(fd, iov, 3);
```
* 优势
  * 减少系统调用次数（将多次 write() 合并为一次 writev()）
  * 提高I/O效率，特别是对于网络套接字

### 2. readv() - 分散读取
   * 功能
     * readv() 从文件描述符读取数据并分散存储到多个缓冲区中。
* 函数原型
```c
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
```
* 参数说明
  * 与 writev() 相同

* 返回值
  * 成功时返回读取的总字节数
  * 到达文件末尾返回 0
  * 失败时返回 -1 并设置 errno

* 示例
```c
struct iovec iov[2];
char header[10];
char body[1024];

iov[0].iov_base = header;
iov[0].iov_len = sizeof(header);
iov[1].iov_base = body;
iov[1].iov_len = sizeof(body);

ssize_t nread = readv(fd, iov, 2);
```
* 优势
  * 可以一次性读取数据到多个缓冲区
  * 适用于需要按固定格式解析数据的场景

### 3. 应用场景
   * 网络编程
     * 发送HTTP响应头和数据体
     * 处理固定格式的网络协议
   * 文件I/O
     * 同时读取文件的多个部分到不同缓冲区
     * 将多个数据结构一次性写入文件

* 性能优化
  * 减少系统调用次数
  * 避免不必要的数据拷贝（相比先合并再发送）

### 4. 注意事项
   * 原子性：对于普通文件，writev() **是原子的**（要么全部写入成功，要么全部失败）
   * 顺序性：数据按 iov 数组顺序处理
   * 缓冲区限制：Linux 中 iovcnt 的最大值通常为 1024 (IOV_MAX)
   * 部分写入/读取：可能只处理部分数据，需要检查返回值

### v5. 与传统 I/O 的比较
   特性	write()/read()	writev()/readv()
   缓冲区	单个	多个
   系统调用次数	多	少
   数据拷贝	可能需要合并	直接操作
   适用场景	简单I/O	结构化I/O
   writev() 和 readv() 特别适合需要处理多个不连续缓冲区的场景，是高性能网络编程中的重要工具。
   


|特性	|write()/read()	| writev()/readv() |
|:--------|:--------------:|:----------------:|
|缓冲区	|单个	|        多个        |
|系统调用次数	|多	|        少         |
|数据拷贝	|可能需要合并	|       直接操作       |
|适用场景	|简单I/O	|      结构化I/O      |

* writev() 和 readv() 特别适合需要处理多个不连续缓冲区的场景，是高性能网络编程中的重要工具。


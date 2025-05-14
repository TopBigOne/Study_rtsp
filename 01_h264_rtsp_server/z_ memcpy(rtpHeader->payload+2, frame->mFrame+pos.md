### 以下代码中的rtpHeader->payload+2，可以改为rtpHeader->payload[2] 吗？
*  memcpy(rtpHeader->payload+2, frame->mFrame+pos, RTP_MAX_PKT_SIZE);
### 根本区别
   | 表达式	| 类型	| 实际含义|
   |:-----:|:--------------------------|:--------------------------|
   | payload+2	| uint8_t* (指针)	| 指向payload起始地址后2字节的内存地址|
   | payload[2]	| uint8_t (单字节)	| payload数组的第3个元素（值）|
### 为什么不能替换？
   * memcpy的参数要求
   m* emcpy的第一个参数需要是目标内存地址，而：

     * payload+2 是合法的内存地址（如 0x7ffd42a8）
     * payload[2] 是单个字节的值（如 0x67），若强行传入会导致：

```c
memcpy(0x67, ...); // 尝试拷贝到非法地址0x67，触发段错误
```
* 语法等价关系
  * 虽然 payload[2] 等价于 *(payload+2)，但：
    * *(payload+2) 是解引用操作，得到的是值
    * payload+2 是地址运算，得到的是指针

* 特殊情况例外
  * 如果一定要用[]语法，可以改为：

```c
memcpy(&rtpHeader->payload[2], ...); // 取第3个元素的地址
```
> 但这不如直接 payload+2 直观。
* 在H264分片传输中：
```c
memcpy(rtpHeader->payload + 2,      // 目标：跳过FU Indicator和Header
frame->mFrame + pos,                // 源：从帧数据的pos位置开始
RTP_MAX_PKT_SIZE);                  // 拷贝固定大小的分片
```
   *  payload+2 指向分片数据的存储位置（紧接在2字节的FU头部之后）
   * 这种写法是网络编程中的经典模式（直接指针运算高效且明确）

### 类比其他场景


* 1: 字符串处理
```c
char str[10];
memcpy(str + 2, "hello", 5); // 从str[2]开始填充
```
* 2: 结构体偏移

```c
struct Data { int a; char buf[0]; };
Data* d = malloc(sizeof(Data) + 100);
memcpy(d->buf + 10, source, 50); // 从buf[10]开始写入
```
### 总结建议
* 保持原代码：payload+2 是最专业且高效的写法

* 如需提高可读性：可以添加注释说明偏移原因


```c
// RFC 6185: FU-A分片需要2字节头部(FU Indicator + FU Header)
memcpy(rtpHeader->payload + 2, ...);
```
* 调试技巧：用gdb打印指针值验证

```bash
(gdb) p rtpHeader->payload+2
$1 = (uint8_t *) 0x7ffff7ffb002
(gdb) x/4xb rtpHeader->payload
0x7ffff7ffb000: 0x7c 0x85 0x00 0x00  # 前两字节是头部(0x7c,0x85)
```
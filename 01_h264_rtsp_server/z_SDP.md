### SDP示例
```sdp
v=0
o=- 9123456789 1 IN IP4 192.168.1.100
t=0 0
a=control:*
a=type:broadcast
m=video 0 RTP/AVP 96
c=IN IP4 0.0.0.0
a=rtpmap:96 H264/90000
a=control:track0
m=audio 0 RTP/AVP 97
c=IN IP4 0.0.0.0
a=rtpmap:97 mpeg4-generic/44100/2
a=control:track1
```

### m=、c=、a= 行 是啥？

> 在SDP（Session Description Protocol）协议中，m=、c=、a= 是描述媒体会话的关键行，每种行都有特定用途和格式要求。以下是详细说明：
### 1. m= 行（媒体描述）
   * 格式：
   
  ```sdp
 m=<media> <port> <proto> <fmt> ...
```
   * 示例：
  ```sdp
 m=video 49170 RTP/AVP 96
```
* 字段解析：
* 
  | 字段     | 含义     | 常见值                                       |
  | :------- | :------: | :----------------------------------------: |
  |  \<media\>   | 媒体类型  | video/audio/text/application                |
  | \<port\>  | 传输端口  | 单播时为0（动态分配），多播时为实际端口     |
  | \<proto\>  | 传输协议  | RTP/AVP（RTP Profile）UDP/TLS/RTP/SAVPF（加密传输） |
  | \<fmt\>   | 负载格式  | 动态负载类型（如96=H264）静态类型（如0=PCMU） |

   * 作用：
     * 定义媒体流的类型、传输方式和编解码格式。

### 2. c= 行（连接信息）
   * 格式：

  ```sdp
 c=<nettype> <addrtype> <connection-address>
  ```
   * 示例：
   ```sdp
c=IN IP4 239.255.0.1/255
```
   * 字段解析：
   * 
   |字段	|含义	|                    常见值                     |
   | :------- | :------: |:------------------------------------------:|
   |\<nettype>	|网络类型	|                IN（Internet）                |
   |\<addrtype>	|地址类型	|                  IP4/IP6                   |
   |\<connection-address>	|连接地址	| 单播：0.0.0.0 多播：组播IP/TTL（如 239.255.0.1/255）  |
   
   作用：
   指定媒体流的网络连接地址（单播或多播）。

### 3. a= 行（属性行）
   * 常见类型及示例：

   |属性类型	|示例|	作用|
   | :------- | :------: |:------------------------------------------:|
   |编解码参数	|a=rtpmap:96 H264/90000	|定义动态负载类型的具体参数|
   |控制URL	|a=control:track1	|指定媒体轨道的控制路径（用于RTSP交互）|
   |帧率	|a=framerate:30	|视频帧率|
   |多播控制	|a=rtcp-unicast: reflection	|支持RTCP单播反射|
   |其他标记|	a=sendrecv	|媒体流方向（sendrecv/recvonly/sendonly/inactive）|
   * 作用：
     * 提供媒体流的扩展属性，是SDP中最灵活的部分。

### 完整SDP示例
```sdp
v=0
o=- 123456789 1 IN IP4 192.168.1.100
t=0 0
a=control:*
m=video 0 RTP/AVP 96
c=IN IP4 0.0.0.0
a=rtpmap:96 H264/90000
a=control:track0
m=audio 0 RTP/AVP 97
c=IN IP4 0.0.0.0
a=rtpmap:97 mpeg4-generic/44100/2
a=control:track1

```
### 关键区别总结

 行类型	|必须性	|出现次数	|典型位置|
|:---| :------: |:------------------------------------------:|:------------------------------------------:|
| m=	|必需	|每个媒体流1次	|媒体块开头|
| c=	|可选|（全局默认）	|每媒体流0-1次	m=行之后|
| a=	|可选	|每媒体流多次	|m=或c=行之后|
### 协议规范依据
* RFC 4566：SDP协议核心规范
* RFC 2327：RTSP中的SDP扩展
* RFC 6184：H264的RTP负载格式（定义a=rtpmap参数）

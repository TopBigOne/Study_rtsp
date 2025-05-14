> 要学 Sink ,得先学 [live555](https://github.com/rgaufman/live555)
> 在 RTSP（Real-Time Streaming Protocol）或基于 RTSP 的流媒体框架（如 Live555）中，**Sink（接收器）**是一个关键的概念，尤其在媒体数据的处理链中


# Sink 是什么？
* 在流媒体系统中，Sink 通常是指：

    > 接收并处理媒体数据的组件。

* 它的职责是从某个 Source（源） 获取媒体帧（如音频、视频），然后 处理、转发、保存 或 显示 这些数据。

* 在 Live555 这样的 RTSP 框架中：

* MediaSink 是一个抽象类，定义了如何处理收到的媒体帧。
* 常见的派生类有：
* RTPSink：用于 RTP 打包并发送数据。
* FileSink：写入文件。
* DummySink：调试用，仅打印或处理数据但不输出。
* RTSPClientSink：客户端播放时用来处理接收到的流。
#  Sink 解决了什么问题？

1. 解耦媒体处理流程
   * Sink 将 数据的产生（Source） 和 数据的使用/输出（Sink） 分离，形成清晰的数据流管道。

   * Source 负责采集或读取媒体（例：摄像头、文件）。
   * Sink 负责如何处理这些媒体（例：发送、存储、显示）。
   * 这样可以灵活组合，比如：
     * 同一个 Source 可以连接不同的 Sink。
     * Sink 的行为可以定制如转码、加密、缓存等。
2. 支持多路输出
   * 一个 Source 可以连接多个 Sink，实现如： 
     * 同时向多个客户端推送音视频。 
     * 一边推流，一边本地存储。
3. 提高扩展性
   * 通过继承和实现不同的 MediaSink，你可以轻松扩展系统功能。例如： 
     * 实现一个新的 MyCustomSink，用于 AI 分析视频帧。 
     * 实现 WebSocketSink，将视频通过 WebSocket 发给浏览器。
> 读到这儿，你是不是会感慨，这TM和java的策略模式+多态 有啥区别？
> 
# 示例：Live555 中的 Sink 使用流程
* RTSP Server 推送视频流的通用流程：
> [ VideoSource ] → [ RTPSink ] → [ 客户端 ]
* 代码示例（伪代码）
```c++
FramedSource* videoSource = createH264VideoSource();
RTPSink* videoSink = H264VideoRTPSink::createNew(env, rtpGroupsock);

// 将 source 和 sink 连接
videoSink->startPlaying(*videoSource, afterPlayingCallback);
```



|  名称	  | 描述                        |
|:-----:|:--------------------------|
| Sink	 | 接收和处理媒体数据的组件              
|  作用	  | 解耦 Source 与处理逻辑，支持多路输出与扩展 |
| 典型用途	 | RTP 推流、文件保存、实时播放、数据分析等    |
### 基于此建立 RTSP/SIP 服务器和客户端来实现多媒体流的传输

*  (1）客户端发起 RTSP OPTION 请求，目的是得到服务器提供什么方法。RTSP 提供的方法一般包括 OPTIONS、DESCRIBE、SETUP、TEARDOWN、PLAY、PAUSE、SCALE、GET_PARAMETER。
* （2）服务器对 RTSP OPTION 回应，服务器实现什么方法就回应哪些方法。在此系统中，我们只对 DESCRIBE、SETUP、TEARDOWN、PLAY、PAUSE 方法做了实现。
* （3）客户端发起 RTSP DESCRIBE 请求，服务器收到的信息主要有媒体的名字，解码类型，视频分辨率等描述，目的是为了从服务器那里得到会话描述信息（SDP）。
* （4）服务器对 RTSP DESCRIBE 响应，发送必要的媒体参数，在传输 H.264 文件时，主要包括 SPS/PPS、媒体名、传输协议等信息。
* （5）客户端发起 RTSP SETUP 请求，目的是请求会话建立并准备传输。请求信息主要包括传输协议和客户端端口号。
* （6）服务器对 RTSP SETUP 响应，发出相应服务器端的端口号和会话标识符。
* （7）客户端发出了 RTSP PLAY 的请求，目的是请求播放视频流。
* （8）服务器对 RTSP PLAY 响应，响应的消息包括会话标识符，RTP 包的序列号，时间戳。此时服务器对 H264 视频流封装打包进行传输。
* （9）客户端发出 RTSP TEARDOWN 请求，目的是关闭连接，终止传输。（10）服务器关闭连接，停止传输。
### 重要的首部字段 
* CSeq： 指定了RTSP请求响应对的序列号，对每个包含一个给定序列号的请求消息，都会有一个相同序列号的回应消息，且每个请求或回应中都必须包括这个头字段
* Content-Length：该字段指明在RTSP协议最后一个标头之后的双 CRLF 之后的内容长度。例如在服务器响应DESCRIBE中，指明sdp信息长度
* Content-Type:告诉客户端实际返回的内容的类型

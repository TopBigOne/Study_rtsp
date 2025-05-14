#ifndef _MEDIA_SINK_H_
#define _MEDIA_SINK_H_
#include <string>
#include <stdint.h>

#include "MediaSource.h"
#include "Event.h"
#include "UsageEnvironment.h"
#include "Rtp.h"


class RtpSink
{
public:
    // 定义发送数据包回调函数类型
    // 参数1: 自定义参数1
    // 参数2: 自定义参数2
    // 参数3: RTP数据包指针
    typedef void (*SendPacketCallback)(void* arg1, void* arg2, RtpPacket* mediaPacket);

    // 构造函数
    // env: 使用环境(通常包含调度器和线程池等)
    // mediaSource: 媒体数据源
    // payloadType: RTP负载类型(如96表示H264)
    RtpSink(UsageEnvironment* env, MediaSource* mediaSource, int payloadType);
    virtual ~RtpSink();
    // 纯虚函数 - 获取媒体描述信息(SDP中的m=行)
    virtual std::string getMediaDescription(uint16_t port) = 0;
    // 纯虚函数 - 获取媒体属性(SDP中的a=行)
    virtual std::string getAttribute() = 0;

    // 设置发送帧回调函数
    // cb: 回调函数指针
    // arg1: 自定义参数1
    // arg2: 自定义参数2
    void setSendFrameCallback(SendPacketCallback cb, void* arg1, void* arg2);

protected:
    // 纯虚函数 - 处理媒体帧(由子类实现具体编码/打包逻辑)
    // frame: 原始媒体帧数据
    virtual void handleFrame(AVFrame* frame) = 0;
    // 发送RTP数据包
    // packet: 要发送的RTP包
    void sendRtpPacket(RtpPacket* packet);
    // 开始传输(启动定时器)
    // ms: 发送间隔(毫秒)
    void start(int ms);
    void stop();

private:
    static void timeoutCallback(void*);

protected:
    UsageEnvironment* mEnv;          // 使用环境指针
    MediaSource* mMediaSource;       // 媒体源指针
    SendPacketCallback mSendPacketCallback; // 发送数据包回调
    void* mArg1;                     // 回调参数1
    void* mArg2;                     // 回调参数2

    // RTP头部相关字段
    uint8_t mCsrcLen;                // CSRC计数器(通常为0)
    uint8_t mExtension;              // 扩展标志位
    uint8_t mPadding;                // 填充标志位
    uint8_t mVersion;                // RTP版本(通常为2)
    uint8_t mPayloadType;            // 负载类型(如96)
    uint8_t mMarker;                 // 标记位(帧结束标志)
    uint16_t mSeq;                   // 序列号(每包递增)
    uint32_t mTimestamp;             // 时间戳(根据采样率递增)
    uint32_t mSSRC;                  // 同步源标识符(随机值)

private:
    TimerEvent* mTimerEvent;         // 定时器事件
    Timer::TimerId mTimerId;         // 定时器ID
};

#endif //_MEDIA_SINK_H_
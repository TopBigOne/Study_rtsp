#include <stdio.h>
#include <string.h>

#include "H264RtpSink.h"
#include "Logging.h"
#include "New.h"

H264RtpSink* H264RtpSink::createNew(UsageEnvironment* env, MediaSource* mediaSource)
{
    if(!mediaSource)
        return NULL;

    //return new H264RtpSink(env, mediaSource);
    return New<H264RtpSink>::allocate(env, mediaSource);
}

H264RtpSink::H264RtpSink(UsageEnvironment* env, MediaSource* mediaSource) :
    RtpSink(env, mediaSource, RTP_PAYLOAD_TYPE_H264),
    mClockRate(90000),
    mFps(mediaSource->getFps())
{
    start(1000/mFps);
}

H264RtpSink::~H264RtpSink()
{

}

/**
 * 生成SDP媒体描述(m=行)
 * @param port
 * @return
 */
std::string H264RtpSink::getMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    // 格式: m=video <port> RTP/AVP <payload_type>
    sprintf(buf, "m=video %hu RTP/AVP %d", port, mPayloadType);

    return std::string(buf);
}

/**
 * 生成SDP属性(a=行)
 * @return
 */
std::string H264RtpSink::getAttribute()
{
    char buf[100];
    // 格式: a=rtpmap:<payload_type> H264/<clock_rate>
    sprintf(buf, "a=rtpmap:%d H264/%d\r\n", mPayloadType, mClockRate);
    // 添加帧率属性
    sprintf(buf+strlen(buf), "a=framerate:%d", mFps);

    return std::string(buf);
}

/**
 * 处理H264帧，封装为RTP包
 * @param frame
 */
void H264RtpSink::handleFrame(AVFrame* frame)
{
    RtpHeader* rtpHeader = mRtpPacket.mRtpHeadr;
    // 获取NALU头部的类型
    uint8_t naluType = frame->mFrame[0];

    // 情况1: 帧大小小于等于RTP最大包大小(单一NALU模式)
    if(frame->mFrameSize <= RTP_MAX_PKT_SIZE)
    {
        // 直接拷贝整个NALU到RTP负载
        memcpy(rtpHeader->payload, frame->mFrame, frame->mFrameSize);
        mRtpPacket.mSize = frame->mFrameSize;
        sendRtpPacket(&mRtpPacket);
        mSeq++;

        // 如果是SPS/PPS NALU，不更新时间戳(这些参数集不占用时间戳)，0x1F = 00011111（取低5位）
        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) // 如果是SPS、PPS就不需要加时间戳
            return;
    }
    // 情况2: 帧大小超过RTP最大包大小(FU-A分片模式)
    else
    {
        // 完整包数量
        int pktNum = frame->mFrameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        // 剩余数据大小
        int remainPktSize = frame->mFrameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        // pos=1跳过NALU头部
        int i, pos = 1;

        /* 发送完整的包 */
        for (i = 0; i < pktNum; i++)
        {
            /*
            *     FU Indicator
            *    0 1 2 3 4 5 6 7
            *   +-+-+-+-+-+-+-+-+
            *   |F|NRI|  Type   |
            *   +---------------+
            * */
            /* 设置FU Indicator(分片指示头)
             * */
            // 格式: |F|NRI|Type(28=分片)|
            rtpHeader->payload[0] = (naluType & 0x60) | 28; //(naluType & 0x60)表示nalu的重要性，28表示为分片
           //  naluType & 0x60：
           // 解释： 十六进制0x60 = 二进制01100000，这个掩码会提取出NALU头中的F和NRI位（即保留高3位，清除低5位）
           // | 28：
           //  解释： 十进制28 = 二进制00011100，这是FU-A分片的类型号。按位或操作将分片类型与保留的F/NRI位合并
            
            /*
            *      FU Header
            *    0 1 2 3 4 5 6 7
            *   +-+-+-+-+-+-+-+-+
            *   |S|E|R|  Type   |
            *   +---------------+
            * */
            /* 设置FU Header(分片头) */
            // 格式: |S|E|R|Type|
            // S（Start）：是否是分片的开始（1为开始）
            // E（End）：是否是分片的结束（1为结束）
            // R：保留位，设为0
            // Type：原始 NAL 单元的类型（如 5=IDR，1=非IDR）
            rtpHeader->payload[1] = naluType & 0x1F;// 原始NALU类型 ，0x1F = 00011111（取低5位）
            //  naluType & 0x1F : 取低5位

            // // 第一个分片 注意是|= 运算
            if (i == 0) //第一包数据
                rtpHeader->payload[1] |= 0x80; // start
            else if (remainPktSize == 0 && i == pktNum - 1) //最后一包数据
                rtpHeader->payload[1] |= 0x40; // end ， 01000000 (设置E位)

                // rtpHeader->payload+2 : 跳过 Fu Indicator 和 Fu header
            memcpy(rtpHeader->payload+2, frame->mFrame+pos, RTP_MAX_PKT_SIZE);
            mRtpPacket.mSize = RTP_MAX_PKT_SIZE+2;// +2是FU头和分片头
            sendRtpPacket(&mRtpPacket);

            mSeq++;// 序列号递增
            pos += RTP_MAX_PKT_SIZE;// 移动数据指针
        }

        /* 发送剩余的数据 */
        if (remainPktSize > 0)
        {

            rtpHeader->payload[0] = (naluType & 0x60) | 28;// FU Indicator
            rtpHeader->payload[1] = naluType & 0x1F;// FU Header
            // |= 运算
            rtpHeader->payload[1] |= 0x40; //end

            // payload+2 ????
            // payload[0]：已经存储了FU Indicator（分片类型标记）
            // payload[1]：已经存储了FU Header（包含S/E标志和NALU类型）
            // payload + 2：跳过前两个字节，指向分片数据的起始位置
            memcpy(rtpHeader->payload+2,
                   frame->mFrame+pos,
                   remainPktSize);
            mRtpPacket.mSize = remainPktSize+2;
            sendRtpPacket(&mRtpPacket);

            mSeq++;
        }
    }
    // 更新时间戳(按帧率递增)
    mTimestamp += mClockRate/mFps;
}

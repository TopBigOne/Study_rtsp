#ifndef _RTP_H_
#define _RTP_H_
#include <stdint.h>
#include <stdlib.h>

#define RTP_VESION              2

#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97

#define RTP_HEADER_SIZE         12
#define RTP_MAX_PKT_SIZE        1400

struct RtpHeader
{
    /* 字节 0 */
    uint8_t csrcLen:4;     // CSRC计数器：表示CSRC标识符的个数（0-15）
    uint8_t extension:1;   // 扩展标志：1=头部有扩展字段，0=无扩展
    uint8_t padding:1;     // 填充标志：1=包尾有填充字节（用于对齐）
    uint8_t version:2;     // 版本号：通常为2（二进制10）

    /* 字节 1 */
    uint8_t payloadType:7; // 负载类型：如96=H264，参见RFC3551
    uint8_t marker:1;      // 标记位：视频流中通常标识帧结束

    /* 字节 2-3 (网络字节序) */
    uint16_t seq;          // 序列号：每发送一个RTP包递增1（用于检测丢包）

    /* 字节 4-7 (网络字节序) */
    uint32_t timestamp;    // 时间戳：基于采样频率的媒体时间（单位：时钟周期）

    /* 字节 8-11 (网络字节序) */
    uint32_t ssrc;         // 同步源标识符：随机生成的唯一ID

    /* 可变长度部分（C99柔性数组技术）*/
    uint8_t payload[0];    // 负载数据起始位置（实际不占空间）
    // 注：payload[0]是C99柔性数组写法，实际存储时紧跟在头部之后
};

class RtpPacket
{
public:
    RtpPacket() :
        _mBuffer(new uint8_t[RTP_MAX_PKT_SIZE+RTP_HEADER_SIZE+100]),
        mBuffer(_mBuffer+4),
        mRtpHeadr((RtpHeader*)mBuffer),
        mSize(0)
    {
        
    }

    ~RtpPacket()
    {
        delete _mBuffer;
    }

    uint8_t* _mBuffer;
    uint8_t* mBuffer;
    RtpHeader* const mRtpHeadr;
    int mSize;
};

#endif //_RTP_H_
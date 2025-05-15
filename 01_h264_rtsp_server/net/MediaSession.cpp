#include <stdio.h>
#include <time.h>
#include <string.h>
#include <algorithm>
#include <assert.h>

#include "MediaSession.h"
#include "SocketsOps.h"
#include "Logging.h"
#include "New.h"
#include "RtpSink.h"

MediaSession* MediaSession::createNew(std::string sessionName)
{
    //return new MediaSession(sessionName);
    return New<MediaSession>::allocate(sessionName);
}

MediaSession::MediaSession(const std::string& sessionName) :
    mSessionName(sessionName),
    mIsStartMulticast(false)
{
    mTracks[0].mTrackId = TrackId0;
    mTracks[1].mTrackId = TrackId1;
    mTracks[0].mIsAlive = false;
    mTracks[1].mIsAlive = false;

    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        mMulticastRtpInstances[i] = NULL;
        mMulticastRtcpInstances[i] = NULL;
    }
}

MediaSession::~MediaSession()
{
    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if(mMulticastRtpInstances[i])
        {
            this->removeRtpInstance(mMulticastRtpInstances[i]);
            //delete mMulticastRtpInstances[i];
            Delete::release(mMulticastRtpInstances[i]);
        }

        if(mMulticastRtcpInstances[i])
            Delete::release(mMulticastRtcpInstances[i]);
            //delete mMulticastRtcpInstances[i];
    }
}

/**
 * 动态生成一个 SDP 字符串，用于描述媒体会话的配置信息（如支持的媒体类型、传输地址、编解码参数等），主要应用于 RTSP/SDP 协议交互中
 * @return
 */
std::string MediaSession::generateSDPDescription()
{
    // 避免重复生成 SDP（性能优化）
    if(!mSdp.empty()){
        return mSdp;
    }

    
    std::string ip = sockets::getLocalIp();
    char buf[2048] = {0};

    snprintf(buf, sizeof(buf),
        "v=0\r\n"               // SDP版本号
        "o=- 9%ld 1 IN IP4 %s\r\n"    // 会话标识：用户名|会话ID|版本|网络类型|地址类型|IP
        "t=0 0\r\n"                   // 时间范围（0表示持久会话）
        "a=control:*\r\n"             // 控制URL（*表示使用请求的URL）
        "a=type:broadcast\r\n",       // 会话类型（广播）
        (long)time(NULL),       // 用时间戳作为会话ID
        ip.c_str());

    // 多播模式特殊处理
    if(isStartMulticast())
    {
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                "a=rtcp-unicast: reflection\r\n");// 支持RTCP单播反射
    }

    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        uint16_t port = 0;

        if(mTracks[i].mIsAlive != true)
            continue;

        if(isStartMulticast())
            port = getMulticastDestRtpPort((TrackId)i);
        
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                    "%s\r\n", mTracks[i].mRtpSink->getMediaDescription(port).c_str());

        // 添加连接信息（c=行）
        if(isStartMulticast())
            snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                        "c=IN IP4 %s/255\r\n", getMulticastDestAddr().c_str());
        else
            snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                        "c=IN IP4 0.0.0.0\r\n");

        // 添加媒体属性（a=行，如编解码参数）
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                    "%s\r\n", mTracks[i].mRtpSink->getAttribute().c_str());

        // 添加控制URL（a=control）
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),											
                "a=control:track%d\r\n", mTracks[i].mTrackId);
    }

    mSdp = buf;

    return mSdp;
}

MediaSession::Track* MediaSession::getTrack(MediaSession::TrackId trackId)
{
    for(int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if(mTracks[i].mTrackId == trackId)
            return &mTracks[i];
    }

    return NULL;
}

/**
 * 将 RTP Sink 添加到媒体会话的指定轨道中,用于向该客户端发送 RTP 数据包
 * @param trackId
 * @param rtpSink
 * @return
 */
bool MediaSession::addRtpSink(MediaSession::TrackId trackId, RtpSink* rtpSink)
{
    Track* track;// 声明一个指向 Track 的指针

    // 根据 trackId 获取对应的 Track 对象
    track = getTrack(trackId);
    if(!track)
        return false;

    // 将传入的 rtpSink 赋值给 track 的 mRtpSink 成员变量
    track->mRtpSink = rtpSink;
    // 将该 track 标记为活跃状态
    track->mIsAlive = true;

    // 设置 RTP Sink 的发送帧回调函数
    // 参数1: 回调函数指针 sendPacketCallback
    // 参数2: this 指针（当前 MediaSession 对象）
    // 参数3: track 指针
    rtpSink->setSendFrameCallback(sendPacketCallback, this, track);

    return true;
}

bool MediaSession::addRtpInstance(MediaSession::TrackId trackId, RtpInstance* rtpInstance)
{
    Track* track = getTrack(trackId);
    if(!track || track->mIsAlive != true)
        return false;
    
    track->mRtpInstances.push_back(rtpInstance);

    return true;
}

bool MediaSession::removeRtpInstance(RtpInstance* rtpInstance)
{
    for(int i = 0; i <  MEDIA_MAX_TRACK_NUM; ++i)
    {
        if(mTracks[i].mIsAlive == false)
            continue;
        
        std::list<RtpInstance*>::iterator it = std::find(mTracks[i].mRtpInstances.begin(),
                                                            mTracks[i].mRtpInstances.end(),
                                                                rtpInstance);
        if(it == mTracks[i].mRtpInstances.end())
            continue;

        mTracks[i].mRtpInstances.erase(it);
        return true;
    }

    return false;
}

void MediaSession::sendPacketCallback(void* arg1, void* arg2, RtpPacket* rtpPacket)
{
    MediaSession* mediaSession = (MediaSession*)arg1;
    MediaSession::Track* track = (MediaSession::Track*)arg2;
    
    mediaSession->sendPacket(track, rtpPacket);
}

void MediaSession::sendPacket(MediaSession::Track* track, RtpPacket* rtpPacket)
{
    std::list<RtpInstance*>::iterator it;

    for(it = track->mRtpInstances.begin(); it != track->mRtpInstances.end(); ++it)
    {
        if((*it)->alive() == true)
        {
            (*it)->send(rtpPacket);
        }
    }
}

bool MediaSession::startMulticast()
{
    /* 随机生成多播地址 */
    struct sockaddr_in addr = { 0 };
    uint32_t range = 0xE8FFFFFF - 0xE8000100;
    addr.sin_addr.s_addr = htonl(0xE8000100 + (rand()) % range);
    mMulticastAddr = inet_ntoa(addr.sin_addr);

    int rtpSockfd1, rtcpSockfd1;
    int rtpSockfd2, rtcpSockfd2;
    uint16_t rtpPort1, rtcpPort1;
    uint16_t rtpPort2, rtcpPort2;
    bool ret;

    rtpSockfd1 = sockets::createUdpSock();
    assert(rtpSockfd1 > 0);

    rtpSockfd2 = sockets::createUdpSock();
    assert(rtpSockfd2 > 0);

    rtcpSockfd1 = sockets::createUdpSock();
    assert(rtcpSockfd1 > 0);

    rtcpSockfd2 = sockets::createUdpSock();
    assert(rtcpSockfd2 > 0);

    uint16_t port = rand() & 0xfffe;
    if(port < 10000)
        port += 10000;
    
    rtpPort1 = port;
    rtcpPort1 = port+1;
    rtpPort2 = rtcpPort1+1;
    rtcpPort2 = rtpPort2+1;

    mMulticastRtpInstances[TrackId0] = RtpInstance::createNewOverUdp(rtpSockfd1, 0, mMulticastAddr, rtpPort1);
    mMulticastRtpInstances[TrackId1] = RtpInstance::createNewOverUdp(rtpSockfd2, 0, mMulticastAddr, rtpPort2);    
    mMulticastRtcpInstances[TrackId0] = RtcpInstance::createNew(rtcpSockfd1, 0, mMulticastAddr, rtcpPort1);
    mMulticastRtcpInstances[TrackId1] = RtcpInstance::createNew(rtcpSockfd2, 0, mMulticastAddr, rtcpPort2);    

    this->addRtpInstance(TrackId0, mMulticastRtpInstances[TrackId0]);
    this->addRtpInstance(TrackId1, mMulticastRtpInstances[TrackId1]);
    mMulticastRtpInstances[TrackId0]->setAlive(true);
    mMulticastRtpInstances[TrackId1]->setAlive(true);

    mIsStartMulticast = true;

    return true;
}

bool MediaSession::isStartMulticast()
{
    return mIsStartMulticast;
}

uint16_t MediaSession::getMulticastDestRtpPort(TrackId trackId)
{
    if(trackId > TrackId1 || !mMulticastRtpInstances[trackId])
        return -1;

    return mMulticastRtpInstances[trackId]->getPeerPort();
}
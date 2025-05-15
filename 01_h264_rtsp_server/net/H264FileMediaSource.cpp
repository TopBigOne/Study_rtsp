#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "H264FileMediaSource.h"
#include "Logging.h"
#include "New.h"
#include "UsageEnvironment.h"

static inline int startCode3(uint8_t* buf);
static inline int startCode4(uint8_t* buf);

H264FileMediaSource* H264FileMediaSource::createNew(UsageEnvironment* env, std::string file)
{
    //return new H264FileMediaSource(env, file);
    return New<H264FileMediaSource>::allocate(env, file);
}

H264FileMediaSource::H264FileMediaSource(UsageEnvironment* env, const std::string& file) :
    MediaSource(env),
    mFile(file)
{
    mFd = ::open(file.c_str(), O_RDONLY);
    assert(mFd > 0);

    setFps(25);

    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i)
        mEnv->threadPool()->addTask(mTask);
}

H264FileMediaSource::~H264FileMediaSource()
{
    ::close(mFd);
}

/**
 * 调用链路：
 *  pthread() -->threadRun()-->run()-->handleTask()-->TaskQueue()-->
 *  Task()#handle()--> MediaSource#taskCallback()-->H264FileMediaSource#readFrame()
 *
 */
void H264FileMediaSource::readFrame()
{
    // 加锁保护队列操作（RAII方式）
    MutexLockGuard mutexLockGuard(mMutex);

    // 检查输入队列是否为空
    if(mAVFrameInputQueue.empty()){
        return;// 无待处理的帧容器，直接返回
    }


    // 获取输入队列首部的帧容器,这个frame 是数据是空的吗？
    AVFrame* frame = mAVFrameInputQueue.front();

    // 从H264文件读取一帧数据到frame->mBuffer
    // 返回值：实际读取的字节数（<0表示错误）
    frame->mFrameSize = getFrameFromH264File(mFd, frame->mBuffer, FRAME_MAX_SIZE);
    if(frame->mFrameSize < 0)
        return;

    // 检查起始码类型（H264帧起始标志）
    if(startCode3(frame->mBuffer))// 判断是否为3字节起始码 0x000001
    {
        // 跳过3字节起始码
        frame->mFrame = frame->mBuffer+3;// 有效数据起始位置
        frame->mFrameSize -= 3;// 有效数据长度
    }
    else// 4字节起始码 0x00000001
    {
        // 跳过4字节起始码，把真实的数据赋值给mFrame
        frame->mFrame = frame->mBuffer+4;
        frame->mFrameSize -= 4;
    }

    // 你上面的代码，mAVFrameInputQueue 调用了front()
    mAVFrameInputQueue.pop();// 从输入队列移除空容器
    mAVFrameOutputQueue.push(frame);// 将填充好的帧送入输出队列

    // 什么需要pop()？
    // 关键原因：避免重复处理同一内存块
    // pop出来的AVFrame没有消失，只是从inputQueue 进入到了outputQueue

}

/**
 * 判断是否为3字节起始码 0x000001
 * @param buf
 * @return
 */
static inline int startCode3(uint8_t* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

/**
 * 4字节起始码 0x00000001
 * @param buf
 * @return
 */
static inline int startCode4(uint8_t* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}

static uint8_t* findNextStartCode(uint8_t* buf, int len)
{
    int i;

    if(len < 3)
        return NULL;

    for(i = 0; i < len-3; ++i)
    {
        if(startCode3(buf) || startCode4(buf))
            return buf;
        
        ++buf;
    }

    if(startCode3(buf))
        return buf;

    return NULL;
}

/**
 * Note: 新数据覆盖旧数据
 * @param fd
 * @param frame
 * @param size
 * @return
 */
int H264FileMediaSource::getFrameFromH264File(int fd, uint8_t* frame, int size)
{
    int rSize, frameSize;
    uint8_t* nextStartCode;

    if(fd < 0){
        return fd;
    }


    rSize = read(fd, frame, size);
    if(!startCode3(frame) && !startCode4(frame))
        return -1;
    
    nextStartCode = findNextStartCode(frame+3, rSize-3);
    if(!nextStartCode)
    {
        lseek(fd, 0, SEEK_SET);
        frameSize = rSize;
    }
    else
    {
        frameSize = (nextStartCode-frame);
        lseek(fd, frameSize-rSize, SEEK_CUR);
    }

    return frameSize;
}
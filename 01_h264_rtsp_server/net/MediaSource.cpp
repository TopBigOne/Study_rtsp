#include "MediaSource.h"
#include "Logging.h"
#include "New.h"

MediaSource::MediaSource(UsageEnvironment* env) :
    mEnv(env)
{
    mMutex = Mutex::createNew();
    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i)
        mAVFrameInputQueue.push(&mAVFrames[i]);
    
    mTask.setTaskCallback(taskCallback, this);
}

MediaSource::~MediaSource()
{
    //delete mMutex;
    Delete::release(mMutex);
}

AVFrame* MediaSource::getFrame()
{
    MutexLockGuard mutexLockGuard(mMutex);

    if(mAVFrameOutputQueue.empty())
    {
        return NULL;
    }

    AVFrame* frame = mAVFrameOutputQueue.front();    
    mAVFrameOutputQueue.pop();

    return frame;
}

/**
 * 不需要对AVFrame 的数据，进行擦除吗？
 * @param frame
 */
void MediaSource::putFrame(AVFrame* frame)
{
    MutexLockGuard mutexLockGuard(mMutex);

    mAVFrameInputQueue.push(frame);

    // 这个task 最后会执行到当前类的 taskCallback(),
    mEnv->threadPool()->addTask(mTask);
}


void MediaSource::taskCallback(void* arg)
{
    MediaSource* source = (MediaSource*)arg;
    // 实际调用  H264FileMediaSource#readFrame()
    source->readFrame();
}

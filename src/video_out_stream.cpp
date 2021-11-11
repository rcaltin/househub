#include "video_out_stream.h"
#include "file_manager.h"

VideoOutStream::VideoOutStream(const std::string &name, uint32_t fps,
                               const cv::Size &outputSize,
                               uint32_t chunkLengthSec, const char *fourcc,
                               const std::string &fileExtension)
    : mName(name), mFps(fps), mOutputSize(outputSize),
      mChunkLengthSec(chunkLengthSec), mFileExtension(fileExtension) {
  setFourcc(fourcc);
}

VideoOutStream::~VideoOutStream() { releaseChunk(); }

bool VideoOutStream::init() { return beginChunk(); }

void VideoOutStream::update(const uint64_t delta) {
  time_t t = std::time(nullptr);

  if (mVideoWriter && t - mLastWriteTime > 0) {
    std::unique_lock<std::mutex> lock(mFrameQueueMutex);

    // move frames to the buffer
    std::list<VideoFrame> buffer;
    while (!mFrameQueue.empty() && mFrameQueue.front().time <= t) {
      buffer.emplace_front(std::move(mFrameQueue.front()));
      mFrameQueue.pop();
    }
    lock.unlock();

    // push a blank frame if the queue empty
    if (buffer.empty()) {
      VideoFrame vf;
      vf.time = t;
      cv::Mat f(mOutputSize, CV_8UC3, cv::Scalar(0, 0, 0));
      timeStampAndResizeFrame(f, vf.frame, t);
      buffer.emplace_front(std::move(vf));
    }

    // extend buffer if frame count less than fps
    while (buffer.size() < mFps) {
      for (auto it = buffer.begin(); it != buffer.end() && buffer.size() < mFps;
           ++it) {
        it = buffer.insert(it, *it);
        ++it;
      }
    }

    // shrink buffer if frame count more than fps
    while (buffer.size() > mFps) {
      buffer.pop_back();
    }

    // flush buffer into video
    while (!buffer.empty()) {
      mVideoWriter->write(buffer.back().frame);
      buffer.pop_back();
    }

    // switch to a new chunk if current chunk complete
    mWrittenFramesCount += mFps;
    if (mChunkLengthSec > 0 &&
        mWrittenFramesCount >= (mChunkLengthSec * mFps)) {
      beginChunk();
    }

    mLastWriteTime = t;
  }
}

void VideoOutStream::pushFrame(cv::Mat &&frame, time_t t) {
  VideoFrame vf;
  vf.time = t != 0 ? t : std::time(nullptr);

  timeStampAndResizeFrame(frame, vf.frame, vf.time);

  // enqueue
  std::unique_lock<std::mutex> lock(mFrameQueueMutex);
  mFrameQueue.emplace(std::move(vf));
}

std::string VideoOutStream::getName() const { return mName; }

void VideoOutStream::setName(const std::string &name) { mName = name; }

uint32_t VideoOutStream::getFps() const { return mFps; }

void VideoOutStream::setFps(uint32_t fps) { mFps = fps; }

cv::Size VideoOutStream::getOutputSize() const { return mOutputSize; }

void VideoOutStream::setOutputSize(const cv::Size &outputSize) {
  mOutputSize = outputSize;
}

uint32_t VideoOutStream::getChunkLengthSec() const { return mChunkLengthSec; }

void VideoOutStream::setChunkLengthSec(uint32_t chunkLengthSec) {
  mChunkLengthSec = chunkLengthSec;
}

const char *VideoOutStream::getFourcc() const { return mFourcc; }

void VideoOutStream::setFourcc(const char *fourcc) {
  if (sizeof(fourcc) >= sizeof(mFourcc)) {
    std::copy(fourcc, fourcc + 4, mFourcc);
  }
}

std::string VideoOutStream::getFileExtension() const { return mFileExtension; }

void VideoOutStream::setFileExtension(const std::string &fileExtension) {
  mFileExtension = fileExtension;
}

void VideoOutStream::timeStampAndResizeFrame(cv::Mat &frameIn,
                                             cv::Mat &frameOut, time_t t) {
  if (!t) {
    t = std::time(nullptr);
  }

  // resize
  cv::resize(frameIn, frameOut, mOutputSize);

  std::string label = timeString(t) + " " + mName;

  // grey wide label as border
  cv::putText(frameOut, label, cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN, 1.5f,
              cv::Scalar(128, 128, 128, 128), 3, false);

  // white narrow label as body
  cv::putText(frameOut, label, cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN, 1.5f,
              cv::Scalar(255, 255, 255, 128), 1, false);
}

bool VideoOutStream::beginChunk() {
  // release current video (if exists)
  releaseChunk();

  // set a new writer
  mVideoWriter.reset(new cv::VideoWriter());

  // create a new video file
  const std::string &file =
      FileManager::instance().generateRecordFile(mName, mFileExtension);
  const int fourcc =
      cv::VideoWriter::fourcc(mFourcc[0], mFourcc[1], mFourcc[2], mFourcc[3]);
  if (!mVideoWriter->open(file, fourcc, mFps, mOutputSize, true)) {
    std::cout << "Error: creating video file failed." << std::endl;
    mVideoWriter.reset(nullptr);
    return false;
  }

  return true;
}

bool VideoOutStream::releaseChunk() {
  mWrittenFramesCount = 0;

  if (!mVideoWriter) {
    return false;
  }

  mVideoWriter->release();

  mVideoWriter.reset(nullptr);

  return true;
}
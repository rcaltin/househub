#include "video_out_stream.h"
#include "file_manager.h"

VideoOutStream::VideoOutStream() {}

VideoOutStream::~VideoOutStream() { releaseChunk(); }

bool VideoOutStream::init(const VideoOutStreamParams &params) {
  mParams = params;
  return beginChunk();
}

void VideoOutStream::update(const uint64_t delta) {
  const time_t t = std::time(nullptr);

  if (mVideoWriter && t - mLastWriteTime > 0) {
    std::unique_lock<std::mutex> lock(mFrameQueueMutex);
    const int fps = mParams.fps;

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
      cv::Mat f(mParams.outputSize, CV_8UC3, cv::Scalar(0, 0, 0));
      resizeAndWatermarkFrame(f, vf.frame, t);
      buffer.emplace_front(std::move(vf));
    }

    // extend buffer if frame count less than fps
    while (buffer.size() < fps) {
      for (auto it = buffer.begin(); it != buffer.end() && buffer.size() < fps;
           ++it) {
        it = buffer.insert(it, *it);
        ++it;
      }
    }

    // shrink buffer if frame count more than fps
    while (buffer.size() > fps) {
      buffer.pop_back();
    }

    // flush buffer into video
    while (!buffer.empty()) {
      mVideoWriter->write(buffer.back().frame);
      buffer.pop_back();
    }

    // switch to a new chunk if current chunk complete
    mWrittenFramesCount += fps;
    if (mParams.chunkLengthSec > 0 &&
        mWrittenFramesCount >= (mParams.chunkLengthSec * fps)) {
      beginChunk();
    }

    mLastWriteTime = t;
  }
}

void VideoOutStream::pushFrame(cv::Mat &&frame, time_t t) {
  VideoFrame vf;
  vf.time = t != 0 ? t : std::time(nullptr);

  resizeAndWatermarkFrame(frame, vf.frame, vf.time);

  // enqueue
  std::unique_lock<std::mutex> lock(mFrameQueueMutex);
  mFrameQueue.emplace(std::move(vf));
}

VideoOutStreamParams &VideoOutStream::params() { return mParams; }

void VideoOutStream::resizeAndWatermarkFrame(cv::Mat &frameIn,
                                             cv::Mat &frameOut, time_t t) {
  if (!t) {
    t = std::time(nullptr);
  }

  // resize
  cv::resize(frameIn, frameOut, mParams.outputSize);

  // watermark
  if (mParams.watermark) {
    std::string label =
        timeString(t, mParams.useLocaltime) + " " + mParams.name;

    // grey wide label as border
    cv::putText(frameOut, label, cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN,
                1.5f, cv::Scalar(128, 128, 128, 128), 3, false);

    // white narrow label as body
    cv::putText(frameOut, label, cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN,
                1.5f, cv::Scalar(255, 255, 255, 128), 1, false);
  }
}

bool VideoOutStream::beginChunk() {
  // release current video (if exists)
  releaseChunk();

  // set a new writer
  mVideoWriter.reset(new cv::VideoWriter());

  // create a new video file
  const std::string &file = FileManager::instance().generateRecordFile(
      mParams.name, mParams.fileExtension);

  const auto fourccStr = mParams.fourcc;
  const int fourcc = cv::VideoWriter::fourcc(fourccStr[0], fourccStr[1],
                                             fourccStr[2], fourccStr[3]);
  if (!mVideoWriter->open(file, fourcc, mParams.fps, mParams.outputSize,
                          true)) {
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
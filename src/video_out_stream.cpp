#include "video_out_stream.h"
#include "file_manager.h"

VideoOutStream::VideoOutStream() {}

VideoOutStream::~VideoOutStream() { releaseChunk(); }

bool VideoOutStream::init(const VideoOutStreamParams &params) {
  mParams = params;
  return beginChunk(std::time(nullptr));
}

void VideoOutStream::update(time_t t) {
  // update once per second
  if (!mVideoWriter || t - mLastWriteTime == 0) {
    return;
  }

  // move frames to the buffer
  std::list<VideoFrame> buffer;
  while (!mFrameQueue.empty() && mFrameQueue.front().time <= t) {
    buffer.emplace_front(std::move(mFrameQueue.front()));
    mFrameQueue.pop();
  }

  // push a blank frame if the queue empty
  if (buffer.empty()) {
    VideoFrame vf;
    vf.time = t;
    vf.frame = cv::Mat(mParams.outputSize, CV_8UC3, cv::Scalar(0, 0, 0));

    // watermark
    if (mParams.watermark) {
      watermarkFrame(vf.frame, t);
    }

    buffer.emplace_front(std::move(vf));
  }

  const uint32_t fps = mParams.fps;

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

  mLastWriteTime = t;

  // switch to a new chunk if current chunk complete
  mWrittenFramesCount += fps;
  const bool newChunk =
      mParams.uniformChunks
          ? t % mParams.chunkLengthSec == 0
          : mWrittenFramesCount >= (mParams.chunkLengthSec * fps);
  if (mParams.chunkLengthSec > 0 && newChunk) {
    beginChunk(t);
  }
}

void VideoOutStream::feed(cv::Mat &&frame, time_t t) {
  VideoFrame vf;
  vf.time = t;
  vf.frame = std::move(frame);

  // watermark
  if (mParams.watermark) {
    watermarkFrame(vf.frame, vf.time);
  }

  // enqueue
  mFrameQueue.emplace(std::move(vf));
}

VideoOutStreamParams &VideoOutStream::params() { return mParams; }

void VideoOutStream::watermarkFrame(cv::Mat &frame, time_t t) {
  if (!t) {
    t = std::time(nullptr);
  }

  std::string label = timeString(t, mParams.useLocaltime) + " " + mParams.name;

  // grey wide label as border
  cv::putText(frame, label, cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN, 1.5f,
              cv::Scalar(128, 128, 128, 128), 3, false);

  // white narrow label as body
  cv::putText(frame, label, cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN, 1.5f,
              cv::Scalar(255, 255, 255, 128), 1, false);
}

bool VideoOutStream::beginChunk(time_t t) {
  // release current video (if exists)
  releaseChunk();

  // set a new writer
  mVideoWriter.reset(new cv::VideoWriter());

  // create a new video file
  const uint32_t len =
      mParams.uniformChunks
          ? (mParams.chunkLengthSec - (t % mParams.chunkLengthSec))
          : mParams.chunkLengthSec;
  mCurrentVideoFile = FileManager::instance().generateRecordFile(
      mParams.name, mParams.fileExtension, len, t);

  const auto fourccStr = mParams.fourcc;
  const int fourcc = cv::VideoWriter::fourcc(fourccStr[0], fourccStr[1],
                                             fourccStr[2], fourccStr[3]);
  if (!mVideoWriter->open(mCurrentVideoFile, fourcc, mParams.fps,
                          mParams.outputSize, true)) {
    LOG(ERROR) << "video file creation failed: " << mCurrentVideoFile;
    mVideoWriter.reset(nullptr);
    return false;
  }

  return true;
}

bool VideoOutStream::releaseChunk() {
  if (!mVideoWriter) {
    return false;
  }

  mVideoWriter->release();

  // rename the file if incomplete or length is infinite
  if (mParams.chunkLengthSec == 0 ||
      mWrittenFramesCount < mParams.chunkLengthSec * mParams.fps) {
    const uint32_t len = mWrittenFramesCount / mParams.fps;
    const std::string &newFileName = FileManager::instance().generateRecordFile(
        mParams.name, mParams.fileExtension, len, mLastWriteTime - len);

    rename(mCurrentVideoFile.c_str(), newFileName.c_str());
  }

  mVideoWriter.reset(nullptr);
  mWrittenFramesCount = 0;

  return true;
}
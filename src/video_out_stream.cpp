#include "video_out_stream.h"
#include "file_manager.h"

VideoOutStream::VideoOutStream() {}

VideoOutStream::~VideoOutStream() { releaseChunk(); }

bool VideoOutStream::init(const VideoOutStreamParams &params) {
  mParams = params;

  return beginChunk(mLastWriteTime = std::time(nullptr) - 1);
}

void VideoOutStream::update(const time_t t) {

  // update once per second
  if (t == mLastWriteTime || !mVideoWriter) {
    return;
  }

  // process the queue for every second since last update till t - 1
  while (mLastWriteTime < t) {
    processQueueForTime(mLastWriteTime + 1);
  }
}

void VideoOutStream::feed(cv::Mat &&frame, const time_t t) {
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

void VideoOutStream::processQueueForTime(const time_t t) {
  // move frames to the buffer
  std::list<VideoFrame> buffer;
  while (!mFrameQueue.empty() && mFrameQueue.front().time <= t) {
    buffer.emplace_back(std::move(mFrameQueue.front()));
    mFrameQueue.pop();
  }

  // push a blank frame if the buffer empty
  if (buffer.empty()) {
    VideoFrame vf;
    vf.time = t;
    vf.frame = cv::Mat(mParams.outputSize, CV_8UC3, cv::Scalar(0, 0, 0));

    // watermark
    if (mParams.watermark) {
      watermarkFrame(vf.frame, vf.time);
    }

    buffer.emplace_back(std::move(vf));
  }

  // extend the buffer when frame count less than fps
  while (buffer.size() < mParams.fps) {
    // duplicate the buffer frames for a smooth video (bound to the fps)
    for (auto it = buffer.begin();
         it != buffer.end() && buffer.size() < mParams.fps; ++it) {
      it = buffer.insert(it, *it);
      ++it; // skip newly inserted
    }
  }

  // shrink the buffer when frame count more than fps
  while (buffer.size() > mParams.fps) {
    // erase half of the buffer frames for a smooth video (bound to the fps)
    for (auto it = buffer.begin();
         it != buffer.end() && buffer.size() > mParams.fps; ++it) {
      it = buffer.erase(it);
    }
  }

  // flush the buffer into the video file
  while (!buffer.empty()) {
    mVideoWriter->write(buffer.front().frame);
    buffer.pop_front();
    mWrittenFramesCount++;
  }

  mLastWriteTime = t;

  // switch to a new chunk if current chunk complete
  const bool newChunkFlag =
      mParams.chunkLengthSec > 0 &&
      (mParams.uniformChunks
           ? (t % mParams.chunkLengthSec) == 0
           : mWrittenFramesCount >= (mParams.chunkLengthSec * mParams.fps));
  if (newChunkFlag) {
    beginChunk(t);
  }
}

void VideoOutStream::watermarkFrame(cv::Mat &frame, const time_t t) {
  std::string label = timeString(t, mParams.useLocaltime) + " " + mParams.name;

  // grey wide label as border
  cv::putText(frame, label, cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN, 1.5f,
              cv::Scalar(128, 128, 128, 128), 3, false);

  // white narrow label as body
  cv::putText(frame, label, cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN, 1.5f,
              cv::Scalar(255, 255, 255, 128), 1, false);
}

bool VideoOutStream::beginChunk(const time_t t) {
  // release current video (if exists)
  releaseChunk();

  // set a new writer
  mVideoWriter.reset(new cv::VideoWriter());
  mWrittenFramesCount = 0;

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
  mVideoWriter.reset(nullptr);

  // rename the file if incomplete or length is infinite
  const uint32_t lengthSec = mWrittenFramesCount / mParams.fps;
  if (mParams.chunkLengthSec && mParams.chunkLengthSec > lengthSec) {

    const time_t tStart = mLastWriteTime - lengthSec;

    const std::string &newFileName = FileManager::instance().generateRecordFile(
        mParams.name, mParams.fileExtension, lengthSec, tStart);

    rename(mCurrentVideoFile.c_str(), newFileName.c_str());
  }

  return true;
}
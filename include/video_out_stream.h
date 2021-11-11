#pragma once

#include "globals.h"
#include <mutex>
#include <queue>

struct VideoFrame {
  cv::Mat frame;
  time_t time;
};

class VideoOutStream {
public:
  VideoOutStream(const std::string &name, uint32_t fps,
                 const cv::Size &outputSize, uint32_t chunkLengthSec,
                 const char *fourcc, const std::string &fileExtension);

  ~VideoOutStream();

  bool init();

  void update(const uint64_t delta);

  void pushFrame(cv::Mat &&frame, time_t t = 0);

  std::string getName() const;
  void setName(const std::string &name);

  uint32_t getFps() const;
  void setFps(uint32_t fps);

  cv::Size getOutputSize() const;
  void setOutputSize(const cv::Size &outputSize);

  uint32_t getChunkLengthSec() const;
  void setChunkLengthSec(uint32_t chunkLengthSec);

  const char *getFourcc() const;
  void setFourcc(const char *fourcc);

  std::string getFileExtension() const;
  void setFileExtension(const std::string &fileExtension);

private:
  void timeStampAndResizeFrame(cv::Mat &frameIn, cv::Mat &frameOut,
                               time_t t = 0);

  bool beginChunk();

  bool releaseChunk();

  std::string mName;
  uint32_t mFps = 0;
  cv::Size mOutputSize;
  uint32_t mChunkLengthSec = 0;
  char mFourcc[4];
  std::string mFileExtension;
  uint32_t mWrittenFramesCount = 0;
  time_t mLastWriteTime = 0;
  std::unique_ptr<cv::VideoWriter> mVideoWriter;
  std::queue<VideoFrame> mFrameQueue;
  std::mutex mFrameQueueMutex;
};

#pragma once

#include "globals.h"
#include <mutex>
#include <queue>

struct VideoFrame {
  cv::Mat frame;
  time_t time;
};

struct VideoOutStreamParams {
  std::string name;
  uint32_t fps{10};
  cv::Size outputSize{1024, 768};
  uint32_t chunkLengthSec{300};
  char fourcc[4]{'m', 'j', 'p', 'g'};
  std::string fileExtension;
  bool watermark{true};
  bool useLocaltime{true};
};

class VideoOutStream {
public:
  VideoOutStream();

  ~VideoOutStream();

  bool init(const VideoOutStreamParams &params);

  void update(const uint64_t delta);

  void pushFrame(cv::Mat &&frame, time_t t = 0);

  VideoOutStreamParams &params();

private:
  void resizeAndWatermarkFrame(cv::Mat &frameIn, cv::Mat &frameOut,
                               time_t t = 0);

  bool beginChunk();

  bool releaseChunk();

  VideoOutStreamParams mParams;
  uint32_t mWrittenFramesCount = 0;
  time_t mLastWriteTime = 0;
  std::unique_ptr<cv::VideoWriter> mVideoWriter;
  std::queue<VideoFrame> mFrameQueue;
  std::mutex mFrameQueueMutex;
  std::string mCurrentVideoFile;
};

#pragma once

#include "icapturer.h"
#include "video_out_stream.h"
#include <atomic>
#include <memory>
#include <thread>

class Capturer : public ICapturer {
public:
  Capturer(const std::string &name, const std::string &streamUri, uint32_t fps,
           cv::Size outputSize, uint32_t filterKernel,  uint32_t chunkLengthSec, const char *fourcc,
           const std::string &fileExtension);

  ~Capturer();

  bool init() override;

  void update(const uint64_t delta) override;

  void startCapture() override;

  void stopCapture() override;

  bool isCapturing() const override;

  std::string getName() const override;
  void setName(const std::string &name) override;

  std::string getStreamUri() const override;
  void setStreamUri(const std::string &streamUri) override;

  uint32_t getFps() const override;
  void setFps(uint32_t fps) override;

  cv::Size getOutputSize() const override;
  void setOutputSize(const cv::Size &outputSize) override;

  uint32_t getFilterK() const override;
  void setFilterK(uint32_t filterK) override;
  
  uint32_t getChunkLengthSec() const override;
  void setChunkLengthSec(uint32_t chunkLengthSec) override;

  const char *getFourcc() const;
  void setFourcc(const char *fourcc);

  std::string getFileExtension() const;
  void setFileExtension(const std::string &fileExtension);

private:
  std::string mName;
  std::string mStreamUri;
  uint32_t mFilterK = 0;
  std::atomic_bool mCapturing = false;
  std::thread mCaptureThread;
  std::unique_ptr<cv::VideoCapture> mVideoCapture;
  std::unique_ptr<VideoOutStream> mOutStream = nullptr;
  time_t mLastGrabTime = 0;
  cv::Mat mLastGrabedFrame;
  std::atomic_bool mExitFlag = false;
};

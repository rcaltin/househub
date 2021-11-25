#pragma once

#include "icapturer.h"
#include <atomic>
#include <memory>
#include <thread>

class Capturer : public ICapturer {
public:
  Capturer();

  ~Capturer();

  bool init(const CapturerParams &params) override;

  void startCapture() override;

  void stopCapture() override;

  bool isCapturing() const override;

  bool isStreamHealthy() const override;

  CapturerParams &params() override;

private:
  CapturerParams mParams;
  std::atomic_bool mExitFlag = false;
  std::atomic_bool mCapturing = false;
  std::atomic_bool mStreamHealthy = false;
  std::thread mCaptureThread;
  std::unique_ptr<cv::VideoCapture> mVideoCapture;
  std::unique_ptr<VideoOutStream> mOutStream = nullptr;
  time_t mLastGrabTime = 0;
  cv::Mat mLastGrabedFrame;
};

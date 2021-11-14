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

  void update(const uint64_t delta) override;

  void startCapture() override;

  void stopCapture() override;

  bool isCapturing() const override;

  CapturerParams &params() override;

private:
  CapturerParams mParams;
  std::atomic_bool mCapturing = false;
  std::thread mCaptureThread;
  std::unique_ptr<cv::VideoCapture> mVideoCapture;
  std::unique_ptr<VideoOutStream> mOutStream = nullptr;
  time_t mLastGrabTime = 0;
  cv::Mat mLastGrabedFrame;
  std::atomic_bool mExitFlag = false;
};

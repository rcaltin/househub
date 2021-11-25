#include "capturer.h"

Capturer::Capturer() {}

Capturer::~Capturer() {
  mExitFlag = true;
  if (mCaptureThread.joinable()) {
    mCaptureThread.join();
  }
}

bool Capturer::init(const CapturerParams &params) {
  mParams = params;
  mOutStream.reset(new VideoOutStream());

  if (!mOutStream->init(mParams.videoOutStreamParams)) {
    return false;
  }

  mCaptureThread = std::thread([this]() {
    mVideoCapture.reset(new cv::VideoCapture(mParams.streamUri));

    while (!mExitFlag) {

      const time_t t = std::time(nullptr);

      if (mCapturing && mVideoCapture->isOpened() && mVideoCapture->grab() &&
          mVideoCapture->retrieve(mLastGrabedFrame)) {
        mLastGrabTime = t;

        if (mParams.filterK > 1) {
          cv::medianBlur(mLastGrabedFrame, mLastGrabedFrame,
                         mParams.filterK % 2 ? mParams.filterK
                                             : mParams.filterK + 1);
        }

        mOutStream->feed(std::move(mLastGrabedFrame), t);

        if (!mStreamHealthy) {
          mStreamHealthy = true;
          LOG(INFO) << "capturer in-stream is up: " << mParams.name;
        }
      }

      // stream health check
      if (t - mLastGrabTime > 1) {
        mVideoCapture.reset(new cv::VideoCapture(mParams.streamUri));

        if (mStreamHealthy) {
          mStreamHealthy = false;
          LOG(WARNING) << "capturer in-stream is down: " << mParams.name;
        }
      }

      mOutStream->update();
    }
  });

  return true;
}

void Capturer::startCapture() {
  mCapturing = true;
  LOG(INFO) << "capturer started: " << mParams.name;
}

void Capturer::stopCapture() {
  mCapturing = false;
  LOG(INFO) << "capturer stopped: " << mParams.name;
}

bool Capturer::isCapturing() const { return mCapturing; }

bool Capturer::isStreamHealthy() const { return mStreamHealthy; }

CapturerParams &Capturer::params() { return mParams; }

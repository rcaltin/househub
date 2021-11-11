#include "capturer.h"

Capturer::Capturer(const std::string &name, const std::string &streamUri,
                   uint32_t fps, cv::Size outputSize, uint32_t filterK,
                   uint32_t chunkLengthSec, const char *fourcc,
                   const std::string &fileExtension)
    : mName(name), mStreamUri(streamUri), mFilterK(filterK), mCapturing(false) {
  mOutStream = std::unique_ptr<VideoOutStream>(new VideoOutStream(
      mName, fps, outputSize, chunkLengthSec, fourcc, fileExtension));
}

Capturer::~Capturer() {
  mExitFlag = true;
  if (mCaptureThread.joinable()) {
    mCaptureThread.join();
  }
}

bool Capturer::init() {
  if (!mOutStream->init()) {
    return false;
  }

  mCaptureThread = std::thread([this]() {
    mVideoCapture.reset(new cv::VideoCapture(mStreamUri));

    while (!mExitFlag) {

      time_t t = std::time(nullptr);

      if (mCapturing && mVideoCapture->isOpened() && mVideoCapture->grab() &&
          mVideoCapture->retrieve(mLastGrabedFrame)) {
        mLastGrabTime = t;

        if (mFilterK > 1) {
            cv::medianBlur(mLastGrabedFrame, mLastGrabedFrame, mFilterK % 2 ? mFilterK : mFilterK + 1);
        }

        mOutStream->pushFrame(std::move(mLastGrabedFrame));
      }

      // connection drop check
      if (t - mLastGrabTime > 1) {
        mVideoCapture.reset(new cv::VideoCapture(mStreamUri));
      }
    }
  });

  return true;
}

void Capturer::update(const uint64_t delta) { mOutStream->update(delta); }

void Capturer::startCapture() { mCapturing = true; }

void Capturer::stopCapture() { mCapturing = false; }

bool Capturer::isCapturing() const { return mCapturing; }

std::string Capturer::getName() const { return mName; }

void Capturer::setName(const std::string &name) {
  mOutStream->setName(mName = name);
}

std::string Capturer::getStreamUri() const { return mStreamUri; }

void Capturer::setStreamUri(const std::string &streamUri) {
  mStreamUri = streamUri;
}

uint32_t Capturer::getFps() const { return mOutStream->getFps(); }

void Capturer::setFps(uint32_t fps) { mOutStream->setFps(fps); }

cv::Size Capturer::getOutputSize() const { return mOutStream->getOutputSize(); }

void Capturer::setOutputSize(const cv::Size &outputSize) {
  mOutStream->setOutputSize(outputSize);
}

uint Capturer::getFilterK() const { return mFilterK; }

void Capturer::setFilterK(uint filterK) { mFilterK = filterK; }

uint32_t Capturer::getChunkLengthSec() const {
  return mOutStream->getChunkLengthSec();
}

void Capturer::setChunkLengthSec(uint32_t chunkLengthSec) {
  mOutStream->setChunkLengthSec(chunkLengthSec);
}

const char *Capturer::getFourcc() const { return mOutStream->getFourcc(); }

void Capturer::setFourcc(const char *fourcc) { mOutStream->setFourcc(fourcc); }

std::string Capturer::getFileExtension() const {
  return mOutStream->getFileExtension();
}

void Capturer::setFileExtension(const std::string &fileExtension) {
  mOutStream->setFileExtension(fileExtension);
}

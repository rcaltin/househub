#pragma once

#include "globals.h"

class ICapturer {
public:
  ICapturer() = default;

  virtual ~ICapturer(){};

  virtual bool init() = 0;

  virtual void update(const uint64_t delta) = 0;

  virtual void startCapture() = 0;

  virtual void stopCapture() = 0;

  virtual bool isCapturing() const = 0;

  virtual std::string getName() const = 0;
  virtual void setName(const std::string &name) = 0;

  virtual std::string getStreamUri() const = 0;
  virtual void setStreamUri(const std::string &streamUri) = 0;

  virtual uint32_t getFps() const = 0;
  virtual void setFps(uint32_t fps) = 0;

  virtual cv::Size getOutputSize() const = 0;
  virtual void setOutputSize(const cv::Size &outputSize) = 0;

  virtual uint32_t getFilterK() const = 0;
  virtual void setFilterK(uint32_t filterK) = 0;

  virtual uint32_t getChunkLengthSec() const = 0;
  virtual void setChunkLengthSec(uint32_t chunkLengthSec) = 0;
};

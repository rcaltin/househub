#pragma once

#include "video_out_stream.h"

struct CapturerParams {
  std::string name;
  std::string type;
  std::string streamUri;
  uint32_t filterK = {0};
  VideoOutStreamParams videoOutStreamParams;
};

class ICapturer {
public:
  ICapturer() = default;

  virtual ~ICapturer(){};

  virtual bool init(const CapturerParams &params) = 0;

  virtual void update(const uint64_t delta) = 0;

  virtual void startCapture() = 0;

  virtual void stopCapture() = 0;

  virtual bool isCapturing() const = 0;

  virtual CapturerParams &params() = 0;
};

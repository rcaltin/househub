#pragma once

#include "video_out_stream.h"

struct CapturerParams {
  std::string name;
  std::string type;
  std::string streamUri;
  uint32_t filterK = {0};
  bool flipX{false};
  bool flipY{false};
  VideoOutStreamParams videoOutStreamParams;
};

class ICapturer {
public:
  ICapturer() = default;

  virtual ~ICapturer(){};

  virtual bool init(const CapturerParams &params) = 0;

  virtual void startCapture() = 0;

  virtual void stopCapture() = 0;

  virtual bool isCapturing() const = 0;

  virtual bool isStreamHealthy() const = 0;

  virtual CapturerParams &params() = 0;
};

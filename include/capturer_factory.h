#pragma once

#include "icapturer.h"
#include <memory>

class CapturerFactory {
public:
  static std::unique_ptr<ICapturer>
  createCapturer(const std::string &name, const std::string &type,
                 const std::string &streamUri, uint32_t fps,
                 uint32_t outputWidth, uint32_t outputHeight, uint32_t filterK,
                 uint32_t chunkLenghtSec, const char *fourcc,
                 const std::string &fileExtension);

private:
  CapturerFactory() = default;

  ~CapturerFactory(){};
};
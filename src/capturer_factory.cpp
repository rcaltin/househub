#include "capturer_factory.h"
#include "capturer.h"
#include <algorithm>

std::unique_ptr<ICapturer>
CapturerFactory::createCapturer(const CapturerParams &params) {

  if (params.type == "default") {
    return std::unique_ptr<ICapturer>(new Capturer());
  }

  return nullptr;
}
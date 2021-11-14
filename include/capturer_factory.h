#pragma once

#include "icapturer.h"
#include <memory>

class CapturerFactory {
public:
  static std::unique_ptr<ICapturer>
  createCapturer(const CapturerParams &params);

private:
  CapturerFactory() = default;

  ~CapturerFactory(){};
};
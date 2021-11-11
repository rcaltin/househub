#include "capturer_factory.h"
#include "capturer.h"
#include <algorithm>

std::unique_ptr<ICapturer> CapturerFactory::createCapturer(
    const std::string &name, const std::string &type,
    const std::string &streamUri, uint32_t fps, uint32_t outputWidth,
    uint32_t outputHeight, uint32_t filterK, uint32_t chunkLenghtSec,
    const char *fourcc, const std::string &fileExtension) {

  if (type == "default") {
    return std::unique_ptr<ICapturer>(
        new Capturer(name, streamUri, fps, cv::Size(outputWidth, outputHeight),
                     filterK, chunkLenghtSec, fourcc, fileExtension));
  }

  return nullptr;
}
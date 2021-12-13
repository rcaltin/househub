#pragma once

#include <chrono>
#include <ctime>
#include <glog/logging.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

#define REQUIRED_INI_VERSION 1

enum ExitCode : int {
  NORMAL = 0,
  BAD_INI_FORMAT = -1,
  BAD_INI_VERSION = -2,
  RW_ERROR = -3,
  BAD_CAPTURER_COUNT = -4,
  BAD_FOURCC = -5,
  BAD_CAPTURER_TYPE = -6,
  NO_CAPTURER = -6
};

static std::string timeString(time_t t, bool localTime = true) {
  if (t == 0) {
    t = std::time(nullptr);
  }
  char tmp[20]; // "%F %T" + '\0' = 20 char
  std::strftime(&tmp[0], sizeof(tmp), "%F %T",
                localTime ? std::localtime(&t) : std::gmtime(&t));

  return std::string(tmp);
}

static time_t stringTime(const std::string &timeString) {

  time_t res = 0;
  int yyyy = 0, MM = 0, dd = 0, hh = 0, mm = 0, ss = 0;

  if (sscanf(timeString.c_str(), "%4d-%2d-%2d %2d:%2d:%2d", &yyyy, &MM, &dd,
             &hh, &mm, &ss) == 6) {
    struct tm tms = {0};
    tms.tm_year = yyyy - 1900; /* years since 1900 */
    tms.tm_mon = MM - 1;
    tms.tm_mday = dd;
    tms.tm_hour = hh;
    tms.tm_min = mm;
    tms.tm_sec = ss;

    res = mktime(&tms);
  }

  return res;
}

static uint64_t timeSinceEpochMs() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

using Strings = std::vector<std::string>;
static Strings split(const std::string &str, char delim) {
  Strings strings;
  size_t start;
  size_t end = 0;
  while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
    end = str.find(delim, start);
    strings.push_back(str.substr(start, end - start));
  }
  return strings;
}
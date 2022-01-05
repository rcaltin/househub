#include "app.h"
#include "capturer_factory.h"
#include "config_manager.h"
#include "file_manager.h"
#include "file_system.h"
#include "globals.h"
#include <csignal>

std::atomic_bool App::sExitFlag = false;

App::App() {
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGKILL, signalHandler);
}

App::~App() {}

ExitCode App::exec(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::SetStderrLogging(google::GLOG_INFO);

  LOG(INFO) << "househub is preparing... ";

#ifdef WIN32
  std::string iniFile = "C:/househub.ini";
#else
  std::string iniFile = "/etc/househub/househub.ini";
#endif
  if (argc == 2) {
    iniFile = argv[1];
  }

  // init config-manager
  if (ExitCode c = initConfigManager(iniFile)) {
    return c;
  }

  // init logging
  if (ExitCode c = initLogging()) {
    return c;
  }

  // init file-manager
  if (ExitCode c = initFileManager()) {
    return c;
  }

  // init capturers
  if (ExitCode c = initCapturers()) {
    return c;
  }

  LOG(INFO) << "househub is started.";

  // main loop
  while (!sExitFlag) {
    std::this_thread::yield();
  }

  LOG(INFO) << "househub is stopped.";

  return ExitCode::NORMAL;
}

void App::exit() { sExitFlag = true; }

ExitCode App::initConfigManager(const std::string &iniFile) {
  auto &cm = ConfigManager::instance();

  if (!cm.init(iniFile)) {
    LOG(FATAL) << "bad ini format: " << iniFile;
    return ExitCode::BAD_INI_FORMAT;
  }

  // ini version check
  if (cm.getInt("app_settings", "ini_version") < REQUIRED_INI_VERSION) {
    LOG(FATAL) << "bad ini version: " << iniFile;
    return ExitCode::BAD_INI_VERSION;
  }

  return ExitCode::NORMAL;
}

ExitCode App::initLogging() {
  auto &cm = ConfigManager::instance();

  // create log dir
  const std::string logDir =
      cm.getString("app_settings", "log_dir", "./househub-logs/");
  if (!fs::exists(logDir) && !fs::create_directories(logDir)) {
    LOG(ERROR) << "log directory could not created: " << logDir;
  } else {
    for (int severity = 0; severity < google::NUM_SEVERITIES; ++severity) {
      google::SetLogDestination(severity, logDir.c_str());
      google::SetLogSymlink(severity, "");
    }
  }

  // set log overdue days
  google::EnableLogCleaner(cm.getInt("app_settings", "log_overdue_days", 30));

  return ExitCode::NORMAL;
}

ExitCode App::initFileManager() {
  auto &cm = ConfigManager::instance();

  FileManagerParams fmp;
  fmp.recordDir =
      cm.getString("file_manager", "record_dir", "./househub-records/");
  fmp.recordDirSizeLimitMB =
      cm.getInt("file_manager", "record_dir_size_limit_mb", 8192);
  fmp.recordDirSizeCheckIntervalSec =
      cm.getInt("file_manager", "record_dir_size_check_interval_sec", 10);
  fmp.useLocalTime = cm.getBool("file_manager", "use_localtime", false);

  auto &fm = FileManager::instance();
  if (!fm.init(fmp)) {
    LOG(FATAL) << "file directory r/w error: " << fmp.recordDir;
    return ExitCode::RW_ERROR;
  }

  return ExitCode::NORMAL;
}

ExitCode App::initCapturers() {
  auto &cm = ConfigManager::instance();

  const auto &capturers = split(cm.getString("app_settings", "capturers"), '|');
  for (const auto &capN : capturers) {
    if (cm.hasSection(capN)) {
      CapturerParams cp;
      cp.name = cm.getString(capN, "name", capN);
      cp.type = cm.getString(capN, "type", "default");
      cp.filterK = cm.getInt(capN, "filter_k", 0);
      cp.flipX = cm.getBool(capN, "flip_x", false);
      cp.flipX = cm.getBool(capN, "flip_y", false);
      cp.streamUri =
          cm.getString(capN, "stream_uri", "http://localhost/stream");
      cp.videoOutStreamParams.name = cp.name;

      cp.videoOutStreamParams.fps = cm.getInt(capN, "output_fps", 10);

      cp.videoOutStreamParams.outputSize =
          cv::Size(cm.getInt(capN, "output_width", 1024),
                   cm.getInt(capN, "output_height", 768));

      cp.videoOutStreamParams.chunkLengthSec =
          cm.getInt(capN, "chunk_length_sec", 60);

      cp.videoOutStreamParams.uniformChunks =
          cm.getBool(capN, "uniform_chunks", true);

      cp.videoOutStreamParams.fileExtension =
          cm.getString(capN, "file_extension", ".avi");

      cp.videoOutStreamParams.watermark = cm.getBool(capN, "watermark", true);

      cp.videoOutStreamParams.useLocaltime =
          cm.getBool(capN, "use_localtime", true);

      const std::string fourcc = cm.getString(capN, "fourcc", "mjpg");
      if (fourcc.length() != 4) {
        LOG(FATAL) << "bad fourcc value.";
        return ExitCode::BAD_FOURCC;
      }
      std::copy(fourcc.c_str(), fourcc.c_str() + 4,
                cp.videoOutStreamParams.fourcc);

      auto cap = CapturerFactory::createCapturer(cp);
      if (!cap) {
        LOG(FATAL) << "bad capturer type: " << cp.type;
        return ExitCode::BAD_CAPTURER_TYPE;
      }

      // init
      if (cap->init(cp)) {
        cap->startCapture();
        mCapturers.push_back(std::move(cap));
      } else {
        LOG(ERROR) << "capturer: " << cp.name
                   << " initialization error. skipped.";
      }
    } else {
      LOG(WARNING)
          << "capturer (" << capN
          << ") expected but definition not found in the ini file. skipped.";
    }
  }

  // check capturer count
  if (mCapturers.size() == 0) {
    LOG(FATAL) << "no capturer loaded. check ini definitions.";
    return ExitCode::NO_CAPTURER;
  }

  return ExitCode::NORMAL;
}

void App::signalHandler(int signum) {
  LOG(INFO) << "!! Signal " << signum << " received. Terminating... !!";

  exit();
}
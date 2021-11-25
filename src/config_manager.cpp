#include "config_manager.h"
#include <algorithm>
#include <ini.h>

ConfigManager::~ConfigManager() {}

ConfigManager &ConfigManager::instance() {
  static ConfigManager cm;
  return cm;
}

bool ConfigManager::init(const std::string &iniFile) {

  return ini_parse(iniFile.c_str(), iniHandler, &mMap) == 0;
}

bool ConfigManager::getBool(const std::string &section, const std::string &key,
                            bool defaultValue) const {
  std::string v = getRaw(section, key);

  if (!v.empty()) {
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    return v == "true" || v == "yes" || v == "on" || v == "1" ||
           v == "enabled" || v == "active";
  }

  return defaultValue;
}

long ConfigManager::getInt(const std::string &section, const std::string &key,
                           long defaultValue) const {
  const std::string v = getRaw(section, key);
  const char *vptr = v.c_str();
  char *eptr = nullptr;
  const long value = strtol(vptr, &eptr, 0);
  return eptr > vptr ? value : defaultValue;
}

double ConfigManager::getDouble(const std::string &section,
                                const std::string &key,
                                double defaultValue) const {
  const std::string v = getRaw(section, key);
  const char *vptr = v.c_str();
  char *eptr = nullptr;
  const double value = strtod(vptr, &eptr);
  return eptr > vptr ? value : defaultValue;
}

std::string ConfigManager::getString(const std::string &section,
                                     const std::string &key,
                                     const std::string &defaultValue) const {
  const std::string v = getRaw(section, key);
  return v.empty() ? defaultValue : v;
}

std::string ConfigManager::getRaw(const std::string &section,
                                  const std::string &key) const {
  const std::string k{section + "|" + key};

  auto it = mMap.find(k);
  return it != mMap.end() ? it->second : "";
}

int ConfigManager::iniHandler(void *userPtr, const char *section,
                              const char *key, const char *value) {
  if (key) {
    KeyValueMap *map = static_cast<KeyValueMap *>(userPtr);
    (*map)[std::string(section) + "|" + std::string(key)] = value;
  }
  return 1;
}
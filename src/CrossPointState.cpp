#include "CrossPointState.h"

#include <HardwareSerial.h>
#include <Preferences.h>

namespace {
// Use ESP32's built-in NVS (Non-Volatile Storage) instead of SD card
// This is much faster and more reliable than SD card writes
Preferences preferences;
constexpr char PREFS_NAMESPACE[] = "app_state";
constexpr char KEY_EPUB_PATH[] = "epub_path";
constexpr char KEY_SLEEP_IMAGE[] = "sleep_img";
constexpr char KEY_WAS_IN_READER[] = "was_reader";
}  // namespace

CrossPointState CrossPointState::instance;

bool CrossPointState::saveToFile() const {
  // Open NVS namespace in read-write mode
  if (!preferences.begin(PREFS_NAMESPACE, false)) {
    Serial.printf("[%lu] [CPS] Failed to open NVS namespace\n", millis());
    return false;
  }

  // Write all state data to NVS (Flash memory)
  preferences.putString(KEY_EPUB_PATH, openEpubPath.c_str());
  preferences.putUChar(KEY_SLEEP_IMAGE, lastSleepImage);
  preferences.putBool(KEY_WAS_IN_READER, wasInReader);

  preferences.end();

  Serial.printf("[%lu] [CPS] State saved to Flash (wasInReader=%d)\n", millis(), wasInReader);
  return true;
}

bool CrossPointState::loadFromFile() {
  // Open NVS namespace in read-only mode
  if (!preferences.begin(PREFS_NAMESPACE, true)) {
    Serial.printf("[%lu] [CPS] Failed to open NVS namespace for reading\n", millis());
    return false;
  }

  // Read all state data from NVS (Flash memory)
  openEpubPath = preferences.getString(KEY_EPUB_PATH, "").c_str();
  lastSleepImage = preferences.getUChar(KEY_SLEEP_IMAGE, 0);
  wasInReader = preferences.getBool(KEY_WAS_IN_READER, false);

  preferences.end();

  Serial.printf("[%lu] [CPS] State loaded from Flash (wasInReader=%d)\n", millis(), wasInReader);
  return true;
}

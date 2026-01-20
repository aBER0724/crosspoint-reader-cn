#include "CrossPointSettings.h"

#include <HardwareSerial.h>
#include <SDCardManager.h>
#include <Serialization.h>

#include <cstring>

#include "fontIds.h"

// Initialize the static instance
CrossPointSettings CrossPointSettings::instance;

namespace {
constexpr uint8_t SETTINGS_FILE_VERSION = 3;  // Appended ASCII spacing fields
// Number of POD settings items (excluding strings)
constexpr uint8_t SETTINGS_POD_COUNT = 21;
constexpr char SETTINGS_FILE[] = "/.crosspoint/settings.bin";

uint8_t clampSpacingStorage(const uint8_t value, const uint8_t minValue,
                            const uint8_t maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

// Helper to read strings safely
void readStringSafe(FsFile &file, std::string &s) {
  uint32_t len;
  serialization::readPod(file, len);
  if (len > 1024) { // Don't allocate more than 1KB
    s = "";
    return;
  }
  s.resize(len);
  if (len > 0) {
    file.read(&s[0], len);
  }
}
} // namespace

bool CrossPointSettings::saveToFile() const {
  SdMan.mkdir("/.crosspoint");

  FsFile outputFile;
  if (!SdMan.openFileForWrite("CPS", SETTINGS_FILE, outputFile)) {
    return false;
  }

  // 1. Version and Item Count
  serialization::writePod(outputFile, SETTINGS_FILE_VERSION);
  serialization::writePod(outputFile, SETTINGS_POD_COUNT);

  // 2. All POD fields (sequential)
  serialization::writePod(outputFile, sleepScreen);
  serialization::writePod(outputFile, extraParagraphSpacing);
  serialization::writePod(outputFile, shortPwrBtn);
  serialization::writePod(outputFile, statusBar);
  serialization::writePod(outputFile, orientation);
  serialization::writePod(outputFile, frontButtonLayout);
  serialization::writePod(outputFile, sideButtonLayout);
  serialization::writePod(outputFile, fontFamily);
  serialization::writePod(outputFile, fontSize);
  serialization::writePod(outputFile, lineSpacing);
  serialization::writePod(outputFile, colorMode);
  serialization::writePod(outputFile, sleepTimeout);
  serialization::writePod(outputFile, refreshFrequency);
  serialization::writePod(outputFile, screenMargin);
  serialization::writePod(outputFile, sleepScreenCoverMode);
  serialization::writePod(outputFile, textAntiAliasing);
  serialization::writePod(outputFile, hideBatteryPercentage);
  serialization::writePod(outputFile, longPressChapterSkip);
  serialization::writePod(outputFile, asciiLetterSpacing);
  serialization::writePod(outputFile, asciiDigitSpacing);
  serialization::writePod(outputFile, cjkSpacing);

  // 3. String fields (at the end)
  serialization::writeString(outputFile, std::string(opdsServerUrl));
  serialization::writeString(outputFile, std::string(sleepImagePath));

  outputFile.close();
  Serial.printf("[%lu] [CPS] Settings saved successfully. Wallpaper: %s\n",
                millis(), sleepImagePath);
  return true;
}

bool CrossPointSettings::loadFromFile() {
  FsFile inputFile;
  if (!SdMan.openFileForRead("CPS", SETTINGS_FILE, inputFile)) {
    return false;
  }

  uint8_t version;
  serialization::readPod(inputFile, version);
  if (version != SETTINGS_FILE_VERSION) {
    inputFile.close();
    return false;
  }

  uint8_t fileSettingsCount = 0;
  serialization::readPod(inputFile, fileSettingsCount);

  // Load available POD settings
  uint8_t read = 0;
  do {
    serialization::readPod(inputFile, sleepScreen);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, extraParagraphSpacing);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, shortPwrBtn);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, statusBar);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, orientation);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, frontButtonLayout);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, sideButtonLayout);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, fontFamily);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, fontSize);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, lineSpacing);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, colorMode);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, sleepTimeout);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, refreshFrequency);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, screenMargin);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, sleepScreenCoverMode);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, textAntiAliasing);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, hideBatteryPercentage);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, longPressChapterSkip);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, asciiLetterSpacing);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, asciiDigitSpacing);
    if (++read >= fileSettingsCount)
      break;
    serialization::readPod(inputFile, cjkSpacing);
    if (++read >= fileSettingsCount)
      break;
  } while (false);

  // Skip remaining pods if any
  for (; read < fileSettingsCount; ++read) {
    uint8_t dummy;
    serialization::readPod(inputFile, dummy);
  }

  // Load strings (even if pods were fewer)
  std::string urlStr, pathStr;
  readStringSafe(inputFile, urlStr);
  strncpy(opdsServerUrl, urlStr.c_str(), sizeof(opdsServerUrl) - 1);
  opdsServerUrl[sizeof(opdsServerUrl) - 1] = '\0';

  readStringSafe(inputFile, pathStr);
  strncpy(sleepImagePath, pathStr.c_str(), sizeof(sleepImagePath) - 1);
  sleepImagePath[sizeof(sleepImagePath) - 1] = '\0';

  inputFile.close();
  Serial.printf("[%lu] [CPS] Settings loaded. Wallpaper path: %s\n", millis(),
                sleepImagePath);
  return true;
}

float CrossPointSettings::getReaderLineCompression() const {
  switch (lineSpacing) {
  case TIGHT:
    return 0.75f;  // Reduced from 0.85f for tighter CJK text spacing
  case WIDE:
    return 1.2f;
  case NORMAL:
  default:
    return 0.95f;  // Slightly reduced from 1.0f for better CJK text density
  }
}

unsigned long CrossPointSettings::getSleepTimeoutMs() const {
  switch (sleepTimeout) {
  case SLEEP_1_MIN:
    return 1UL * 60 * 1000;
  case SLEEP_5_MIN:
    return 5UL * 60 * 1000;
  case SLEEP_15_MIN:
    return 15UL * 60 * 1000;
  case SLEEP_30_MIN:
    return 30UL * 60 * 1000;
  case SLEEP_10_MIN:
  default:
    return 10UL * 60 * 1000;
  }
}

int CrossPointSettings::getRefreshFrequency() const {
  switch (refreshFrequency) {
  case REFRESH_1:
    return 1;
  case REFRESH_5:
    return 5;
  case REFRESH_10:
    return 10;
  case REFRESH_30:
    return 30;
  case REFRESH_15:
  default:
    return 15;
  }
}

int CrossPointSettings::getReaderFontId() const {
  switch (fontSize) {
  case SMALL:
    return NOTOSANS_12_FONT_ID;
  case LARGE:
    return NOTOSANS_16_FONT_ID;
  case MEDIUM:
  default:
    return NOTOSANS_14_FONT_ID;
  }
}

bool CrossPointSettings::isDarkMode() const {
  return colorMode == DARK_MODE;
}

int CrossPointSettings::getAsciiLetterSpacing() const {
  const uint8_t raw =
      clampSpacingStorage(asciiLetterSpacing, ASCII_SPACING_STORAGE_MIN,
                          ASCII_SPACING_STORAGE_MAX);
  return static_cast<int>(raw) - ASCII_SPACING_OFFSET;
}

int CrossPointSettings::getAsciiDigitSpacing() const {
  const uint8_t raw =
      clampSpacingStorage(asciiDigitSpacing, ASCII_SPACING_STORAGE_MIN,
                          ASCII_SPACING_STORAGE_MAX);
  return static_cast<int>(raw) - ASCII_SPACING_OFFSET;
}

int CrossPointSettings::getCjkSpacing() const {
  const uint8_t raw =
      clampSpacingStorage(cjkSpacing, CJK_SPACING_STORAGE_MIN,
                          CJK_SPACING_STORAGE_MAX);
  return static_cast<int>(raw) - CJK_SPACING_OFFSET;
}

#include "SettingsActivity.h"
#include "../reader/FileSelectionActivity.h"

#include <GfxRenderer.h>
#include <HardwareSerial.h>
#include <I18n.h>

#include <cstring>

#include "CalibreSettingsActivity.h"
#include "CrossPointSettings.h"
#include "FontSelectActivity.h"
#include "MappedInputManager.h"
#include "OtaUpdateActivity.h"
#include "fontIds.h"

// Define the static settings list
namespace {
constexpr int settingsCount = 23;
const SettingInfo settingsList[settingsCount] = {
    // Should match with SLEEP_SCREEN_MODE
    SettingInfo::Enum("Sleep Screen", &CrossPointSettings::sleepScreen,
                      {"Dark", "Light", "Custom", "Cover", "None"}),
    SettingInfo::Enum("Sleep Screen Cover Mode",
                      &CrossPointSettings::sleepScreenCoverMode,
                      {"Fit", "Crop"}),
    SettingInfo::Enum("Status Bar", &CrossPointSettings::statusBar,
                      {"None", "No Progress", "Full"}),
    SettingInfo::Enum("Hide Battery %",
                      &CrossPointSettings::hideBatteryPercentage,
                      {"Never", "In Reader", "Always"}),
    SettingInfo::Toggle("Extra Paragraph Spacing",
                        &CrossPointSettings::extraParagraphSpacing),
    SettingInfo::Toggle("Text Anti-Aliasing",
                        &CrossPointSettings::textAntiAliasing),
    SettingInfo::Enum("Short Power Button Click",
                      &CrossPointSettings::shortPwrBtn,
                      {"Ignore", "Sleep", "Page Turn"}),
    SettingInfo::Enum(
        "Reading Orientation", &CrossPointSettings::orientation,
        {"Portrait", "Landscape CW", "Inverted", "Landscape CCW"}),
    SettingInfo::Enum("Front Button Layout",
                      &CrossPointSettings::frontButtonLayout,
                      {"Bck, Cnfrm, Lft, Rght", "Lft, Rght, Bck, Cnfrm",
                       "Lft, Bck, Cnfrm, Rght"}),
    SettingInfo::Enum("Side Button Layout (reader)",
                      &CrossPointSettings::sideButtonLayout,
                      {"Prev, Next", "Next, Prev"}),
    SettingInfo::Toggle("Long-press Chapter Skip",
                        &CrossPointSettings::longPressChapterSkip),
    SettingInfo::Action("External Chinese Font"),
    SettingInfo::Action("External UI Font"),
    SettingInfo::Enum("Reader Font Size", &CrossPointSettings::fontSize,
                      {"Small", "Medium", "Large"}),
    SettingInfo::Enum("Reader Line Spacing", &CrossPointSettings::lineSpacing,
                      {"Tight", "Normal", "Wide"}),
    SettingInfo::Value("Reader Screen Margin",
                       &CrossPointSettings::screenMargin, {5, 40, 5}),
    SettingInfo::Enum("Reader Paragraph Alignment",
                      &CrossPointSettings::paragraphAlignment,
                      {"Justify", "Left", "Center", "Right"}),
    SettingInfo::Enum("Time to Sleep", &CrossPointSettings::sleepTimeout,
                      {"1 min", "5 min", "10 min", "15 min", "30 min"}),
    SettingInfo::Enum(
        "Refresh Frequency", &CrossPointSettings::refreshFrequency,
        {"1 page", "5 pages", "10 pages", "15 pages", "30 pages"}),
    SettingInfo::Action("Language"),
    SettingInfo::Action("Select Wallpaper"),
    SettingInfo::Action("Calibre Settings"),
    SettingInfo::Action("Check for updates")};

// Translate setting name to localized string
const char *translateSettingName(const char *name) {
  if (strcmp(name, "Sleep Screen") == 0)
    return TR(SLEEP_SCREEN);
  if (strcmp(name, "Sleep Screen Cover Mode") == 0)
    return TR(SLEEP_COVER_MODE);
  if (strcmp(name, "Status Bar") == 0)
    return TR(STATUS_BAR);
  if (strcmp(name, "Hide Battery %") == 0)
    return TR(HIDE_BATTERY);
  if (strcmp(name, "Extra Paragraph Spacing") == 0)
    return TR(EXTRA_SPACING);
  if (strcmp(name, "Text Anti-Aliasing") == 0)
    return TR(TEXT_AA);
  if (strcmp(name, "Short Power Button Click") == 0)
    return TR(SHORT_PWR_BTN);
  if (strcmp(name, "Reading Orientation") == 0)
    return TR(ORIENTATION);
  if (strcmp(name, "Front Button Layout") == 0)
    return TR(FRONT_BTN_LAYOUT);
  if (strcmp(name, "Side Button Layout (reader)") == 0)
    return TR(SIDE_BTN_LAYOUT);
  if (strcmp(name, "Long-press Chapter Skip") == 0)
    return TR(LONG_PRESS_SKIP);
  if (strcmp(name, "External Chinese Font") == 0)
    return TR(EXT_CHINESE_FONT);
  if (strcmp(name, "External UI Font") == 0)
    return TR(EXT_UI_FONT);
  if (strcmp(name, "Reader Font Size") == 0)
    return TR(FONT_SIZE);
  if (strcmp(name, "Reader Line Spacing") == 0)
    return TR(LINE_SPACING);
  if (strcmp(name, "Reader Screen Margin") == 0)
    return TR(SCREEN_MARGIN);
  if (strcmp(name, "Reader Paragraph Alignment") == 0)
    return TR(PARA_ALIGNMENT);
  if (strcmp(name, "Time to Sleep") == 0)
    return TR(TIME_TO_SLEEP);
  if (strcmp(name, "Refresh Frequency") == 0)
    return TR(REFRESH_FREQ);
  if (strcmp(name, "Language") == 0)
    return TR(LANGUAGE);
  if (strcmp(name, "Calibre Settings") == 0)
    return TR(CALIBRE_SETTINGS);
  if (strcmp(name, "Check for updates") == 0)
    return TR(CHECK_UPDATES);
  if (strcmp(name, "Select Wallpaper") == 0)
    return TR(SELECT_WALLPAPER);
  return name; // Fallback to original
}

// Translate setting value to localized string
const char *translateSettingValue(const char *value) {
  // Sleep Screen values
  if (strcmp(value, "Dark") == 0)
    return TR(DARK);
  if (strcmp(value, "Light") == 0)
    return TR(LIGHT);
  if (strcmp(value, "Custom") == 0)
    return TR(CUSTOM);
  if (strcmp(value, "Cover") == 0)
    return TR(COVER);
  if (strcmp(value, "None") == 0)
    return TR(NONE);
  // Cover Mode
  if (strcmp(value, "Fit") == 0)
    return TR(FIT);
  if (strcmp(value, "Crop") == 0)
    return TR(CROP);
  // Status Bar
  if (strcmp(value, "No Progress") == 0)
    return TR(NO_PROGRESS);
  if (strcmp(value, "Full") == 0)
    return TR(FULL);
  // Hide Battery %
  if (strcmp(value, "Never") == 0)
    return TR(NEVER);
  if (strcmp(value, "In Reader") == 0)
    return TR(IN_READER);
  if (strcmp(value, "Always") == 0)
    return TR(ALWAYS);
  // Short Power Button Click
  if (strcmp(value, "Ignore") == 0)
    return TR(IGNORE);
  if (strcmp(value, "Sleep") == 0)
    return TR(SLEEP);
  if (strcmp(value, "Page Turn") == 0)
    return TR(PAGE_TURN);
  // Orientation
  if (strcmp(value, "Portrait") == 0)
    return TR(PORTRAIT);
  if (strcmp(value, "Landscape CW") == 0)
    return TR(LANDSCAPE_CW);
  if (strcmp(value, "Inverted") == 0)
    return TR(INVERTED);
  if (strcmp(value, "Landscape CCW") == 0)
    return TR(LANDSCAPE_CCW);
  // Button Layout
  if (strcmp(value, "Prev, Next") == 0)
    return TR(PREV_NEXT);
  if (strcmp(value, "Next, Prev") == 0)
    return TR(NEXT_PREV);
  // Font Size
  if (strcmp(value, "Small") == 0)
    return TR(SMALL);
  if (strcmp(value, "Medium") == 0)
    return TR(MEDIUM);
  if (strcmp(value, "Large") == 0)
    return TR(LARGE);
  if (strcmp(value, "X Large") == 0)
    return TR(X_LARGE);
  // Line Spacing
  if (strcmp(value, "Tight") == 0)
    return TR(TIGHT);
  if (strcmp(value, "Normal") == 0)
    return TR(NORMAL);
  if (strcmp(value, "Wide") == 0)
    return TR(WIDE);
  // Alignment
  if (strcmp(value, "Justify") == 0)
    return TR(JUSTIFY);
  if (strcmp(value, "Left") == 0)
    return TR(LEFT);
  if (strcmp(value, "Center") == 0)
    return TR(CENTER);
  if (strcmp(value, "Right") == 0)
    return TR(RIGHT);
  // Time to Sleep
  if (strcmp(value, "1 min") == 0)
    return TR(MIN_1);
  if (strcmp(value, "5 min") == 0)
    return TR(MIN_5);
  if (strcmp(value, "10 min") == 0)
    return TR(MIN_10);
  if (strcmp(value, "15 min") == 0)
    return TR(MIN_15);
  if (strcmp(value, "30 min") == 0)
    return TR(MIN_30);
  // Refresh Frequency
  if (strcmp(value, "1 page") == 0)
    return TR(PAGES_1);
  if (strcmp(value, "5 pages") == 0)
    return TR(PAGES_5);
  if (strcmp(value, "10 pages") == 0)
    return TR(PAGES_10);
  if (strcmp(value, "15 pages") == 0)
    return TR(PAGES_15);
  if (strcmp(value, "30 pages") == 0)
    return TR(PAGES_30);
  // Toggle values
  if (strcmp(value, "ON") == 0)
    return TR(ON);
  if (strcmp(value, "OFF") == 0)
    return TR(OFF);
  return value; // Fallback to original
}
} // namespace

void SettingsActivity::taskTrampoline(void *param) {
  auto *self = static_cast<SettingsActivity *>(param);
  self->displayTaskLoop();
}

void SettingsActivity::onEnter() {
  Activity::onEnter();
  renderingMutex = xSemaphoreCreateMutex();

  // Reset selection to first item
  selectedSettingIndex = 0;

  // Trigger first update
  updateRequired = true;

  xTaskCreate(&SettingsActivity::taskTrampoline, "SettingsActivityTask",
              4096,              // Stack size
              this,              // Parameters
              1,                 // Priority
              &displayTaskHandle // Task handle
  );
}

void SettingsActivity::onExit() {
  ActivityWithSubactivity::onExit();

  // Wait until not rendering to delete task to avoid killing mid-instruction to
  // EPD
  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;
}

void SettingsActivity::loop() {
  if (subActivity) {
    subActivity->loop();
    return;
  }

  // Handle actions with early return
  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    toggleCurrentSetting();
    updateRequired = true;
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    SETTINGS.saveToFile();
    onGoHome();
    return;
  }

  // Handle navigation
  if (mappedInput.wasPressed(MappedInputManager::Button::Up) ||
      mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    // Move selection up (with wrap-around)
    selectedSettingIndex = (selectedSettingIndex > 0)
                               ? (selectedSettingIndex - 1)
                               : (settingsCount - 1);
    updateRequired = true;
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Down) ||
             mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    // Move selection down (with wrap around)
    selectedSettingIndex = (selectedSettingIndex < settingsCount - 1)
                               ? (selectedSettingIndex + 1)
                               : 0;
    updateRequired = true;
  }
}

void SettingsActivity::toggleCurrentSetting() {
  // Validate index
  if (selectedSettingIndex < 0 || selectedSettingIndex >= settingsCount) {
    return;
  }

  const auto &setting = settingsList[selectedSettingIndex];

  if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
    // Toggle the boolean value using the member pointer
    const bool currentValue = SETTINGS.*(setting.valuePtr);
    SETTINGS.*(setting.valuePtr) = !currentValue;
  } else if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
    const uint8_t currentValue = SETTINGS.*(setting.valuePtr);
    SETTINGS.*(setting.valuePtr) =
        (currentValue + 1) % static_cast<uint8_t>(setting.enumValues.size());

    // Hot update: Apply UI font size change immediately
    if (setting.valuePtr == &CrossPointSettings::fontSize) {
      renderer.setUiFontSize(SETTINGS.fontSize);
      Serial.printf("[%lu] [SET] UI font size updated to %d (%dpx)\n", millis(),
                    SETTINGS.fontSize, 20 + SETTINGS.fontSize * 2);
    }
  } else if (setting.type == SettingType::VALUE &&
             setting.valuePtr != nullptr) {
    // Decreasing would also be nice for large ranges I think but oh well can't
    // have everything
    const int8_t currentValue = SETTINGS.*(setting.valuePtr);
    // Wrap to minValue if exceeding setting value boundary
    if (currentValue + setting.valueRange.step > setting.valueRange.max) {
      SETTINGS.*(setting.valuePtr) = setting.valueRange.min;
    } else {
      SETTINGS.*(setting.valuePtr) = currentValue + setting.valueRange.step;
    }
  } else if (setting.type == SettingType::ACTION) {
    if (strcmp(setting.name, "Calibre Settings") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(
          new CalibreSettingsActivity(renderer, mappedInput, [this] {
            exitActivity();
            updateRequired = true;
          }));
      xSemaphoreGive(renderingMutex);
    } else if (strcmp(setting.name, "Check for updates") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new OtaUpdateActivity(renderer, mappedInput, [this] {
        exitActivity();
        updateRequired = true;
      }));
      xSemaphoreGive(renderingMutex);
    } else if (strcmp(setting.name, "External Chinese Font") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new FontSelectActivity(
          renderer, mappedInput, FontSelectActivity::SelectMode::Reader,
          [this] {
            exitActivity();
            updateRequired = true;
          }));
      xSemaphoreGive(renderingMutex);
    } else if (strcmp(setting.name, "External UI Font") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new FontSelectActivity(
          renderer, mappedInput, FontSelectActivity::SelectMode::UI, [this] {
            exitActivity();
            updateRequired = true;
          }));
      xSemaphoreGive(renderingMutex);
    } else if (strcmp(setting.name, "Language") == 0) {
      // Toggle language: English -> Chinese -> Japanese -> English
      const auto currentLang = I18N.getLanguage();
      Language newLang;
      switch (currentLang) {
      case Language::ENGLISH:
        newLang = Language::CHINESE;
        break;
      case Language::CHINESE:
        newLang = Language::JAPANESE;
        break;
      case Language::JAPANESE:
      default:
        newLang = Language::ENGLISH;
        break;
      }
      I18N.setLanguage(newLang);
    } else if (strcmp(setting.name, "Select Wallpaper") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new FileSelectionActivity(
          renderer, mappedInput,
          [this](const std::string &path) {
            strncpy(SETTINGS.sleepImagePath, path.c_str(),
                    sizeof(SETTINGS.sleepImagePath) - 1);
            SETTINGS.sleepImagePath[sizeof(SETTINGS.sleepImagePath) - 1] = '\0';
            // Auto switch to custom mode
            SETTINGS.sleepScreen =
                CrossPointSettings::SLEEP_SCREEN_MODE::CUSTOM;
            SETTINGS.saveToFile();
            exitActivity();
            onGoHome(); // Return to home after setting wallpaper
          },
          [this] {
            exitActivity();
            updateRequired = true;
          }));
      xSemaphoreGive(renderingMutex);
    }
  } else {
    // Only toggle if it's a toggle type and has a value pointer
    return;
  }

  // Save settings when they change
  SETTINGS.saveToFile();
}

void SettingsActivity::displayTaskLoop() {
  while (true) {
    if (updateRequired && !subActivity) {
      updateRequired = false;
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      render();
      xSemaphoreGive(renderingMutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void SettingsActivity::render() const {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  // Calculate row height based on current UI font size
  // Font sizes: SMALL=18px, MEDIUM=20px, LARGE=22px
  // Row height = font height + spacing (8-12px)
  const int rowHeight = 20 + renderer.getUiFontSize() * 2 +
                        10; // 30px/32px/34px for small/medium/large

  // Draw header
  renderer.drawCenteredText(UI_12_FONT_ID, 15, TR(SETTINGS_TITLE), true,
                            EpdFontFamily::BOLD);

  // Draw selection
  renderer.fillRect(0, 60 + selectedSettingIndex * rowHeight - 2, pageWidth - 1,
                    rowHeight);

  // Draw all settings
  for (int i = 0; i < settingsCount; i++) {
    const int settingY =
        60 + i * rowHeight; // Dynamic row height based on font size

    // Draw setting name (translated)
    renderer.drawText(UI_10_FONT_ID, 20, settingY,
                      translateSettingName(settingsList[i].name),
                      i != selectedSettingIndex);

    // Draw value based on setting type
    const char *valueText = "";
    if (settingsList[i].type == SettingType::TOGGLE &&
        settingsList[i].valuePtr != nullptr) {
      const bool value = SETTINGS.*(settingsList[i].valuePtr);
      valueText = value ? TR(ON) : TR(OFF);
    } else if (settingsList[i].type == SettingType::ENUM &&
               settingsList[i].valuePtr != nullptr) {
      const uint8_t value = SETTINGS.*(settingsList[i].valuePtr);
      valueText =
          translateSettingValue(settingsList[i].enumValues[value].c_str());
    } else if (settingsList[i].type == SettingType::VALUE &&
               settingsList[i].valuePtr != nullptr) {
      // Value type shows raw number - no translation needed
      static char valueBuf[16];
      snprintf(valueBuf, sizeof(valueBuf), "%d",
               static_cast<int>(SETTINGS.*(settingsList[i].valuePtr)));
      valueText = valueBuf;
    } else if (settingsList[i].type == SettingType::ACTION &&
               strcmp(settingsList[i].name, "Language") == 0) {
      // Show current language for Language setting
      switch (I18N.getLanguage()) {
      case Language::CHINESE:
        valueText = TR(CHINESE);
        break;
      case Language::JAPANESE:
        valueText = TR(JAPANESE);
        break;
      case Language::ENGLISH:
      default:
        valueText = TR(ENGLISH);
        break;
      }
    } else if (settingsList[i].type == SettingType::ACTION &&
               strcmp(settingsList[i].name, "Select Wallpaper") == 0) {
      if (strlen(SETTINGS.sleepImagePath) > 0) {
        // Show only the filename, not the full path
        const char *slash = strrchr(SETTINGS.sleepImagePath, '/');
        valueText = slash ? slash + 1 : SETTINGS.sleepImagePath;
      } else {
        valueText = TR(NONE);
      }
    }
    const auto width = renderer.getTextWidth(UI_10_FONT_ID, valueText);
    renderer.drawText(UI_10_FONT_ID, pageWidth - 20 - width, settingY,
                      valueText, i != selectedSettingIndex);
  }

  // Draw version text above button hints
  renderer.drawText(
      SMALL_FONT_ID,
      pageWidth - 20 - renderer.getTextWidth(SMALL_FONT_ID, CROSSPOINT_VERSION),
      pageHeight - 60, CROSSPOINT_VERSION);

  // Draw help text
  const auto labels = mappedInput.mapLabels(TR(SAVE), TR(TOGGLE), "", "");
  renderer.drawButtonHints(UI_10_FONT_ID, labels.btn1, labels.btn2, labels.btn3,
                           labels.btn4);

  // Always use standard refresh for settings screen
  renderer.displayBuffer();
}

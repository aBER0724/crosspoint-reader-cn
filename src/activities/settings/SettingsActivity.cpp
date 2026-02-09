#include "SettingsActivity.h"

#include <FontManager.h>
#include <GfxRenderer.h>
#include <HardwareSerial.h>
#include <I18n.h>

#include "ButtonRemapActivity.h"
#include "CalibreSettingsActivity.h"
#include "FontSelectActivity.h"
#include "ClearCacheActivity.h"
#include "CrossPointSettings.h"
#include "KOReaderSettingsActivity.h"
#include "LanguageSelectActivity.h"
#include "MappedInputManager.h"
#include "OtaUpdateActivity.h"
#include "OrientationHelper.h"
#include "SettingsList.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {

// Translate setting display names from English keys to localized strings
const char* translateSettingName(const char* name) {
  // Display
  if (strcmp(name, "Sleep Screen") == 0) return TR(SLEEP_SCREEN);
  if (strcmp(name, "Sleep Screen Cover Mode") == 0) return TR(SLEEP_COVER_MODE);
  if (strcmp(name, "Sleep Screen Cover Filter") == 0) return TR(SLEEP_COVER_FILTER);
  if (strcmp(name, "Status Bar") == 0) return TR(STATUS_BAR);
  if (strcmp(name, "Hide Battery %") == 0) return TR(HIDE_BATTERY);
  if (strcmp(name, "Refresh Frequency") == 0) return TR(REFRESH_FREQ);
  if (strcmp(name, "UI Theme") == 0) return TR(UI_THEME);
  if (strcmp(name, "Sunlight Fading Fix") == 0) return TR(FADING_FIX);
  if (strcmp(name, "Color Mode") == 0) return TR(COLOR_MODE);
  if (strcmp(name, "UI Orientation") == 0) return TR(UI_ORIENTATION);
  if (strcmp(name, "UI Font") == 0) return TR(EXT_UI_FONT);
  // Reader
  if (strcmp(name, "Font Family") == 0) return TR(FONT_FAMILY);
  if (strcmp(name, "Font Size") == 0) return TR(FONT_SIZE);
  if (strcmp(name, "Line Spacing") == 0) return TR(LINE_SPACING);
  if (strcmp(name, "Screen Margin") == 0) return TR(SCREEN_MARGIN);
  if (strcmp(name, "Paragraph Alignment") == 0) return TR(PARA_ALIGNMENT);
  if (strcmp(name, "Book's Embedded Style") == 0) return TR(EMBEDDED_STYLE);
  if (strcmp(name, "Hyphenation") == 0) return TR(HYPHENATION);
  if (strcmp(name, "Reading Orientation") == 0) return TR(ORIENTATION);
  if (strcmp(name, "Extra Paragraph Spacing") == 0) return TR(EXTRA_SPACING);
  if (strcmp(name, "Text Anti-Aliasing") == 0) return TR(TEXT_AA);
  if (strcmp(name, "First Line Indent") == 0) return TR(FIRST_LINE_INDENT);
  // Controls
  if (strcmp(name, "Remap Front Buttons") == 0) return TR(REMAP_BUTTONS);
  if (strcmp(name, "Side Button Layout (reader)") == 0) return TR(SIDE_BTN_LAYOUT);
  if (strcmp(name, "Long-press Chapter Skip") == 0) return TR(LONG_PRESS_SKIP);
  if (strcmp(name, "Short Power Button Click") == 0) return TR(SHORT_PWR_BTN);
  // System
  if (strcmp(name, "Time to Sleep") == 0) return TR(TIME_TO_SLEEP);
  if (strcmp(name, "Language") == 0) return TR(LANGUAGE);
  if (strcmp(name, "KOReader Sync") == 0) return TR(KOREADER_SYNC);
  if (strcmp(name, "Clear Cache") == 0) return TR(CLEAR_READING_CACHE);
  if (strcmp(name, "Check for updates") == 0) return TR(CHECK_UPDATES);
  return name;
}

// Translate enum/value display text from English to localized strings
const char* translateEnumValue(const std::string& value) {
  // Sleep screen modes
  if (value == "Dark") return TR(DARK);
  if (value == "Light") return TR(LIGHT);
  if (value == "Custom") return TR(CUSTOM);
  if (value == "Cover") return TR(COVER);
  if (value == "None") return TR(NONE);
  if (value == "Cover + Custom") return TR(COVER_CUSTOM);
  // Cover mode
  if (value == "Fit") return TR(FIT);
  if (value == "Crop") return TR(CROP);
  // Cover filter
  if (value == "Contrast") return TR(CONTRAST);
  if (value == "Inverted") return TR(INVERTED);
  // Status bar
  if (value == "No Progress") return TR(NO_PROGRESS);
  if (value == "Full w/ Percentage") return TR(STATUS_FULL_PCT);
  if (value == "Full w/ Book Bar") return TR(STATUS_FULL_BOOK);
  if (value == "Book Bar Only") return TR(STATUS_BOOK_ONLY);
  if (value == "Full w/ Chapter Bar") return TR(STATUS_FULL_CHAPTER);
  // Hide battery
  if (value == "Never") return TR(NEVER);
  if (value == "In Reader") return TR(IN_READER);
  if (value == "Always") return TR(ALWAYS);
  // UI Theme
  if (value == "Classic") return TR(CLASSIC);
  if (value == "Lyra") return TR(LYRA);
  // Orientation
  if (value == "Portrait") return TR(PORTRAIT);
  if (value == "Landscape CW") return TR(LANDSCAPE_CW);
  if (value == "Landscape CCW") return TR(LANDSCAPE_CCW);
  // Font family
  if (value == "Bookerly") return TR(BOOKERLY);
  if (value == "Noto Sans") return TR(NOTO_SANS);
  if (value == "Open Dyslexic") return TR(OPEN_DYSLEXIC);
  // Font size
  if (value == "Small") return TR(SMALL);
  if (value == "Medium") return TR(MEDIUM);
  if (value == "Large") return TR(LARGE);
  if (value == "X Large") return TR(X_LARGE);
  // Line spacing
  if (value == "Tight") return TR(TIGHT);
  if (value == "Normal") return TR(NORMAL);
  if (value == "Wide") return TR(WIDE);
  // Paragraph alignment
  if (value == "Justify") return TR(JUSTIFY);
  if (value == "Left") return TR(LEFT);
  if (value == "Center") return TR(CENTER);
  if (value == "Right") return TR(RIGHT);
  if (value == "Book's Style") return TR(BOOKS_STYLE);
  // Side button layout
  if (value == "Prev, Next") return TR(PREV_NEXT);
  if (value == "Next, Prev") return TR(NEXT_PREV);
  // Short power button
  if (value == "Ignore") return TR(IGNORE);
  if (value == "Sleep") return TR(SLEEP);
  if (value == "Page Turn") return TR(PAGE_TURN);
  // Time to sleep
  if (value == "1 min") return TR(MIN_1);
  if (value == "5 min") return TR(MIN_5);
  if (value == "10 min") return TR(MIN_10);
  if (value == "15 min") return TR(MIN_15);
  if (value == "30 min") return TR(MIN_30);
  // Refresh frequency
  if (value == "1 page") return TR(PAGES_1);
  if (value == "5 pages") return TR(PAGES_5);
  if (value == "10 pages") return TR(PAGES_10);
  if (value == "15 pages") return TR(PAGES_15);
  if (value == "30 pages") return TR(PAGES_30);
  return value.c_str();
}

}  // namespace

const char* SettingsActivity::categoryNames[categoryCount] = {"Display", "Reader", "Controls", "System"};

void SettingsActivity::taskTrampoline(void* param) {
  auto* self = static_cast<SettingsActivity*>(param);
  self->displayTaskLoop();
}

void SettingsActivity::onEnter() {
  Activity::onEnter();
  renderingMutex = xSemaphoreCreateMutex();

  // Build per-category vectors from the shared settings list
  displaySettings.clear();
  readerSettings.clear();
  controlsSettings.clear();
  systemSettings.clear();

  for (auto& setting : getSettingsList()) {
    if (!setting.category) continue;
    if (strcmp(setting.category, "Display") == 0) {
      displaySettings.push_back(std::move(setting));
    } else if (strcmp(setting.category, "Reader") == 0) {
      readerSettings.push_back(std::move(setting));
    } else if (strcmp(setting.category, "Controls") == 0) {
      controlsSettings.push_back(std::move(setting));
    } else if (strcmp(setting.category, "System") == 0) {
      systemSettings.push_back(std::move(setting));
    }
    // Web-only categories (KOReader Sync, OPDS Browser) are skipped for device UI
  }

  // Append device-only ACTION items
  displaySettings.push_back(SettingInfo::Action("UI Font"));
  controlsSettings.insert(controlsSettings.begin(), SettingInfo::Action("Remap Front Buttons"));
  systemSettings.push_back(SettingInfo::Action("Language"));
  systemSettings.push_back(SettingInfo::Action("KOReader Sync"));
  systemSettings.push_back(SettingInfo::Action("OPDS Browser"));
  systemSettings.push_back(SettingInfo::Action("Clear Cache"));
  systemSettings.push_back(SettingInfo::Action("Check for updates"));

  // Reset selection to first category
  selectedCategoryIndex = 0;
  selectedSettingIndex = 0;

  // Initialize with first category (Display)
  currentSettings = &displaySettings;
  settingsCount = static_cast<int>(displaySettings.size());

  // Trigger first update
  updateRequired = true;

  xTaskCreate(&SettingsActivity::taskTrampoline, "SettingsActivityTask",
              4096,               // Stack size
              this,               // Parameters
              1,                  // Priority
              &displayTaskHandle  // Task handle
  );
}

void SettingsActivity::onExit() {
  ActivityWithSubactivity::onExit();

  // Wait until not rendering to delete task to avoid killing mid-instruction to EPD
  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;

  UITheme::getInstance().reload();  // Re-apply theme in case it was changed
}

void SettingsActivity::loop() {
  if (subActivity) {
    subActivity->loop();
    return;
  }
  bool hasChangedCategory = false;

  // Handle actions with early return
  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    if (selectedSettingIndex == 0) {
      selectedCategoryIndex = (selectedCategoryIndex < categoryCount - 1) ? (selectedCategoryIndex + 1) : 0;
      hasChangedCategory = true;
      updateRequired = true;
    } else {
      toggleCurrentSetting();
      updateRequired = true;
      return;
    }
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    SETTINGS.saveToFile();
    onGoHome();
    return;
  }

  // Handle navigation
  buttonNavigator.onNextRelease([this] {
    selectedSettingIndex = ButtonNavigator::nextIndex(selectedSettingIndex, settingsCount + 1);
    updateRequired = true;
  });

  buttonNavigator.onPreviousRelease([this] {
    selectedSettingIndex = ButtonNavigator::previousIndex(selectedSettingIndex, settingsCount + 1);
    updateRequired = true;
  });

  buttonNavigator.onNextContinuous([this, &hasChangedCategory] {
    hasChangedCategory = true;
    selectedCategoryIndex = ButtonNavigator::nextIndex(selectedCategoryIndex, categoryCount);
    updateRequired = true;
  });

  buttonNavigator.onPreviousContinuous([this, &hasChangedCategory] {
    hasChangedCategory = true;
    selectedCategoryIndex = ButtonNavigator::previousIndex(selectedCategoryIndex, categoryCount);
    updateRequired = true;
  });

  if (hasChangedCategory) {
    selectedSettingIndex = (selectedSettingIndex == 0) ? 0 : 1;
    switch (selectedCategoryIndex) {
      case 0:
        currentSettings = &displaySettings;
        break;
      case 1:
        currentSettings = &readerSettings;
        break;
      case 2:
        currentSettings = &controlsSettings;
        break;
      case 3:
        currentSettings = &systemSettings;
        break;
    }
    settingsCount = static_cast<int>(currentSettings->size());
  }
}

void SettingsActivity::toggleCurrentSetting() {
  int selectedSetting = selectedSettingIndex - 1;
  if (selectedSetting < 0 || selectedSetting >= settingsCount) {
    return;
  }

  const auto& setting = (*currentSettings)[selectedSetting];

  if (setting.type == SettingType::TOGGLE && setting.valuePtr != nullptr) {
    // Toggle the boolean value using the member pointer
    const bool currentValue = SETTINGS.*(setting.valuePtr);
    SETTINGS.*(setting.valuePtr) = !currentValue;
  } else if (setting.type == SettingType::ENUM && setting.valuePtr != nullptr) {
    // Font Size: skip when external font is active (fixed bitmap size)
    if (strcmp(setting.name, "Font Size") == 0 && FontMgr.isExternalFontEnabled()) {
      return;
    }
    // Font Family: open FontSelectActivity instead of cycling enum
    if (strcmp(setting.name, "Font Family") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new FontSelectActivity(renderer, mappedInput,
          FontSelectActivity::SelectMode::Reader, [this] {
            exitActivity();
            updateRequired = true;
          }));
      xSemaphoreGive(renderingMutex);
      return;
    }
    const uint8_t currentValue = SETTINGS.*(setting.valuePtr);
    SETTINGS.*(setting.valuePtr) = (currentValue + 1) % static_cast<uint8_t>(setting.enumValues.size());
  } else if (setting.type == SettingType::VALUE && setting.valuePtr != nullptr) {
    const int8_t currentValue = SETTINGS.*(setting.valuePtr);
    if (currentValue + setting.valueRange.step > setting.valueRange.max) {
      SETTINGS.*(setting.valuePtr) = setting.valueRange.min;
    } else {
      SETTINGS.*(setting.valuePtr) = currentValue + setting.valueRange.step;
    }
  } else if (setting.type == SettingType::ACTION) {
    if (strcmp(setting.name, "Remap Front Buttons") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new ButtonRemapActivity(renderer, mappedInput, [this] {
        exitActivity();
        updateRequired = true;
      }));
      xSemaphoreGive(renderingMutex);
    } else if (strcmp(setting.name, "KOReader Sync") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new KOReaderSettingsActivity(renderer, mappedInput, [this] {
        exitActivity();
        updateRequired = true;
      }));
      xSemaphoreGive(renderingMutex);
    } else if (strcmp(setting.name, "OPDS Browser") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new CalibreSettingsActivity(renderer, mappedInput, [this] {
        exitActivity();
        updateRequired = true;
      }));
      xSemaphoreGive(renderingMutex);
    } else if (strcmp(setting.name, "Clear Cache") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new ClearCacheActivity(renderer, mappedInput, [this] {
        exitActivity();
        updateRequired = true;
      }));
      xSemaphoreGive(renderingMutex);
    } else if (strcmp(setting.name, "Language") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new LanguageSelectActivity(renderer, mappedInput, [this] {
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
    } else if (strcmp(setting.name, "UI Font") == 0) {
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      exitActivity();
      enterNewActivity(new FontSelectActivity(renderer, mappedInput,
          FontSelectActivity::SelectMode::UI, [this] {
            exitActivity();
            updateRequired = true;
          }));
      xSemaphoreGive(renderingMutex);
    }
  } else {
    return;
  }

  SETTINGS.saveToFile();

  // Apply dark mode immediately when color mode setting changes
  renderer.setDarkMode(SETTINGS.colorMode == CrossPointSettings::COLOR_MODE::DARK_MODE);

  // Apply UI orientation immediately when orientation setting changes
  OrientationHelper::applyOrientation(renderer, mappedInput, this);
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

  auto metrics = UITheme::getInstance().getMetrics();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, TR(SETTINGS_TITLE));

  const char* translatedCategories[] = {TR(CAT_DISPLAY), TR(CAT_READER), TR(CAT_CONTROLS), TR(CAT_SYSTEM)};
  std::vector<TabInfo> tabs;
  tabs.reserve(categoryCount);
  for (int i = 0; i < categoryCount; i++) {
    tabs.push_back({translatedCategories[i], selectedCategoryIndex == i});
  }
  GUI.drawTabBar(renderer, Rect{0, metrics.topPadding + metrics.headerHeight, pageWidth, metrics.tabBarHeight}, tabs,
                 selectedSettingIndex == 0);

  const auto& settings = *currentSettings;
  GUI.drawList(
      renderer,
      Rect{0, metrics.topPadding + metrics.headerHeight + metrics.tabBarHeight + metrics.verticalSpacing, pageWidth,
           pageHeight - (metrics.topPadding + metrics.headerHeight + metrics.tabBarHeight + metrics.buttonHintsHeight +
                         metrics.verticalSpacing * 2)},
      settingsCount, selectedSettingIndex - 1,
      [&settings](int index) { return std::string(translateSettingName(settings[index].name)); },
      nullptr, nullptr,
      [&settings](int i) {
        std::string valueText = "";
        if (settings[i].type == SettingType::TOGGLE && settings[i].valuePtr != nullptr) {
          const bool value = SETTINGS.*(settings[i].valuePtr);
          valueText = value ? TR(ON) : TR(OFF);
        } else if (settings[i].type == SettingType::ENUM && settings[i].valuePtr != nullptr) {
          // Font Family: show external font name when external font is active
          if (strcmp(settings[i].name, "Font Family") == 0 && FontMgr.isExternalFontEnabled()) {
            const FontInfo* info = FontMgr.getFontInfo(FontMgr.getSelectedIndex());
            valueText = info ? info->name : "External";
          // Font Size: show actual pixel size when external font is active
          } else if (strcmp(settings[i].name, "Font Size") == 0 && FontMgr.isExternalFontEnabled()) {
            const FontInfo* info = FontMgr.getFontInfo(FontMgr.getSelectedIndex());
            valueText = info ? (std::to_string(info->size) + "pt") : "—";
          } else {
            const uint8_t value = SETTINGS.*(settings[i].valuePtr);
            valueText = translateEnumValue(settings[i].enumValues[value]);
          }
        } else if (settings[i].type == SettingType::VALUE && settings[i].valuePtr != nullptr) {
          valueText = std::to_string(SETTINGS.*(settings[i].valuePtr));
        } else if (settings[i].type == SettingType::ACTION && strcmp(settings[i].name, "UI Font") == 0) {
          // Show current UI font name or "Built-in"
          if (FontMgr.isUiFontEnabled()) {
            const int idx = FontMgr.getUiSelectedIndex();
            const FontInfo* info = FontMgr.getFontInfo(idx);
            valueText = info ? info->name : "External";
          } else {
            valueText = TR(BUILTIN_DISABLED);
          }
        }
        return valueText;
      });

  // Draw version text
  renderer.drawText(SMALL_FONT_ID,
                    pageWidth - metrics.versionTextRightX - renderer.getTextWidth(SMALL_FONT_ID, CROSSPOINT_VERSION),
                    metrics.versionTextY, CROSSPOINT_VERSION);

  // Draw help text
  const auto labels = mappedInput.mapLabels(TR(BACK), TR(TOGGLE), TR(DIR_UP), TR(DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  // Always use standard refresh for settings screen
  renderer.displayBuffer();
}

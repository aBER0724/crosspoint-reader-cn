#include "KOReaderSettingsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <cstring>
#include <string>

#include "KOReaderAuthActivity.h"
#include "KOReaderCredentialStore.h"
#include "MappedInputManager.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int MENU_ITEMS = 5;
}  // namespace

void KOReaderSettingsActivity::taskTrampoline(void* param) {
  auto* self = static_cast<KOReaderSettingsActivity*>(param);
  self->displayTaskLoop();
}

void KOReaderSettingsActivity::onEnter() {
  ActivityWithSubactivity::onEnter();

  renderingMutex = xSemaphoreCreateMutex();
  selectedIndex = 0;
  updateRequired = true;

  xTaskCreate(&KOReaderSettingsActivity::taskTrampoline, "KOReaderSettingsTask",
              4096,               // Stack size
              this,               // Parameters
              1,                  // Priority
              &displayTaskHandle  // Task handle
  );
}

void KOReaderSettingsActivity::onExit() {
  ActivityWithSubactivity::onExit();

  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;
}

void KOReaderSettingsActivity::loop() {
  if (subActivity) {
    subActivity->loop();
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    onBack();
    return;
  }

  if (mappedInput.wasPressed(MappedInputManager::Button::Confirm)) {
    handleSelection();
    return;
  }

  // Handle navigation
  buttonNavigator.onNext([this] {
    selectedIndex = (selectedIndex + 1) % MENU_ITEMS;
    updateRequired = true;
  });

  buttonNavigator.onPrevious([this] {
    selectedIndex = (selectedIndex + MENU_ITEMS - 1) % MENU_ITEMS;
    updateRequired = true;
  });
}

void KOReaderSettingsActivity::handleSelection() {
  xSemaphoreTake(renderingMutex, portMAX_DELAY);

  if (selectedIndex == 0) {
    // Username
    exitActivity();
    enterNewActivity(new KeyboardEntryActivity(
        renderer, mappedInput, TR(KOREADER_USERNAME), KOREADER_STORE.getUsername(), 10,
        64,     // maxLength
        false,  // not password
        [this](const std::string& username) {
          KOREADER_STORE.setCredentials(username, KOREADER_STORE.getPassword());
          KOREADER_STORE.saveToFile();
          exitActivity();
          updateRequired = true;
        },
        [this]() {
          exitActivity();
          updateRequired = true;
        }));
  } else if (selectedIndex == 1) {
    // Password
    exitActivity();
    enterNewActivity(new KeyboardEntryActivity(
        renderer, mappedInput, TR(KOREADER_PASSWORD), KOREADER_STORE.getPassword(), 10,
        64,     // maxLength
        false,  // show characters
        [this](const std::string& password) {
          KOREADER_STORE.setCredentials(KOREADER_STORE.getUsername(), password);
          KOREADER_STORE.saveToFile();
          exitActivity();
          updateRequired = true;
        },
        [this]() {
          exitActivity();
          updateRequired = true;
        }));
  } else if (selectedIndex == 2) {
    // Sync Server URL - prefill with https:// if empty to save typing
    const std::string currentUrl = KOREADER_STORE.getServerUrl();
    const std::string prefillUrl = currentUrl.empty() ? "https://" : currentUrl;
    exitActivity();
    enterNewActivity(new KeyboardEntryActivity(
        renderer, mappedInput, TR(SYNC_SERVER_URL), prefillUrl, 10,
        128,    // maxLength - URLs can be long
        false,  // not password
        [this](const std::string& url) {
          // Clear if user just left the prefilled https://
          const std::string urlToSave = (url == "https://" || url == "http://") ? "" : url;
          KOREADER_STORE.setServerUrl(urlToSave);
          KOREADER_STORE.saveToFile();
          exitActivity();
          updateRequired = true;
        },
        [this]() {
          exitActivity();
          updateRequired = true;
        }));
  } else if (selectedIndex == 3) {
    // Document Matching - toggle between Filename and Binary
    const auto current = KOREADER_STORE.getMatchMethod();
    const auto newMethod =
        (current == DocumentMatchMethod::FILENAME) ? DocumentMatchMethod::BINARY : DocumentMatchMethod::FILENAME;
    KOREADER_STORE.setMatchMethod(newMethod);
    KOREADER_STORE.saveToFile();
    updateRequired = true;
  } else if (selectedIndex == 4) {
    // Authenticate
    if (!KOREADER_STORE.hasCredentials()) {
      // Can't authenticate without credentials - just show message briefly
      xSemaphoreGive(renderingMutex);
      return;
    }
    exitActivity();
    enterNewActivity(new KOReaderAuthActivity(renderer, mappedInput, [this] {
      exitActivity();
      updateRequired = true;
    }));
  }

  xSemaphoreGive(renderingMutex);
}

void KOReaderSettingsActivity::displayTaskLoop() {
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

void KOReaderSettingsActivity::render() {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();

  // Draw header
  renderer.drawCenteredText(UI_12_FONT_ID, 15, TR(KOREADER_SYNC), true, EpdFontFamily::BOLD);

  // Draw selection highlight
  renderer.fillRect(0, 60 + selectedIndex * 30 - 2, pageWidth - 1, 30);

  // Draw menu items
  const char* menuNames[MENU_ITEMS] = {TR(USERNAME), TR(PASSWORD), TR(SYNC_SERVER_URL), TR(DOCUMENT_MATCHING),
                                       TR(AUTHENTICATE)};
  for (int i = 0; i < MENU_ITEMS; i++) {
    const int settingY = 60 + i * 30;
    const bool isSelected = (i == selectedIndex);

    renderer.drawText(UI_10_FONT_ID, 20, settingY, menuNames[i], !isSelected);

    // Draw status for each item
    std::string statusStr;
    if (i == 0) {
      statusStr = KOREADER_STORE.getUsername().empty() ? std::string("[") + TR(NOT_SET) + "]"
                                                       : std::string("[") + TR(SET) + "]";
    } else if (i == 1) {
      statusStr = KOREADER_STORE.getPassword().empty() ? std::string("[") + TR(NOT_SET) + "]"
                                                       : std::string("[") + TR(SET) + "]";
    } else if (i == 2) {
      statusStr = KOREADER_STORE.getServerUrl().empty() ? "[Default]" : "[Custom]";
    } else if (i == 3) {
      statusStr = KOREADER_STORE.getMatchMethod() == DocumentMatchMethod::FILENAME
                      ? std::string("[") + TR(FILENAME) + "]"
                      : std::string("[") + TR(BINARY) + "]";
    } else if (i == 4) {
      statusStr = KOREADER_STORE.hasCredentials() ? "" : std::string("[") + TR(SET_CREDENTIALS_FIRST) + "]";
    }

    const auto width = renderer.getTextWidth(UI_10_FONT_ID, statusStr.c_str());
    renderer.drawText(UI_10_FONT_ID, pageWidth - 20 - width, settingY, statusStr.c_str(), !isSelected);
  }

  // Draw button hints
  const auto labels = mappedInput.mapLabels(TR(BACK), TR(SELECT), "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

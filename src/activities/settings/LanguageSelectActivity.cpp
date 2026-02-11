#include "LanguageSelectActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

void LanguageSelectActivity::taskTrampoline(void *param) {
  auto *self = static_cast<LanguageSelectActivity *>(param);
  self->displayTaskLoop();
}

void LanguageSelectActivity::onEnter() {
  ActivityWithSubactivity::onEnter();

  renderingMutex = xSemaphoreCreateMutex();

  // Set current selection based on current language
  selectedIndex = static_cast<int>(I18N.getLanguage());

  updateRequired = false;  // Don't trigger render immediately to avoid race with parent activity

  xTaskCreate(&LanguageSelectActivity::taskTrampoline, "LanguageSelectTask",
              4096, this, 1, &displayTaskHandle);
}

void LanguageSelectActivity::onExit() {
  ActivityWithSubactivity::onExit();

  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;
}

void LanguageSelectActivity::loop() {
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

  if (mappedInput.wasPressed(MappedInputManager::Button::Up) ||
      mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    selectedIndex = (selectedIndex + totalItems - 1) % totalItems;
    updateRequired = true;
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Down) ||
             mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    selectedIndex = (selectedIndex + 1) % totalItems;
    updateRequired = true;
  }
}

void LanguageSelectActivity::handleSelection() {
  xSemaphoreTake(renderingMutex, portMAX_DELAY);

  // Set the selected language (setLanguage internally calls saveSettings)
  I18N.setLanguage(static_cast<Language>(selectedIndex));

  xSemaphoreGive(renderingMutex);

  // Return to previous page
  onBack();
}

void LanguageSelectActivity::displayTaskLoop() {
  // Wait for parent activity's rendering to complete (screen refresh takes ~422ms)
  // Wait 500ms to be safe and avoid race conditions with parent activity
  vTaskDelay(500 / portTICK_PERIOD_MS);
  updateRequired = true;

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

void LanguageSelectActivity::render() {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  auto metrics = UITheme::getInstance().getMetrics();

  // Title
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, TR(LANGUAGE));

  // Current language marker
  const int currentLang = static_cast<int>(I18N.getLanguage());

  // Language names in their native language
  const char *langNames[] = {
    "English",
    "\xE7\xAE\x80\xE4\xBD\x93\xE4\xB8\xAD\xE6\x96\x87",  // 简体中文
    "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E"               // 日本語
  };

  // Language list
  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  GUI.drawList(
      renderer, Rect{0, contentTop, pageWidth, contentHeight}, totalItems, selectedIndex,
      [&langNames](int i) -> std::string {
        return std::string(langNames[i]);
      },
      nullptr, nullptr,
      [currentLang](int i) -> std::string {
        return (i == currentLang) ? std::string(TR(ON)) : std::string("");
      });

  // Button hints
  const auto labels = mappedInput.mapLabels(TR(BACK), TR(SELECT), TR(DIR_UP), TR(DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

#include "FontSelectActivity.h"

#include <FontManager.h>
#include <GfxRenderer.h>

#include "MappedInputManager.h"
#include "fontIds.h"

void FontSelectActivity::taskTrampoline(void *param) {
  auto *self = static_cast<FontSelectActivity *>(param);
  self->displayTaskLoop();
}

void FontSelectActivity::onEnter() {
  ActivityWithSubactivity::onEnter();

  renderingMutex = xSemaphoreCreateMutex();

  // Scan fonts
  FontMgr.scanFonts();

  // Total items = 1 (Built-in) + external font count
  totalItems = 1 + FontMgr.getFontCount();

  // Set current selection based on mode
  int currentFont = (mode == SelectMode::Reader) ? FontMgr.getSelectedIndex()
                                                 : FontMgr.getUiSelectedIndex();
  if (currentFont < 0) {
    selectedIndex = 0; // Built-in
  } else {
    selectedIndex = currentFont + 1; // External font index + 1
  }

  updateRequired = true;

  xTaskCreate(&FontSelectActivity::taskTrampoline, "FontSelectTask", 4096, this,
              1, &displayTaskHandle);
}

void FontSelectActivity::onExit() {
  ActivityWithSubactivity::onExit();

  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;
}

void FontSelectActivity::loop() {
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

void FontSelectActivity::handleSelection() {
  xSemaphoreTake(renderingMutex, portMAX_DELAY);

  if (selectedIndex == 0) {
    // Select Built-in (disable external font)
    if (mode == SelectMode::Reader)
      FontMgr.selectFont(-1);
    else
      FontMgr.selectUiFont(-1);
  } else {
    // Select external font
    if (mode == SelectMode::Reader)
      FontMgr.selectFont(selectedIndex - 1);
    else
      FontMgr.selectUiFont(selectedIndex - 1);
  }

  xSemaphoreGive(renderingMutex);

  // Return to previous page
  onBack();
}

void FontSelectActivity::displayTaskLoop() {
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

void FontSelectActivity::render() {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  // Calculate row height based on current UI font size
  const int rowHeight = 20 + renderer.getUiFontSize() * 2 +
                        10; // 30px/32px/34px for small/medium/large

  // Title
  const char *title = (mode == SelectMode::Reader) ? "Reader External Font"
                                                   : "UI External Font";
  renderer.drawCenteredText(UI_12_FONT_ID, 15, title, true,
                            EpdFontFamily::BOLD);

  // Current selected font marker
  int currentFont = (mode == SelectMode::Reader) ? FontMgr.getSelectedIndex()
                                                 : FontMgr.getUiSelectedIndex();

  // Draw options
  for (int i = 0; i < totalItems && i < 20; i++) { // Max 20 items
    const int itemY = 60 + i * rowHeight;
    const bool isSelected = (i == selectedIndex);
    const bool isCurrent =
        (i == 0 && currentFont < 0) || (i > 0 && i - 1 == currentFont);

    // Draw selection highlight
    if (isSelected) {
      renderer.fillRect(0, itemY - 2, pageWidth - 1, rowHeight);
    }

    // Draw text
    if (i == 0) {
      // Built-in option
      renderer.drawText(UI_10_FONT_ID, 20, itemY, "Built-in (Disabled)",
                        !isSelected);
    } else {
      // External font
      const FontInfo *info = FontMgr.getFontInfo(i - 1);
      if (info) {
        char label[64];
        snprintf(label, sizeof(label), "%s (%dpt)", info->name, info->size);
        renderer.drawText(UI_10_FONT_ID, 20, itemY, label, !isSelected);
      }
    }

    // Draw current selection marker
    if (isCurrent) {
      const auto width = renderer.getTextWidth(UI_10_FONT_ID, "[ON]");
      renderer.drawText(UI_10_FONT_ID, pageWidth - 20 - width, itemY, "[ON]",
                        !isSelected);
    }
  }

  // Button hints
  const auto labels = mappedInput.mapLabels("< Back", "Select", "", "");
  renderer.drawButtonHints(UI_10_FONT_ID, labels.btn1, labels.btn2, labels.btn3,
                           labels.btn4);

  renderer.displayBuffer();
}

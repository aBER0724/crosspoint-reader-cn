#include "FontSelectActivity.h"

#include <FontManager.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int kBuiltinReaderFontCount = 3;
constexpr CrossPointSettings::FONT_FAMILY kBuiltinReaderFonts[kBuiltinReaderFontCount] = {
    CrossPointSettings::BOOKERLY, CrossPointSettings::NOTOSANS, CrossPointSettings::OPENDYSLEXIC};
constexpr StrId kBuiltinReaderFontLabels[kBuiltinReaderFontCount] = {StrId::BOOKERLY, StrId::NOTO_SANS,
                                                                     StrId::OPEN_DYSLEXIC};
}  // namespace

void FontSelectActivity::onEnter() {
  ActivityWithSubactivity::onEnter();

  // Wait for parent activity's rendering to complete (screen refresh takes ~422ms)
  // Wait 500ms to be safe and avoid race conditions with parent activity
  vTaskDelay(500 / portTICK_PERIOD_MS);

  // Scan fonts
  FontMgr.scanFonts();

  if (mode == SelectMode::Reader) {
    totalItems = kBuiltinReaderFontCount + FontMgr.getFontCount();

    const int currentExternal = FontMgr.getSelectedIndex();
    if (currentExternal >= 0) {
      selectedIndex = kBuiltinReaderFontCount + currentExternal;
    } else {
      const int familyIndex = static_cast<int>(SETTINGS.fontFamily);
      selectedIndex = (familyIndex < kBuiltinReaderFontCount) ? familyIndex : 0;
    }
  } else {
    // Built-in UI font + external fonts
    totalItems = 1 + FontMgr.getFontCount();

    const int currentFont = FontMgr.getUiSelectedIndex();
    selectedIndex = (currentFont < 0) ? 0 : currentFont + 1;
  }

  // 同步渲染，不使用后台任务
  render();
}

void FontSelectActivity::onExit() {
  ActivityWithSubactivity::onExit();
  // 不需要清理任务和 mutex，因为我们不再使用它们
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

  bool needsRender = false;

  if (mappedInput.wasPressed(MappedInputManager::Button::Up) ||
      mappedInput.wasPressed(MappedInputManager::Button::Left)) {
    selectedIndex = (selectedIndex + totalItems - 1) % totalItems;
    needsRender = true;
  } else if (mappedInput.wasPressed(MappedInputManager::Button::Down) ||
             mappedInput.wasPressed(MappedInputManager::Button::Right)) {
    selectedIndex = (selectedIndex + 1) % totalItems;
    needsRender = true;
  }

  // 同步渲染
  if (needsRender) {
    render();
  }
}

void FontSelectActivity::handleSelection() {
  Serial.printf("[FONT_SELECT] handleSelection: mode=%d, selectedIndex=%d\n", static_cast<int>(mode), selectedIndex);

  if (mode == SelectMode::Reader) {
    if (selectedIndex < kBuiltinReaderFontCount) {
      // Select built-in reader font
      Serial.printf("[FONT_SELECT] Selecting built-in reader font index %d\n", selectedIndex);
      FontMgr.selectFont(-1);
      SETTINGS.fontFamily = static_cast<uint8_t>(kBuiltinReaderFonts[selectedIndex]);
      SETTINGS.saveToFile();
    } else {
      // Select external reader font
      const int externalIndex = selectedIndex - kBuiltinReaderFontCount;
      Serial.printf("[FONT_SELECT] Selecting reader font index %d\n", externalIndex);
      FontMgr.selectFont(externalIndex);
    }
    renderer.setReaderFallbackFontId(SETTINGS.getBuiltInReaderFontId());
  } else {
    if (selectedIndex == 0) {
      // Select built-in UI font (disable external font)
      Serial.printf("[FONT_SELECT] Disabling UI font\n");
      FontMgr.selectUiFont(-1);
    } else {
      // Select external UI font
      const int externalIndex = selectedIndex - 1;
      Serial.printf("[FONT_SELECT] Selecting UI font index %d\n", externalIndex);
      FontMgr.selectUiFont(externalIndex);
    }
  }

  Serial.printf("[FONT_SELECT] After selection: readerIndex=%d, uiIndex=%d\n", FontMgr.getSelectedIndex(),
                FontMgr.getUiSelectedIndex());

  // Return to previous page
  onBack();
}

void FontSelectActivity::render() {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  auto metrics = UITheme::getInstance().getMetrics();

  // Title
  const char* title = (mode == SelectMode::Reader) ? TR(EXT_READER_FONT) : TR(EXT_UI_FONT);
  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, title);

  // Current active font index (for the ON marker)
  int currentIndex = 0;
  if (mode == SelectMode::Reader) {
    const int currentExternal = FontMgr.getSelectedIndex();
    if (currentExternal >= 0) {
      currentIndex = kBuiltinReaderFontCount + currentExternal;
    } else {
      const int familyIndex = static_cast<int>(SETTINGS.fontFamily);
      currentIndex = (familyIndex < kBuiltinReaderFontCount) ? familyIndex : 0;
    }
  } else {
    const int currentFont = FontMgr.getUiSelectedIndex();
    currentIndex = (currentFont < 0) ? 0 : currentFont + 1;
  }

  // Font list
  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  GUI.drawList(
      renderer, Rect{0, contentTop, pageWidth, contentHeight}, totalItems, selectedIndex,
      [this](int i) -> std::string {
        if (mode == SelectMode::Reader) {
          if (i < kBuiltinReaderFontCount) {
            return std::string(I18N.get(kBuiltinReaderFontLabels[i]));
          }
          const FontInfo* info = FontMgr.getFontInfo(i - kBuiltinReaderFontCount);
          if (info) {
            char label[64];
            snprintf(label, sizeof(label), "%s (%dpt)", info->name, info->size);
            return std::string(label);
          }
        } else {
          if (i == 0) {
            return std::string(TR(BUILTIN_DISABLED));
          }
          const FontInfo* info = FontMgr.getFontInfo(i - 1);
          if (info) {
            char label[64];
            snprintf(label, sizeof(label), "%s (%dpt)", info->name, info->size);
            return std::string(label);
          }
        }
        return "";
      },
      nullptr, nullptr,
      [currentIndex](int i) -> std::string { return (i == currentIndex) ? std::string(TR(ON)) : std::string(""); });

  // Button hints
  const auto labels = mappedInput.mapLabels(TR(BACK), TR(SELECT), TR(DIR_UP), TR(DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

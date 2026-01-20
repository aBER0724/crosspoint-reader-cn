#include "EpubReaderChapterSelectionActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Utf8.h>

#include <vector>

#include "MappedInputManager.h"
#include "fontIds.h"
#include "util/OrientationUtils.h"

namespace {
// Time threshold for treating a long press as a page-up/page-down
constexpr int SKIP_PAGE_MS = 700;
constexpr uint32_t CP_CHAPTER_PREFIX = 0x7B2C;
constexpr uint32_t CP_CHAPTER_MARK = 0x7AE0;
constexpr uint32_t CP_SPACE = 0x20;
constexpr uint32_t CP_IDEOGRAPHIC_SPACE = 0x3000;

// Calculate row height based on current UI font size
// Font sizes: SMALL=20px, MEDIUM=22px, LARGE=24px
// Row height = font height + spacing (8-12px)
inline int getRowHeight(const GfxRenderer& renderer) {
  return 20 + renderer.getUiFontSize() * 2 + 10;  // 30px/32px/34px for small/medium/large
}

void appendUtf8(std::string& out, const uint32_t cp) {
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp <= 0x10FFFF) {
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

std::string addSpaceAfterChapterMarker(const std::string& title) {
  const unsigned char* ptr = reinterpret_cast<const unsigned char*>(title.c_str());
  std::vector<uint32_t> codepoints;
  uint32_t cp;

  while ((cp = utf8NextCodepoint(&ptr))) {
    codepoints.push_back(cp);
  }

  if (codepoints.size() < 3 || codepoints[0] != CP_CHAPTER_PREFIX) {
    return title;
  }

  size_t chapterIndex = codepoints.size();
  for (size_t i = 1; i < codepoints.size(); ++i) {
    if (codepoints[i] == CP_CHAPTER_MARK) {
      chapterIndex = i;
      break;
    }
  }

  if (chapterIndex == codepoints.size() || chapterIndex + 1 >= codepoints.size()) {
    return title;
  }

  const uint32_t nextCp = codepoints[chapterIndex + 1];
  if (nextCp == CP_SPACE || nextCp == CP_IDEOGRAPHIC_SPACE) {
    return title;
  }

  std::string result;
  result.reserve(title.size() + 1);
  for (size_t i = 0; i < codepoints.size(); ++i) {
    appendUtf8(result, codepoints[i]);
    if (i == chapterIndex) {
      result.push_back(' ');
    }
  }
  return result;
}
}  // namespace

int EpubReaderChapterSelectionActivity::getPageItems() const {
  // Layout constants used in renderScreen
  const int startY = getUiTopInset(renderer) + 60;
  const int lineHeight = getRowHeight(renderer);

  const int screenHeight = renderer.getScreenHeight();
  const int endY = screenHeight - lineHeight;

  const int availableHeight = endY - startY;
  int items = availableHeight / lineHeight;

  // Ensure we always have at least one item per page to avoid division by zero
  if (items < 1) {
    items = 1;
  }
  return items;
}

void EpubReaderChapterSelectionActivity::taskTrampoline(void* param) {
  auto* self = static_cast<EpubReaderChapterSelectionActivity*>(param);
  self->displayTaskLoop();
}

void EpubReaderChapterSelectionActivity::onEnter() {
  Activity::onEnter();

  if (!epub) {
    return;
  }

  renderingMutex = xSemaphoreCreateMutex();
  selectorIndex = epub->getTocIndexForSpineIndex(currentSpineIndex);
  if (selectorIndex == -1) {
    selectorIndex = 0;
  }

  // Trigger first update
  updateRequired = true;
  xTaskCreate(&EpubReaderChapterSelectionActivity::taskTrampoline, "EpubReaderChapterSelectionActivityTask",
              4096,               // Stack size
              this,               // Parameters
              1,                  // Priority
              &displayTaskHandle  // Task handle
  );
}

void EpubReaderChapterSelectionActivity::onExit() {
  Activity::onExit();

  // Wait until not rendering to delete task to avoid killing mid-instruction to EPD
  xSemaphoreTake(renderingMutex, portMAX_DELAY);
  if (displayTaskHandle) {
    vTaskDelete(displayTaskHandle);
    displayTaskHandle = nullptr;
  }
  vSemaphoreDelete(renderingMutex);
  renderingMutex = nullptr;
}

void EpubReaderChapterSelectionActivity::loop() {
  const bool prevReleased = mappedInput.wasReleased(MappedInputManager::Button::Up) ||
                            mappedInput.wasReleased(MappedInputManager::Button::Left);
  const bool nextReleased = mappedInput.wasReleased(MappedInputManager::Button::Down) ||
                            mappedInput.wasReleased(MappedInputManager::Button::Right);

  const bool skipPage = mappedInput.getHeldTime() > SKIP_PAGE_MS;
  const int pageItems = getPageItems();

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    const auto newSpineIndex = epub->getSpineIndexForTocIndex(selectorIndex);
    if (newSpineIndex == -1) {
      onGoBack();
    } else {
      onSelectSpineIndex(newSpineIndex);
    }
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoBack();
  } else if (prevReleased) {
    if (skipPage) {
      selectorIndex =
          ((selectorIndex / pageItems - 1) * pageItems + epub->getTocItemsCount()) % epub->getTocItemsCount();
    } else {
      selectorIndex = (selectorIndex + epub->getTocItemsCount() - 1) % epub->getTocItemsCount();
    }
    updateRequired = true;
  } else if (nextReleased) {
    if (skipPage) {
      selectorIndex = ((selectorIndex / pageItems + 1) * pageItems) % epub->getTocItemsCount();
    } else {
      selectorIndex = (selectorIndex + 1) % epub->getTocItemsCount();
    }
    updateRequired = true;
  }
}

void EpubReaderChapterSelectionActivity::displayTaskLoop() {
  while (true) {
    if (updateRequired) {
      updateRequired = false;
      xSemaphoreTake(renderingMutex, portMAX_DELAY);
      renderScreen();
      xSemaphoreGive(renderingMutex);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void EpubReaderChapterSelectionActivity::renderScreen() {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const int leftInset = getUiLeftInset(renderer);
  const int rightInset = getUiRightInset(renderer);
  const int contentWidth = pageWidth - leftInset - rightInset;
  const int topInset = getUiTopInset(renderer);
  const int pageItems = getPageItems();
  const int rowHeight = getRowHeight(renderer);

  const std::string title =
      renderer.truncatedText(UI_12_FONT_ID, epub->getTitle().c_str(), contentWidth - 40, EpdFontFamily::BOLD);
  const int titleWidth =
      renderer.getTextWidth(UI_12_FONT_ID, title.c_str(), EpdFontFamily::BOLD);
  const int titleX = leftInset + (contentWidth - titleWidth) / 2;
  renderer.drawText(UI_12_FONT_ID, titleX, topInset + 15, title.c_str(), true,
                    EpdFontFamily::BOLD);

  const auto pageStartIndex = selectorIndex / pageItems * pageItems;
  const int listStartY = topInset + 60;
  renderer.fillRect(leftInset, listStartY + (selectorIndex % pageItems) * rowHeight - 2,
                    contentWidth - 1, rowHeight);
  for (int tocIndex = pageStartIndex; tocIndex < epub->getTocItemsCount() && tocIndex < pageStartIndex + pageItems;
       tocIndex++) {
    auto item = epub->getTocItem(tocIndex);
    const int xPos = leftInset + 20 + (item.level - 1) * 15;
    const int yPos = listStartY + (tocIndex % pageItems) * rowHeight;
    // Calculate max width for title to prevent overlap
    const int maxWidth = contentWidth - (xPos - leftInset) - 10;
    // Truncate title if too long to prevent overlap with screen edge
    const std::string spacedTitle = addSpaceAfterChapterMarker(item.title);
    const std::string truncatedTitle = renderer.truncatedText(UI_10_FONT_ID, spacedTitle.c_str(), maxWidth);
    renderer.drawText(UI_10_FONT_ID, xPos, yPos, truncatedTitle.c_str(),
                      tocIndex != selectorIndex);
  }

  const auto labels = mappedInput.mapLabels(TR(BACK), TR(SELECT), "", "");
  renderer.drawButtonHints(UI_10_FONT_ID, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

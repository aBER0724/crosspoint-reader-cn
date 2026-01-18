#include "SleepActivity.h"
#include <cstring>

#include <Epub.h>
#include <GfxRenderer.h>
#include <I18n.h>
#include <SDCardManager.h>
#include <Txt.h>
#include <Xtc.h>

#include "CrossPointSettings.h"
#include "CrossPointState.h"
#include "fontIds.h"
#include "images/CrossLarge.h"
#include "util/StringUtils.h"

void SleepActivity::onEnter() {
  Activity::onEnter();
  renderPopup("Entering Sleep...");

  if (SETTINGS.sleepScreen == CrossPointSettings::SLEEP_SCREEN_MODE::BLANK) {
    return renderBlankSleepScreen();
  }

  if (SETTINGS.sleepScreen == CrossPointSettings::SLEEP_SCREEN_MODE::CUSTOM) {
    return renderCustomSleepScreen();
  }

  if (SETTINGS.sleepScreen == CrossPointSettings::SLEEP_SCREEN_MODE::COVER) {
    return renderCoverSleepScreen();
  }

  renderDefaultSleepScreen();
}

void SleepActivity::renderPopup(const char *message) const {
  const int textWidth =
      renderer.getTextWidth(UI_12_FONT_ID, message, EpdFontFamily::BOLD);
  constexpr int margin = 20;
  const int x = (renderer.getScreenWidth() - textWidth - margin * 2) / 2;
  constexpr int y = 117;
  const int w = textWidth + margin * 2;
  const int h = renderer.getLineHeight(UI_12_FONT_ID) + margin * 2;
  // renderer.clearScreen();
  renderer.fillRect(x - 5, y - 5, w + 10, h + 10, true);
  renderer.fillRect(x + 5, y + 5, w - 10, h - 10, false);
  renderer.drawText(UI_12_FONT_ID, x + margin, y + margin, message, true,
                    EpdFontFamily::BOLD);
  renderer.displayBuffer();
}

void SleepActivity::renderCustomSleepScreen() const {
  FsFile file;

  // 1. Try specifically set wallpaper path first
  if (strlen(SETTINGS.sleepImagePath) > 0) {
    if (SdMan.openFileForRead("SLP", SETTINGS.sleepImagePath, file)) {
      Serial.printf("[%lu] [SLP] Loading set wallpaper: %s\n", millis(),
                    SETTINGS.sleepImagePath);
      if (StringUtils::checkFileExtension(SETTINGS.sleepImagePath, ".bmp")) {
        Bitmap bitmap(file, true);
        if (bitmap.parseHeaders() == BmpReaderError::Ok) {
          renderBitmapSleepScreen(bitmap);
          return;
        }
      } else {
        renderXtgSleepScreen(file);
        return;
      }
    }
  }

  // 2. Try random file from /sleep directory
  auto dir = SdMan.open("/sleep");
  if (dir && dir.isDirectory()) {
    std::vector<std::string> files;
    char name[500];
    // collect all valid BMP files
    for (auto file = dir.openNextFile(); file; file = dir.openNextFile()) {
      if (file.isDirectory()) {
        file.close();
        continue;
      }
      file.getName(name, sizeof(name));
      auto filename = std::string(name);
      if (filename[0] == '.') {
        file.close();
        continue;
      }

      bool isBmp = StringUtils::checkFileExtension(filename, ".bmp");
      bool isXtg = StringUtils::checkFileExtension(filename, ".xtg");
      bool isXth = StringUtils::checkFileExtension(filename, ".xth");

      if (!isBmp && !isXtg && !isXth) {
        Serial.printf("[%lu] [SLP] Skipping unsupported file: %s\n", millis(),
                      name);
        file.close();
        continue;
      }

      if (isBmp) {
        Bitmap bitmap(file);
        if (bitmap.parseHeaders() != BmpReaderError::Ok) {
          Serial.printf("[%lu] [SLP] Skipping invalid BMP file: %s\n", millis(),
                        name);
          file.close();
          continue;
        }
      }
      // For XTG/XTH we just verify it exists and name is clean

      files.emplace_back(filename);
      file.close();
    }
    const auto numFiles = files.size();
    if (numFiles > 0) {
      // Generate a random number between 1 and numFiles
      auto randomFileIndex = random(numFiles);
      // If we picked the same image as last time, reroll
      while (numFiles > 1 && randomFileIndex == APP_STATE.lastSleepImage) {
        randomFileIndex = random(numFiles);
      }
      APP_STATE.lastSleepImage = randomFileIndex;
      APP_STATE.saveToFile();
      const auto filename = "/sleep/" + files[randomFileIndex];
      FsFile file;
      if (SdMan.openFileForRead("SLP", filename, file)) {
        Serial.printf("[%lu] [SLP] Randomly loading: /sleep/%s\n", millis(),
                      files[randomFileIndex].c_str());
        delay(100);
        if (StringUtils::checkFileExtension(filename, ".bmp")) {
          Bitmap bitmap(file, true);
          if (bitmap.parseHeaders() == BmpReaderError::Ok) {
            renderBitmapSleepScreen(bitmap);
            dir.close();
            return;
          }
        } else {
          // XTG or XTH
          renderXtgSleepScreen(file);
          dir.close();
          return;
        }
      }
    }
  }
  if (dir)
    dir.close();

  // Look for sleep.bmp on the root of the sd card to determine if we should
  // render a custom sleep screen instead of the default.
  if (SdMan.openFileForRead("SLP", "/sleep.bmp", file)) {
    Bitmap bitmap(file, true);
    if (bitmap.parseHeaders() == BmpReaderError::Ok) {
      Serial.printf("[%lu] [SLP] Loading: /sleep.bmp\n", millis());
      renderBitmapSleepScreen(bitmap);
      return;
    }
  }

  // Look for sleep.xtg or sleep.xth on root
  if (SdMan.openFileForRead("SLP", "/sleep.xtg", file)) {
    Serial.printf("[%lu] [SLP] Loading: /sleep.xtg\n", millis());
    renderXtgSleepScreen(file);
    return;
  }
  if (SdMan.openFileForRead("SLP", "/sleep.xth", file)) {
    Serial.printf("[%lu] [SLP] Loading: /sleep.xth\n", millis());
    renderXtgSleepScreen(file);
    return;
  }

  renderDefaultSleepScreen();
}

void SleepActivity::renderDefaultSleepScreen() const {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  renderer.drawImage(CrossLarge, (pageWidth + 128) / 2, (pageHeight - 128) / 2,
                     128, 128);
  renderer.drawCenteredText(UI_10_FONT_ID, pageHeight / 2 + 70, TR(CROSSPOINT),
                            true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight / 2 + 95, TR(SLEEPING));

  // Ensure sleep screen matches its dedicated setting regardless of global dark
  // mode
  bool desiredDark =
      (SETTINGS.sleepScreen == CrossPointSettings::SLEEP_SCREEN_MODE::DARK);
  if (desiredDark != renderer.isDarkMode()) {
    renderer.invertScreen();
  }

  renderer.displayBuffer(EInkDisplay::HALF_REFRESH);
}

void SleepActivity::renderBitmapSleepScreen(const Bitmap &bitmap) const {
  int x, y;
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  float cropX = 0, cropY = 0;

  Serial.printf("[%lu] [SLP] bitmap %d x %d, screen %d x %d\n", millis(),
                bitmap.getWidth(), bitmap.getHeight(), pageWidth, pageHeight);
  if (bitmap.getWidth() > pageWidth || bitmap.getHeight() > pageHeight) {
    // image will scale, make sure placement is right
    float ratio = static_cast<float>(bitmap.getWidth()) /
                  static_cast<float>(bitmap.getHeight());
    const float screenRatio =
        static_cast<float>(pageWidth) / static_cast<float>(pageHeight);

    Serial.printf("[%lu] [SLP] bitmap ratio: %f, screen ratio: %f\n", millis(),
                  ratio, screenRatio);
    if (ratio > screenRatio) {
      // image wider than viewport ratio, scaled down image needs to be centered
      // vertically
      if (SETTINGS.sleepScreenCoverMode ==
          CrossPointSettings::SLEEP_SCREEN_COVER_MODE::CROP) {
        cropX = 1.0f - (screenRatio / ratio);
        Serial.printf("[%lu] [SLP] Cropping bitmap x: %f\n", millis(), cropX);
        ratio = (1.0f - cropX) * static_cast<float>(bitmap.getWidth()) /
                static_cast<float>(bitmap.getHeight());
      }
      x = 0;
      y = std::round((static_cast<float>(pageHeight) -
                      static_cast<float>(pageWidth) / ratio) /
                     2);
      Serial.printf("[%lu] [SLP] Centering with ratio %f to y=%d\n", millis(),
                    ratio, y);
    } else {
      // image taller than viewport ratio, scaled down image needs to be
      // centered horizontally
      if (SETTINGS.sleepScreenCoverMode ==
          CrossPointSettings::SLEEP_SCREEN_COVER_MODE::CROP) {
        cropY = 1.0f - (ratio / screenRatio);
        Serial.printf("[%lu] [SLP] Cropping bitmap y: %f\n", millis(), cropY);
        ratio = static_cast<float>(bitmap.getWidth()) /
                ((1.0f - cropY) * static_cast<float>(bitmap.getHeight()));
      }
      x = std::round((pageWidth - pageHeight * ratio) / 2);
      y = 0;
      Serial.printf("[%lu] [SLP] Centering with ratio %f to x=%d\n", millis(),
                    ratio, x);
    }
  } else {
    // center the image
    x = (pageWidth - bitmap.getWidth()) / 2;
    y = (pageHeight - bitmap.getHeight()) / 2;
  }

  Serial.printf("[%lu] [SLP] drawing to %d x %d\n", millis(), x, y);
  renderer.clearScreen();
  renderer.drawBitmap(bitmap, x, y, pageWidth, pageHeight, cropX, cropY);
  renderer.displayBuffer(EInkDisplay::HALF_REFRESH);

  if (bitmap.hasGreyscale()) {
    bitmap.rewindToData();
    memset(renderer.getFrameBuffer(), 0x00, GfxRenderer::getBufferSize());
    renderer.setRenderMode(GfxRenderer::GRAYSCALE_LSB);
    renderer.drawBitmap(bitmap, x, y, pageWidth, pageHeight, cropX, cropY);
    renderer.copyGrayscaleLsbBuffers();

    bitmap.rewindToData();
    memset(renderer.getFrameBuffer(), 0x00, GfxRenderer::getBufferSize());
    renderer.setRenderMode(GfxRenderer::GRAYSCALE_MSB);
    renderer.drawBitmap(bitmap, x, y, pageWidth, pageHeight, cropX, cropY);
    renderer.copyGrayscaleMsbBuffers();

    renderer.displayGrayBuffer();
    renderer.setRenderMode(GfxRenderer::BW);
  }
}

void SleepActivity::renderXtgSleepScreen(FsFile &file) const {
  // Use Portrait orientation to ensure correct 480x800 buffer mapping
  const auto originalOrientation = renderer.getOrientation();
  renderer.setOrientation(GfxRenderer::Portrait);

  xtc::XtgPageHeader header;
  const size_t fileSize = file.size();
  bool hasHeader = false;
  size_t headerSize = 0;

  // 1. Attempt to read standard header
  if (file.read(&header, sizeof(header)) == sizeof(header)) {
    if (header.magic == xtc::XTG_MAGIC || header.magic == xtc::XTH_MAGIC) {
      hasHeader = true;
      headerSize = sizeof(header);
    }
  }

  uint16_t w, h;
  size_t dataSize;
  bool is2Bit = false;

  if (hasHeader) {
    w = header.width;
    h = header.height;
    dataSize = header.dataSize;
    is2Bit = (header.magic == xtc::XTH_MAGIC);
  } else {
    // 2. Fallback: Check for raw file with potential 22/56-byte header
    w = 480;
    h = 800;
    dataSize = (static_cast<size_t>(w) * h + 7) / 8; // 48000

    if (fileSize > dataSize && (fileSize - dataSize < 100)) {
      headerSize = fileSize - dataSize; // Likely 22 or 56
    } else {
      headerSize = 0;
    }
    file.seek(headerSize);
    is2Bit = false;
  }

  uint8_t *buffer = static_cast<uint8_t *>(malloc(dataSize));
  if (!buffer) {
    file.close();
    renderer.setOrientation(originalOrientation);
    renderDefaultSleepScreen();
    return;
  }

  // Load exactly dataSize bytes to prevent memory corruption
  file.read(buffer, dataSize);
  file.close();

  renderer.clearScreen();
  const int offsetX = (renderer.getScreenWidth() - w) / 2;
  const int offsetY = (renderer.getScreenHeight() - h) / 2;

  if (is2Bit) {
    // 2-bit XTH mode
    const size_t planeSize = (static_cast<size_t>(w) * h + 7) / 8;
    const uint8_t *plane1 = buffer;
    const uint8_t *plane2 = buffer + planeSize;
    const size_t colBytes = (h + 7) / 8;

    auto getPixel = [&](uint16_t px, uint16_t py) -> uint8_t {
      const size_t colIndex = w - 1 - px;
      const size_t byteInCol = py / 8;
      const size_t bitInByte = 7 - (py % 8);
      const size_t byteOffset = colIndex * colBytes + byteInCol;
      if (byteOffset + planeSize >= dataSize)
        return 0;
      const uint8_t b1 = (plane1[byteOffset] >> bitInByte) & 1;
      const uint8_t b2 = (plane2[byteOffset] >> bitInByte) & 1;
      return (b1 << 1) | b2;
    };

    for (uint16_t y = 0; y < h; y++) {
      for (uint16_t x = 0; x < w; x++) {
        uint8_t pv = getPixel(x, y);
        bool shouldDraw = false;
        if (renderer.isDarkMode()) {
          if (pv == 3)
            shouldDraw = true; // Only core ink pixels become white
        } else if (pv >= 1) {
          shouldDraw = true; // All non-background pixels are black
        }
        if (shouldDraw)
          renderer.drawPixel(x + offsetX, y + offsetY, true);
      }
    }
    renderer.displayBuffer(EInkDisplay::HALF_REFRESH);

    memset(renderer.getFrameBuffer(), 0x00, GfxRenderer::getBufferSize());
    for (uint16_t y = 0; y < h; y++) {
      for (uint16_t x = 0; x < w; x++) {
        if (getPixel(x, y) == 1)
          renderer.drawPixel(x + offsetX, y + offsetY, false);
      }
    }
    renderer.copyGrayscaleLsbBuffers();

    memset(renderer.getFrameBuffer(), 0x00, GfxRenderer::getBufferSize());
    for (uint16_t y = 0; y < h; y++) {
      for (uint16_t x = 0; x < w; x++) {
        const uint8_t pv = getPixel(x, y);
        if (pv == 1 || pv == 2)
          renderer.drawPixel(x + offsetX, y + offsetY, false);
      }
    }
    renderer.copyGrayscaleMsbBuffers();
    renderer.displayGrayBuffer();
    renderer.setRenderMode(GfxRenderer::BW);
  } else {
    // 1-bit XTG mode
    const size_t rowBytes = (w + 7) / 8;
    for (uint16_t y = 0; y < h; y++) {
      for (uint16_t x = 0; x < w; x++) {
        const size_t byteIdx = y * rowBytes + x / 8;
        if (byteIdx < dataSize) {
          if (!((buffer[byteIdx] >> (7 - (x % 8))) & 1)) {
            renderer.drawPixel(x + offsetX, y + offsetY, true);
          }
        }
      }
    }
    renderer.displayBuffer(EInkDisplay::HALF_REFRESH);
  }

  free(buffer);
  renderer.setOrientation(originalOrientation);
}

void SleepActivity::renderCoverSleepScreen() const {
  if (APP_STATE.openEpubPath.empty()) {
    return renderDefaultSleepScreen();
  }

  std::string coverBmpPath;
  bool cropped = SETTINGS.sleepScreenCoverMode ==
                 CrossPointSettings::SLEEP_SCREEN_COVER_MODE::CROP;

  // Check if the current book is XTC, TXT, or EPUB
  if (StringUtils::checkFileExtension(APP_STATE.openEpubPath, ".xtc") ||
      StringUtils::checkFileExtension(APP_STATE.openEpubPath, ".xtch")) {
    // Handle XTC file
    Xtc lastXtc(APP_STATE.openEpubPath, "/.crosspoint");
    if (!lastXtc.load()) {
      Serial.println("[SLP] Failed to load last XTC");
      return renderDefaultSleepScreen();
    }

    if (!lastXtc.generateCoverBmp()) {
      Serial.println("[SLP] Failed to generate XTC cover bmp");
      return renderDefaultSleepScreen();
    }

    coverBmpPath = lastXtc.getCoverBmpPath();
  } else if (StringUtils::checkFileExtension(APP_STATE.openEpubPath, ".txt")) {
    // Handle TXT file - looks for cover image in the same folder
    Txt lastTxt(APP_STATE.openEpubPath, "/.crosspoint");
    if (!lastTxt.load()) {
      Serial.println("[SLP] Failed to load last TXT");
      return renderDefaultSleepScreen();
    }

    if (!lastTxt.generateCoverBmp()) {
      Serial.println("[SLP] No cover image found for TXT file");
      return renderDefaultSleepScreen();
    }

    coverBmpPath = lastTxt.getCoverBmpPath();
  } else if (StringUtils::checkFileExtension(APP_STATE.openEpubPath, ".epub")) {
    // Handle EPUB file
    Epub lastEpub(APP_STATE.openEpubPath, "/.crosspoint");
    if (!lastEpub.load()) {
      Serial.println("[SLP] Failed to load last epub");
      return renderDefaultSleepScreen();
    }

    if (!lastEpub.generateCoverBmp(cropped)) {
      Serial.println("[SLP] Failed to generate cover bmp");
      return renderDefaultSleepScreen();
    }

    coverBmpPath = lastEpub.getCoverBmpPath(cropped);
  } else {
    return renderDefaultSleepScreen();
  }

  FsFile file;
  if (SdMan.openFileForRead("SLP", coverBmpPath, file)) {
    Bitmap bitmap(file);
    if (bitmap.parseHeaders() == BmpReaderError::Ok) {
      renderBitmapSleepScreen(bitmap);
      return;
    }
  }

  renderDefaultSleepScreen();
}

void SleepActivity::renderBlankSleepScreen() const {
  renderer.clearScreen();
  renderer.displayBuffer(EInkDisplay::HALF_REFRESH);
}

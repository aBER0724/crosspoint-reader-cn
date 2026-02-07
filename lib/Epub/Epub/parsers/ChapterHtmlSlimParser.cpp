#include "ChapterHtmlSlimParser.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <HardwareSerial.h>
#include <SDCardManager.h>
#include <expat.h>

#include "../Page.h"

const char* HEADER_TAGS[] = {"h1", "h2", "h3", "h4", "h5", "h6"};
constexpr int NUM_HEADER_TAGS = sizeof(HEADER_TAGS) / sizeof(HEADER_TAGS[0]);

// Minimum file size (in bytes) to show progress bar - smaller chapters don't benefit from it
constexpr size_t MIN_SIZE_FOR_PROGRESS = 50 * 1024;  // 50KB
constexpr size_t MAX_WORDS_BEFORE_FLUSH = 400;
constexpr size_t MIN_WORDS_BEFORE_FLUSH = 100;
constexpr size_t LOW_FREE_HEAP_BEFORE_FLUSH = 24 * 1024;
constexpr size_t CRITICAL_FREE_HEAP_BEFORE_FLUSH = 12 * 1024;

const char* BLOCK_TAGS[] = {"p", "li", "div", "br", "blockquote"};
constexpr int NUM_BLOCK_TAGS = sizeof(BLOCK_TAGS) / sizeof(BLOCK_TAGS[0]);

const char* BOLD_TAGS[] = {"b", "strong"};
constexpr int NUM_BOLD_TAGS = sizeof(BOLD_TAGS) / sizeof(BOLD_TAGS[0]);

const char* ITALIC_TAGS[] = {"i", "em"};
constexpr int NUM_ITALIC_TAGS = sizeof(ITALIC_TAGS) / sizeof(ITALIC_TAGS[0]);

const char* IMAGE_TAGS[] = {"img"};
constexpr int NUM_IMAGE_TAGS = sizeof(IMAGE_TAGS) / sizeof(IMAGE_TAGS[0]);

const char* SKIP_TAGS[] = {"head"};
constexpr int NUM_SKIP_TAGS = sizeof(SKIP_TAGS) / sizeof(SKIP_TAGS[0]);

bool isWhitespace(const char c) { return c == ' ' || c == '\r' || c == '\n' || c == '\t'; }

// Check if a Unicode codepoint is an invisible/zero-width character that should be skipped
bool isInvisibleCodepoint(const uint32_t cp) {
  if (cp == 0xFEFF) return true;   // BOM / Zero Width No-Break Space
  if (cp == 0x200B) return true;   // Zero Width Space
  if (cp == 0x200C) return true;   // Zero Width Non-Joiner
  if (cp == 0x200D) return true;   // Zero Width Joiner
  if (cp == 0x200E) return true;   // Left-to-Right Mark
  if (cp == 0x200F) return true;   // Right-to-Left Mark
  if (cp == 0x2060) return true;   // Word Joiner
  if (cp == 0x00AD) return true;   // Soft Hyphen
  if (cp == 0x034F) return true;   // Combining Grapheme Joiner
  if (cp == 0x061C) return true;   // Arabic Letter Mark
  if (cp >= 0x2066 && cp <= 0x2069) return true;  // Directional isolates
  if (cp >= 0x202A && cp <= 0x202E) return true;  // Directional formatting
  return false;
}

// Check if a Unicode codepoint is CJK (Chinese/Japanese/Korean)
// CJK characters should be treated as individual "words" for line breaking
bool isCjkCodepointForSplit(const uint32_t cp) {
  // CJK Unified Ideographs: U+4E00 - U+9FFF
  if (cp >= 0x4E00 && cp <= 0x9FFF) return true;
  // CJK Unified Ideographs Extension A: U+3400 - U+4DBF
  if (cp >= 0x3400 && cp <= 0x4DBF) return true;
  // CJK Punctuation: U+3000 - U+303F
  if (cp >= 0x3000 && cp <= 0x303F) return true;
  // Hiragana: U+3040 - U+309F
  if (cp >= 0x3040 && cp <= 0x309F) return true;
  // Katakana: U+30A0 - U+30FF
  if (cp >= 0x30A0 && cp <= 0x30FF) return true;
  // CJK Compatibility Ideographs: U+F900 - U+FAFF
  if (cp >= 0xF900 && cp <= 0xFAFF) return true;
  // Fullwidth forms: U+FF00 - U+FFEF
  if (cp >= 0xFF00 && cp <= 0xFFEF) return true;
  return false;
}

// Get UTF-8 byte length for a lead byte
int getUtf8ByteLength(unsigned char leadByte) {
  if ((leadByte & 0x80) == 0) return 1;       // ASCII: 0xxxxxxx
  if ((leadByte & 0xE0) == 0xC0) return 2;    // 2-byte: 110xxxxx
  if ((leadByte & 0xF0) == 0xE0) return 3;    // 3-byte: 1110xxxx
  if ((leadByte & 0xF8) == 0xF0) return 4;    // 4-byte: 11110xxx
  return 1;  // Invalid, treat as single byte
}

// Decode UTF-8 codepoint from bytes
uint32_t decodeUtf8Codepoint(const char* s, int len) {
  if (len <= 0) return 0;
  unsigned char b0 = static_cast<unsigned char>(s[0]);

  if ((b0 & 0x80) == 0) {
    return b0;  // ASCII
  }
  if (len >= 2 && (b0 & 0xE0) == 0xC0) {
    unsigned char b1 = static_cast<unsigned char>(s[1]);
    return ((b0 & 0x1F) << 6) | (b1 & 0x3F);
  }
  if (len >= 3 && (b0 & 0xF0) == 0xE0) {
    unsigned char b1 = static_cast<unsigned char>(s[1]);
    unsigned char b2 = static_cast<unsigned char>(s[2]);
    return ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
  }
  if (len >= 4 && (b0 & 0xF8) == 0xF0) {
    unsigned char b1 = static_cast<unsigned char>(s[1]);
    unsigned char b2 = static_cast<unsigned char>(s[2]);
    unsigned char b3 = static_cast<unsigned char>(s[3]);
    return ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
  }
  return b0;  // Fallback
}

// given the start and end of a tag, check to see if it matches a known tag
bool matches(const char* tag_name, const char* possible_tags[], const int possible_tag_count) {
  for (int i = 0; i < possible_tag_count; i++) {
    if (strcmp(tag_name, possible_tags[i]) == 0) {
      return true;
    }
  }
  return false;
}

// start a new text block if needed
void ChapterHtmlSlimParser::startNewTextBlock(const TextBlock::Style style) {
  if (currentTextBlock) {
    // already have a text block running and it is empty - just reuse it
    if (currentTextBlock->isEmpty()) {
      currentTextBlock->setStyle(style);
      return;
    }

    makePages();
  }
  currentTextBlock.reset(new ParsedText(style, extraParagraphSpacing, hyphenationEnabled, firstLineIndent));
}

void XMLCALL ChapterHtmlSlimParser::startElement(void* userData, const XML_Char* name, const XML_Char** atts) {
  auto* self = static_cast<ChapterHtmlSlimParser*>(userData);

  // Middle of skip
  if (self->skipUntilDepth < self->depth) {
    self->depth += 1;
    return;
  }

  // Special handling for tables - show placeholder text instead of dropping silently
  if (strcmp(name, "table") == 0) {
    // Add placeholder text
    self->startNewTextBlock(TextBlock::CENTER_ALIGN);
    if (self->currentTextBlock) {
      self->currentTextBlock->addWord("[Table omitted]", EpdFontFamily::ITALIC);
    }

    // Skip table contents
    self->skipUntilDepth = self->depth;
    self->depth += 1;
    return;
  }

  if (matches(name, IMAGE_TAGS, NUM_IMAGE_TAGS)) {
    // TODO: Start processing image tags
    std::string alt;
    if (atts != nullptr) {
      for (int i = 0; atts[i]; i += 2) {
        if (strcmp(atts[i], "alt") == 0) {
          alt = "[Image: " + std::string(atts[i + 1]) + "]";
        }
      }
      Serial.printf("[%lu] [EHP] Image alt: %s\n", millis(), alt.c_str());

      self->startNewTextBlock(TextBlock::CENTER_ALIGN);
      self->italicUntilDepth = min(self->italicUntilDepth, self->depth);
      self->depth += 1;
      self->characterData(userData, alt.c_str(), alt.length());

    } else {
      // Skip for now
      self->skipUntilDepth = self->depth;
      self->depth += 1;
      return;
    }
  }

  if (matches(name, SKIP_TAGS, NUM_SKIP_TAGS)) {
    // start skip
    self->skipUntilDepth = self->depth;
    self->depth += 1;
    return;
  }

  // Skip blocks with role="doc-pagebreak" and epub:type="pagebreak"
  if (atts != nullptr) {
    for (int i = 0; atts[i]; i += 2) {
      if (strcmp(atts[i], "role") == 0 && strcmp(atts[i + 1], "doc-pagebreak") == 0 ||
          strcmp(atts[i], "epub:type") == 0 && strcmp(atts[i + 1], "pagebreak") == 0) {
        self->skipUntilDepth = self->depth;
        self->depth += 1;
        return;
      }
    }
  }

  if (matches(name, HEADER_TAGS, NUM_HEADER_TAGS)) {
    self->startNewTextBlock(TextBlock::CENTER_ALIGN);
    self->boldUntilDepth = std::min(self->boldUntilDepth, self->depth);
  } else if (matches(name, BLOCK_TAGS, NUM_BLOCK_TAGS)) {
    if (strcmp(name, "br") == 0) {
      self->startNewTextBlock(self->currentTextBlock->getStyle());
    } else {
      self->startNewTextBlock((TextBlock::Style)self->paragraphAlignment);
      if (strcmp(name, "li") == 0) {
        self->currentTextBlock->addWord("\xe2\x80\xa2", EpdFontFamily::REGULAR);
      }
    }
  } else if (matches(name, BOLD_TAGS, NUM_BOLD_TAGS)) {
    self->boldUntilDepth = std::min(self->boldUntilDepth, self->depth);
  } else if (matches(name, ITALIC_TAGS, NUM_ITALIC_TAGS)) {
    self->italicUntilDepth = std::min(self->italicUntilDepth, self->depth);
  }

  self->depth += 1;
}

void XMLCALL ChapterHtmlSlimParser::characterData(void* userData, const XML_Char* s, const int len) {
  auto* self = static_cast<ChapterHtmlSlimParser*>(userData);

  // Middle of skip
  if (self->skipUntilDepth < self->depth) {
    return;
  }

  auto flushIfNeeded = [self]() {
    if (!self->currentTextBlock) {
      return;
    }
    const size_t wordCount = self->currentTextBlock->size();
    if (wordCount == 0) {
      return;
    }
    const size_t freeHeap = ESP.getFreeHeap();
    const bool tooManyWords = wordCount >= MAX_WORDS_BEFORE_FLUSH;
    const bool lowHeapWithBuffer =
        freeHeap < LOW_FREE_HEAP_BEFORE_FLUSH && wordCount >= MIN_WORDS_BEFORE_FLUSH;
    const bool criticalHeap = freeHeap < CRITICAL_FREE_HEAP_BEFORE_FLUSH;

    if (!(tooManyWords || lowHeapWithBuffer || criticalHeap)) {
      return;
    }

    const bool includeLastLine = criticalHeap;
    Serial.printf("[%lu] [EHP] Flushing text block (words=%u, free=%u)\n", millis(),
                  static_cast<unsigned>(wordCount),
                  static_cast<unsigned>(freeHeap));
      self->currentTextBlock->layoutAndExtractLines(
          self->renderer, self->fontId, self->viewportWidth,
          [self](const std::shared_ptr<TextBlock>& textBlock) { self->addLineToPage(textBlock); }, includeLastLine);
  };

  EpdFontFamily::Style fontStyle = EpdFontFamily::REGULAR;
  if (self->boldUntilDepth < self->depth && self->italicUntilDepth < self->depth) {
    fontStyle = EpdFontFamily::BOLD_ITALIC;
  } else if (self->boldUntilDepth < self->depth) {
    fontStyle = EpdFontFamily::BOLD;
  } else if (self->italicUntilDepth < self->depth) {
    fontStyle = EpdFontFamily::ITALIC;
  }

  int i = 0;
  while (i < len) {
    // Check for whitespace (ASCII only)
    if (isWhitespace(s[i])) {
      // Flush any buffered content as a word
      if (self->partWordBufferIndex > 0) {
        self->partWordBuffer[self->partWordBufferIndex] = '\0';
        self->currentTextBlock->addWord(self->partWordBuffer, fontStyle);
        self->partWordBufferIndex = 0;
        flushIfNeeded();
      }
      i++;
      continue;
    }

    // Determine UTF-8 character length
    const unsigned char b0 = static_cast<unsigned char>(s[i]);
    const int charLen = getUtf8ByteLength(b0);
    if (i + charLen > len) {
      // Incomplete UTF-8 sequence at end, just add the byte
      if (self->partWordBufferIndex < MAX_WORD_SIZE) {
        self->partWordBuffer[self->partWordBufferIndex++] = s[i];
      }
      i++;
      continue;
    }

    // Decode the codepoint to check if it's CJK
    const uint32_t cp = decodeUtf8Codepoint(&s[i], charLen);

    // Skip invisible/zero-width Unicode characters that fonts can't render
    if (isInvisibleCodepoint(cp)) {
      i += charLen;
      continue;
    }

    // Treat ideographic space (U+3000) as whitespace - flush buffer and skip
    if (cp == 0x3000) {
      if (self->partWordBufferIndex > 0) {
        self->partWordBuffer[self->partWordBufferIndex] = '\0';
        self->currentTextBlock->addWord(self->partWordBuffer, fontStyle);
        self->partWordBufferIndex = 0;
        flushIfNeeded();
      }
      i += charLen;
      continue;
    }

    if (isCjkCodepointForSplit(cp)) {
      // CJK character: flush any buffered ASCII content first
      if (self->partWordBufferIndex > 0) {
        self->partWordBuffer[self->partWordBufferIndex] = '\0';
        self->currentTextBlock->addWord(self->partWordBuffer, fontStyle);
        self->partWordBufferIndex = 0;
        flushIfNeeded();
      }

      // Add this CJK character as its own "word"
      char cjkWord[5] = {0};  // Max 4 bytes for UTF-8 + null terminator
      for (int j = 0; j < charLen && j < 4; j++) {
        cjkWord[j] = s[i + j];
      }
      self->currentTextBlock->addWord(cjkWord, fontStyle);
      flushIfNeeded();
      i += charLen;
      continue;
    }

    // Non-CJK character: buffer it
    // If we're about to run out of space, flush the buffer first
    if (self->partWordBufferIndex + charLen >= MAX_WORD_SIZE) {
      self->partWordBuffer[self->partWordBufferIndex] = '\0';
      self->currentTextBlock->addWord(self->partWordBuffer, fontStyle);
      self->partWordBufferIndex = 0;
      flushIfNeeded();
    }

    // Add all bytes of this character to the buffer
    for (int j = 0; j < charLen; j++) {
      self->partWordBuffer[self->partWordBufferIndex++] = s[i + j];
    }
    i += charLen;
  }

  flushIfNeeded();
}

void XMLCALL ChapterHtmlSlimParser::endElement(void* userData, const XML_Char* name) {
  auto* self = static_cast<ChapterHtmlSlimParser*>(userData);

  if (self->partWordBufferIndex > 0) {
    // Only flush out part word buffer if we're closing a block tag or are at the top of the HTML file.
    // We don't want to flush out content when closing inline tags like <span>.
    // Currently this also flushes out on closing <b> and <i> tags, but they are line tags so that shouldn't happen,
    // text styling needs to be overhauled to fix it.
    const bool shouldBreakText =
        matches(name, BLOCK_TAGS, NUM_BLOCK_TAGS) || matches(name, HEADER_TAGS, NUM_HEADER_TAGS) ||
        matches(name, BOLD_TAGS, NUM_BOLD_TAGS) || matches(name, ITALIC_TAGS, NUM_ITALIC_TAGS) || self->depth == 1;

    if (shouldBreakText) {
      EpdFontFamily::Style fontStyle = EpdFontFamily::REGULAR;
      if (self->boldUntilDepth < self->depth && self->italicUntilDepth < self->depth) {
        fontStyle = EpdFontFamily::BOLD_ITALIC;
      } else if (self->boldUntilDepth < self->depth) {
        fontStyle = EpdFontFamily::BOLD;
      } else if (self->italicUntilDepth < self->depth) {
        fontStyle = EpdFontFamily::ITALIC;
      }

      self->partWordBuffer[self->partWordBufferIndex] = '\0';
      self->currentTextBlock->addWord(self->partWordBuffer, fontStyle);
      self->partWordBufferIndex = 0;
    }
  }

  self->depth -= 1;

  // Leaving skip
  if (self->skipUntilDepth == self->depth) {
    self->skipUntilDepth = INT_MAX;
  }

  // Leaving bold
  if (self->boldUntilDepth == self->depth) {
    self->boldUntilDepth = INT_MAX;
  }

  // Leaving italic
  if (self->italicUntilDepth == self->depth) {
    self->italicUntilDepth = INT_MAX;
  }
}

bool ChapterHtmlSlimParser::parseAndBuildPages() {
  startNewTextBlock((TextBlock::Style)this->paragraphAlignment);

  const XML_Parser parser = XML_ParserCreate(nullptr);
  int done;

  if (!parser) {
    Serial.printf("[%lu] [EHP] Couldn't allocate memory for parser\n", millis());
    return false;
  }

  FsFile file;
  if (!SdMan.openFileForRead("EHP", filepath, file)) {
    XML_ParserFree(parser);
    return false;
  }

  // Get file size for progress calculation
  const size_t totalSize = file.size();
  size_t bytesRead = 0;
  int lastProgress = -1;

  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, startElement, endElement);
  XML_SetCharacterDataHandler(parser, characterData);

  do {
    void* const buf = XML_GetBuffer(parser, 1024);
    if (!buf) {
      Serial.printf("[%lu] [EHP] Couldn't allocate memory for buffer\n", millis());
      XML_StopParser(parser, XML_FALSE);                // Stop any pending processing
      XML_SetElementHandler(parser, nullptr, nullptr);  // Clear callbacks
      XML_SetCharacterDataHandler(parser, nullptr);
      XML_ParserFree(parser);
      file.close();
      return false;
    }

    const size_t len = file.read(buf, 1024);

    if (len == 0 && file.available() > 0) {
      Serial.printf("[%lu] [EHP] File read error\n", millis());
      XML_StopParser(parser, XML_FALSE);                // Stop any pending processing
      XML_SetElementHandler(parser, nullptr, nullptr);  // Clear callbacks
      XML_SetCharacterDataHandler(parser, nullptr);
      XML_ParserFree(parser);
      file.close();
      return false;
    }

    // Update progress (call every 10% change to avoid too frequent updates)
    // Only show progress for larger chapters where rendering overhead is worth it
    bytesRead += len;
    if (progressFn && totalSize >= MIN_SIZE_FOR_PROGRESS) {
      const int progress = static_cast<int>((bytesRead * 100) / totalSize);
      if (lastProgress / 10 != progress / 10) {
        lastProgress = progress;
        progressFn(progress);
      }
    }

    done = file.available() == 0;

    if (XML_ParseBuffer(parser, static_cast<int>(len), done) == XML_STATUS_ERROR) {
      Serial.printf("[%lu] [EHP] Parse error at line %lu:\n%s\n", millis(), XML_GetCurrentLineNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));
      XML_StopParser(parser, XML_FALSE);                // Stop any pending processing
      XML_SetElementHandler(parser, nullptr, nullptr);  // Clear callbacks
      XML_SetCharacterDataHandler(parser, nullptr);
      XML_ParserFree(parser);
      file.close();
      return false;
    }
  } while (!done);

  XML_StopParser(parser, XML_FALSE);                // Stop any pending processing
  XML_SetElementHandler(parser, nullptr, nullptr);  // Clear callbacks
  XML_SetCharacterDataHandler(parser, nullptr);
  XML_ParserFree(parser);
  file.close();

  // Process last page if there is still text
  if (currentTextBlock) {
    makePages();
    completePageFn(std::move(currentPage));
    currentPage.reset();
    currentTextBlock.reset();
  }

  return true;
}

void ChapterHtmlSlimParser::addLineToPage(std::shared_ptr<TextBlock> line) {
  const int lineHeight = renderer.getLineHeight(fontId) * lineCompression;

  if (currentPageNextY + lineHeight > viewportHeight) {
    completePageFn(std::move(currentPage));
    currentPage.reset(new Page());
    currentPageNextY = 0;
  }

  currentPage->elements.push_back(std::make_shared<PageLine>(line, 0, currentPageNextY));
  currentPageNextY += lineHeight;
}

void ChapterHtmlSlimParser::makePages() {
  if (!currentTextBlock) {
    Serial.printf("[%lu] [EHP] !! No text block to make pages for !!\n", millis());
    return;
  }

  if (!currentPage) {
    currentPage.reset(new Page());
    currentPageNextY = 0;
  }

  const int lineHeight = renderer.getLineHeight(fontId) * lineCompression;
  currentTextBlock->layoutAndExtractLines(
      renderer, fontId, viewportWidth,
      [this](const std::shared_ptr<TextBlock>& textBlock) { addLineToPage(textBlock); });
  // Extra paragraph spacing if enabled
  if (extraParagraphSpacing) {
    currentPageNextY += lineHeight / 2;
  }
}

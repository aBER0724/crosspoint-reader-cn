#include "ChapterHtmlSlimParser.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <HardwareSerial.h>
#include <expat.h>

#include "../Page.h"

const char* HEADER_TAGS[] = {"h1", "h2", "h3", "h4", "h5", "h6"};
constexpr int NUM_HEADER_TAGS = sizeof(HEADER_TAGS) / sizeof(HEADER_TAGS[0]);

// Minimum file size (in bytes) to show indexing popup - smaller chapters don't benefit from it
constexpr size_t MIN_SIZE_FOR_POPUP = 50 * 1024;  // 50KB
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

const char* UNDERLINE_TAGS[] = {"u", "ins"};
constexpr int NUM_UNDERLINE_TAGS = sizeof(UNDERLINE_TAGS) / sizeof(UNDERLINE_TAGS[0]);

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

bool isHeaderOrBlock(const char* name) {
  return matches(name, HEADER_TAGS, NUM_HEADER_TAGS) || matches(name, BLOCK_TAGS, NUM_BLOCK_TAGS);
}

// Update effective bold/italic/underline based on block style and inline style stack
void ChapterHtmlSlimParser::updateEffectiveInlineStyle() {
  // Start with block-level styles
  effectiveBold = currentCssStyle.hasFontWeight() && currentCssStyle.fontWeight == CssFontWeight::Bold;
  effectiveItalic = currentCssStyle.hasFontStyle() && currentCssStyle.fontStyle == CssFontStyle::Italic;
  effectiveUnderline =
      currentCssStyle.hasTextDecoration() && currentCssStyle.textDecoration == CssTextDecoration::Underline;

  // Apply inline style stack in order
  for (const auto& entry : inlineStyleStack) {
    if (entry.hasBold) {
      effectiveBold = entry.bold;
    }
    if (entry.hasItalic) {
      effectiveItalic = entry.italic;
    }
    if (entry.hasUnderline) {
      effectiveUnderline = entry.underline;
    }
  }
}

// flush the contents of partWordBuffer to currentTextBlock
void ChapterHtmlSlimParser::flushPartWordBuffer() {
  // Determine font style from depth-based tracking and CSS effective style
  const bool isBold = boldUntilDepth < depth || effectiveBold;
  const bool isItalic = italicUntilDepth < depth || effectiveItalic;
  const bool isUnderline = underlineUntilDepth < depth || effectiveUnderline;

  // Combine style flags using bitwise OR
  EpdFontFamily::Style fontStyle = EpdFontFamily::REGULAR;
  if (isBold) {
    fontStyle = static_cast<EpdFontFamily::Style>(fontStyle | EpdFontFamily::BOLD);
  }
  if (isItalic) {
    fontStyle = static_cast<EpdFontFamily::Style>(fontStyle | EpdFontFamily::ITALIC);
  }
  if (isUnderline) {
    fontStyle = static_cast<EpdFontFamily::Style>(fontStyle | EpdFontFamily::UNDERLINE);
  }

  // flush the buffer
  partWordBuffer[partWordBufferIndex] = '\0';
  currentTextBlock->addWord(partWordBuffer, fontStyle, false, nextWordContinues);
  partWordBufferIndex = 0;
  nextWordContinues = false;
}

// start a new text block if needed
void ChapterHtmlSlimParser::startNewTextBlock(const BlockStyle& blockStyle) {
  nextWordContinues = false;  // New block = new paragraph, no continuation
  if (currentTextBlock) {
    // already have a text block running and it is empty - just reuse it
    if (currentTextBlock->isEmpty()) {
      // Merge with existing block style to accumulate CSS styling from parent block elements.
      // This handles cases like <div style="margin-bottom:2em"><h1>text</h1></div> where the
      // div's margin should be preserved, even though it has no direct text content.
      currentTextBlock->setBlockStyle(currentTextBlock->getBlockStyle().getCombinedBlockStyle(blockStyle));
      return;
    }

    makePages();
  }
  currentTextBlock.reset(new ParsedText(extraParagraphSpacing, hyphenationEnabled, blockStyle, firstLineIndent));
}

void XMLCALL ChapterHtmlSlimParser::startElement(void* userData, const XML_Char* name, const XML_Char** atts) {
  auto* self = static_cast<ChapterHtmlSlimParser*>(userData);

  // Middle of skip
  if (self->skipUntilDepth < self->depth) {
    self->depth += 1;
    return;
  }

  // Extract class and style attributes for CSS processing
  std::string classAttr;
  std::string styleAttr;
  if (atts != nullptr) {
    for (int i = 0; atts[i]; i += 2) {
      if (strcmp(atts[i], "class") == 0) {
        classAttr = atts[i + 1];
      } else if (strcmp(atts[i], "style") == 0) {
        styleAttr = atts[i + 1];
      }
    }
  }

  auto centeredBlockStyle = BlockStyle();
  centeredBlockStyle.textAlignDefined = true;
  centeredBlockStyle.alignment = CssTextAlign::Center;

  // Special handling for tables - show placeholder text instead of dropping silently
  if (strcmp(name, "table") == 0) {
    // Add placeholder text
    self->startNewTextBlock(centeredBlockStyle);

    self->italicUntilDepth = min(self->italicUntilDepth, self->depth);
    // Advance depth before processing character data (like you would for an element with text)
    self->depth += 1;
    self->characterData(userData, "[Table omitted]", strlen("[Table omitted]"));

    // Skip table contents (skip until parent as we pre-advanced depth above)
    self->skipUntilDepth = self->depth - 1;
    return;
  }

  if (matches(name, IMAGE_TAGS, NUM_IMAGE_TAGS)) {
    // TODO: Start processing image tags
    std::string alt = "[Image]";
    if (atts != nullptr) {
      for (int i = 0; atts[i]; i += 2) {
        if (strcmp(atts[i], "alt") == 0) {
          if (strlen(atts[i + 1]) > 0) {
            alt = "[Image: " + std::string(atts[i + 1]) + "]";
          }
          break;
        }
      }
    }

    Serial.printf("[%lu] [EHP] Image alt: %s\n", millis(), alt.c_str());

    self->startNewTextBlock(centeredBlockStyle);
    self->italicUntilDepth = min(self->italicUntilDepth, self->depth);
    // Advance depth before processing character data (like you would for an element with text)
    self->depth += 1;
    self->characterData(userData, alt.c_str(), alt.length());

    // Skip table contents (skip until parent as we pre-advanced depth above)
    self->skipUntilDepth = self->depth - 1;
    return;
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

  // Compute CSS style for this element
  CssStyle cssStyle;
  if (self->cssParser) {
    // Get combined tag + class styles
    cssStyle = self->cssParser->resolveStyle(name, classAttr);
    // Merge inline style (highest priority)
    if (!styleAttr.empty()) {
      CssStyle inlineStyle = CssParser::parseInlineStyle(styleAttr);
      cssStyle.applyOver(inlineStyle);
    }
  }

  const float emSize = static_cast<float>(self->renderer.getLineHeight(self->fontId)) * self->lineCompression;
  const auto userAlignmentBlockStyle = BlockStyle::fromCssStyle(
      cssStyle, emSize, static_cast<CssTextAlign>(self->paragraphAlignment), self->viewportWidth);

  if (matches(name, HEADER_TAGS, NUM_HEADER_TAGS)) {
    self->currentCssStyle = cssStyle;
    auto headerBlockStyle = BlockStyle::fromCssStyle(cssStyle, emSize, CssTextAlign::Center, self->viewportWidth);
    headerBlockStyle.textAlignDefined = true;
    if (self->embeddedStyle && cssStyle.hasTextAlign()) {
      headerBlockStyle.alignment = cssStyle.textAlign;
    }
    self->startNewTextBlock(headerBlockStyle);
    self->boldUntilDepth = std::min(self->boldUntilDepth, self->depth);
    self->updateEffectiveInlineStyle();
  } else if (matches(name, BLOCK_TAGS, NUM_BLOCK_TAGS)) {
    if (strcmp(name, "br") == 0) {
      if (self->partWordBufferIndex > 0) {
        // flush word preceding <br/> to currentTextBlock before calling startNewTextBlock
        self->flushPartWordBuffer();
      }
      self->startNewTextBlock(self->currentTextBlock->getBlockStyle());
    } else {
      self->currentCssStyle = cssStyle;
      self->startNewTextBlock(userAlignmentBlockStyle);
      self->updateEffectiveInlineStyle();

      if (strcmp(name, "li") == 0) {
        self->currentTextBlock->addWord("\xe2\x80\xa2", EpdFontFamily::REGULAR);
      }
    }
  } else if (matches(name, UNDERLINE_TAGS, NUM_UNDERLINE_TAGS)) {
    // Flush buffer before style change so preceding text gets current style
    if (self->partWordBufferIndex > 0) {
      self->flushPartWordBuffer();
      self->nextWordContinues = true;
    }
    self->underlineUntilDepth = std::min(self->underlineUntilDepth, self->depth);
    // Push inline style entry for underline tag
    StyleStackEntry entry;
    entry.depth = self->depth;  // Track depth for matching pop
    entry.hasUnderline = true;
    entry.underline = true;
    if (cssStyle.hasFontWeight()) {
      entry.hasBold = true;
      entry.bold = cssStyle.fontWeight == CssFontWeight::Bold;
    }
    if (cssStyle.hasFontStyle()) {
      entry.hasItalic = true;
      entry.italic = cssStyle.fontStyle == CssFontStyle::Italic;
    }
    self->inlineStyleStack.push_back(entry);
    self->updateEffectiveInlineStyle();
  } else if (matches(name, BOLD_TAGS, NUM_BOLD_TAGS)) {
    // Flush buffer before style change so preceding text gets current style
    if (self->partWordBufferIndex > 0) {
      self->flushPartWordBuffer();
      self->nextWordContinues = true;
    }
    self->boldUntilDepth = std::min(self->boldUntilDepth, self->depth);
    // Push inline style entry for bold tag
    StyleStackEntry entry;
    entry.depth = self->depth;  // Track depth for matching pop
    entry.hasBold = true;
    entry.bold = true;
    if (cssStyle.hasFontStyle()) {
      entry.hasItalic = true;
      entry.italic = cssStyle.fontStyle == CssFontStyle::Italic;
    }
    if (cssStyle.hasTextDecoration()) {
      entry.hasUnderline = true;
      entry.underline = cssStyle.textDecoration == CssTextDecoration::Underline;
    }
    self->inlineStyleStack.push_back(entry);
    self->updateEffectiveInlineStyle();
  } else if (matches(name, ITALIC_TAGS, NUM_ITALIC_TAGS)) {
    // Flush buffer before style change so preceding text gets current style
    if (self->partWordBufferIndex > 0) {
      self->flushPartWordBuffer();
      self->nextWordContinues = true;
    }
    self->italicUntilDepth = std::min(self->italicUntilDepth, self->depth);
    // Push inline style entry for italic tag
    StyleStackEntry entry;
    entry.depth = self->depth;  // Track depth for matching pop
    entry.hasItalic = true;
    entry.italic = true;
    if (cssStyle.hasFontWeight()) {
      entry.hasBold = true;
      entry.bold = cssStyle.fontWeight == CssFontWeight::Bold;
    }
    if (cssStyle.hasTextDecoration()) {
      entry.hasUnderline = true;
      entry.underline = cssStyle.textDecoration == CssTextDecoration::Underline;
    }
    self->inlineStyleStack.push_back(entry);
    self->updateEffectiveInlineStyle();
  } else if (strcmp(name, "span") == 0 || !isHeaderOrBlock(name)) {
    // Handle span and other inline elements for CSS styling
    if (cssStyle.hasFontWeight() || cssStyle.hasFontStyle() || cssStyle.hasTextDecoration()) {
      // Flush buffer before style change so preceding text gets current style
      if (self->partWordBufferIndex > 0) {
        self->flushPartWordBuffer();
        self->nextWordContinues = true;
      }
      StyleStackEntry entry;
      entry.depth = self->depth;  // Track depth for matching pop
      if (cssStyle.hasFontWeight()) {
        entry.hasBold = true;
        entry.bold = cssStyle.fontWeight == CssFontWeight::Bold;
      }
      if (cssStyle.hasFontStyle()) {
        entry.hasItalic = true;
        entry.italic = cssStyle.fontStyle == CssFontStyle::Italic;
      }
      if (cssStyle.hasTextDecoration()) {
        entry.hasUnderline = true;
        entry.underline = cssStyle.textDecoration == CssTextDecoration::Underline;
      }
      self->inlineStyleStack.push_back(entry);
      self->updateEffectiveInlineStyle();
    }
  }

  // Unprocessed tag, just increasing depth and continue forward
  self->depth += 1;
}

void XMLCALL ChapterHtmlSlimParser::characterData(void* userData, const XML_Char* s, const int len) {
  auto* self = static_cast<ChapterHtmlSlimParser*>(userData);

  // Middle of skip
  if (self->skipUntilDepth < self->depth) {
    return;
  }

  // Periodic flush to limit memory usage on large text blocks (important for ESP32)
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

  int i = 0;
  while (i < len) {
    // Check for whitespace (ASCII only)
    if (isWhitespace(s[i])) {
      // Flush any buffered content as a word
      if (self->partWordBufferIndex > 0) {
        self->flushPartWordBuffer();
        flushIfNeeded();
      }
      // Whitespace is a real word boundary -- reset continuation state
      self->nextWordContinues = false;
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

    // Decode the codepoint to check if it's CJK or invisible
    const uint32_t cp = decodeUtf8Codepoint(&s[i], charLen);

    // Skip invisible/zero-width Unicode characters that fonts can't render
    if (isInvisibleCodepoint(cp)) {
      i += charLen;
      continue;
    }

    // Treat ideographic space (U+3000) as whitespace - flush buffer and skip
    if (cp == 0x3000) {
      if (self->partWordBufferIndex > 0) {
        self->flushPartWordBuffer();
        flushIfNeeded();
      }
      self->nextWordContinues = false;
      i += charLen;
      continue;
    }

    if (isCjkCodepointForSplit(cp)) {
      // CJK character: flush any buffered content first
      if (self->partWordBufferIndex > 0) {
        self->flushPartWordBuffer();
        flushIfNeeded();
      }

      // Add this CJK character as its own "word"
      char cjkWord[5] = {0};  // Max 4 bytes for UTF-8 + null terminator
      for (int j = 0; j < charLen && j < 4; j++) {
        cjkWord[j] = s[i + j];
      }
      self->currentTextBlock->addWord(cjkWord, EpdFontFamily::REGULAR);
      flushIfNeeded();
      i += charLen;
      continue;
    }

    // Non-CJK character: buffer it
    // If we're about to run out of space, flush the buffer first
    if (self->partWordBufferIndex + charLen >= MAX_WORD_SIZE) {
      self->flushPartWordBuffer();
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

  // Check if any style state will change after we decrement depth
  // If so, we MUST flush the partWordBuffer with the CURRENT style first
  // Note: depth hasn't been decremented yet, so we check against (depth - 1)
  const bool willPopStyleStack =
      !self->inlineStyleStack.empty() && self->inlineStyleStack.back().depth == self->depth - 1;
  const bool willClearBold = self->boldUntilDepth == self->depth - 1;
  const bool willClearItalic = self->italicUntilDepth == self->depth - 1;
  const bool willClearUnderline = self->underlineUntilDepth == self->depth - 1;

  const bool styleWillChange = willPopStyleStack || willClearBold || willClearItalic || willClearUnderline;
  const bool headerOrBlockTag = isHeaderOrBlock(name);

  // Flush buffer with current style BEFORE any style changes
  if (self->partWordBufferIndex > 0) {
    // Flush if style will change OR if we're closing a block/structural element
    const bool isInlineTag = !headerOrBlockTag && strcmp(name, "table") != 0 &&
                             !matches(name, IMAGE_TAGS, NUM_IMAGE_TAGS) && self->depth != 1;
    const bool shouldFlush = styleWillChange || headerOrBlockTag || matches(name, BOLD_TAGS, NUM_BOLD_TAGS) ||
                             matches(name, ITALIC_TAGS, NUM_ITALIC_TAGS) ||
                             matches(name, UNDERLINE_TAGS, NUM_UNDERLINE_TAGS) || strcmp(name, "table") == 0 ||
                             matches(name, IMAGE_TAGS, NUM_IMAGE_TAGS) || self->depth == 1;

    if (shouldFlush) {
      self->flushPartWordBuffer();
      // If closing an inline element, the next word fragment continues the same visual word
      if (isInlineTag) {
        self->nextWordContinues = true;
      }
    }
  }

  self->depth -= 1;

  // Leaving skip
  if (self->skipUntilDepth == self->depth) {
    self->skipUntilDepth = INT_MAX;
  }

  // Leaving bold tag
  if (self->boldUntilDepth == self->depth) {
    self->boldUntilDepth = INT_MAX;
  }

  // Leaving italic tag
  if (self->italicUntilDepth == self->depth) {
    self->italicUntilDepth = INT_MAX;
  }

  // Leaving underline tag
  if (self->underlineUntilDepth == self->depth) {
    self->underlineUntilDepth = INT_MAX;
  }

  // Pop from inline style stack if we pushed an entry at this depth
  // This handles all inline elements: b, i, u, span, etc.
  if (!self->inlineStyleStack.empty() && self->inlineStyleStack.back().depth == self->depth) {
    self->inlineStyleStack.pop_back();
    self->updateEffectiveInlineStyle();
  }

  // Clear block style when leaving header or block elements
  if (headerOrBlockTag) {
    self->currentCssStyle.reset();
    self->updateEffectiveInlineStyle();
  }
}

bool ChapterHtmlSlimParser::parseAndBuildPages() {
  auto paragraphAlignmentBlockStyle = BlockStyle();
  paragraphAlignmentBlockStyle.textAlignDefined = true;
  // Resolve None sentinel to Justify for initial block (no CSS context yet)
  const auto align = (this->paragraphAlignment == static_cast<uint8_t>(CssTextAlign::None))
                         ? CssTextAlign::Justify
                         : static_cast<CssTextAlign>(this->paragraphAlignment);
  paragraphAlignmentBlockStyle.alignment = align;
  startNewTextBlock(paragraphAlignmentBlockStyle);

  const XML_Parser parser = XML_ParserCreate(nullptr);
  int done;

  if (!parser) {
    Serial.printf("[%lu] [EHP] Couldn't allocate memory for parser\n", millis());
    return false;
  }

  FsFile file;
  if (!Storage.openFileForRead("EHP", filepath, file)) {
    XML_ParserFree(parser);
    return false;
  }

  // Get file size to decide whether to show indexing popup.
  if (popupFn && file.size() >= MIN_SIZE_FOR_POPUP) {
    popupFn();
  }

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

  // Apply horizontal left inset (margin + padding) as x position offset
  const int16_t xOffset = line->getBlockStyle().leftInset();
  currentPage->elements.push_back(std::make_shared<PageLine>(line, xOffset, currentPageNextY));
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

  // Apply top spacing before the paragraph (stored in pixels)
  const BlockStyle& blockStyle = currentTextBlock->getBlockStyle();
  if (blockStyle.marginTop > 0) {
    currentPageNextY += blockStyle.marginTop;
  }
  if (blockStyle.paddingTop > 0) {
    currentPageNextY += blockStyle.paddingTop;
  }

  // Calculate effective width accounting for horizontal margins/padding
  const int horizontalInset = blockStyle.totalHorizontalInset();
  const uint16_t effectiveWidth =
      (horizontalInset < viewportWidth) ? static_cast<uint16_t>(viewportWidth - horizontalInset) : viewportWidth;

  currentTextBlock->layoutAndExtractLines(
      renderer, fontId, effectiveWidth,
      [this](const std::shared_ptr<TextBlock>& textBlock) { addLineToPage(textBlock); });

  // Apply bottom spacing after the paragraph (stored in pixels)
  if (blockStyle.marginBottom > 0) {
    currentPageNextY += blockStyle.marginBottom;
  }
  if (blockStyle.paddingBottom > 0) {
    currentPageNextY += blockStyle.paddingBottom;
  }

  // Extra paragraph spacing if enabled (default behavior)
  if (extraParagraphSpacing) {
    currentPageNextY += lineHeight / 2;
  }
}

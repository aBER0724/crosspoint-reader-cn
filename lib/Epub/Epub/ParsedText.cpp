#include "ParsedText.h"

#include <GfxRenderer.h>
#include <Utf8.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <vector>

constexpr int MAX_COST = std::numeric_limits<int>::max();

namespace {
bool isCjkCodepoint(const uint32_t cp) {
  if (cp >= 0x4E00 && cp <= 0x9FFF)
    return true;
  if (cp >= 0x3400 && cp <= 0x4DBF)
    return true;
  if (cp >= 0x3000 && cp <= 0x303F)
    return true;
  if (cp >= 0x3040 && cp <= 0x309F)
    return true;
  if (cp >= 0x30A0 && cp <= 0x30FF)
    return true;
  if (cp >= 0xF900 && cp <= 0xFAFF)
    return true;
  if (cp >= 0xFF00 && cp <= 0xFFEF)
    return true;
  if (cp >= 0x2000 && cp <= 0x206F)
    return true;
  return false;
}

bool isCjkSpacingCodepoint(const uint32_t cp) {
  if (cp == 0x20)
    return true;
  if (cp >= 0x2000 && cp <= 0x200B)
    return true;
  if (cp == 0x3000)
    return true;
  return false;
}

bool isCjkWord(const std::string& word) {
  const unsigned char* ptr = reinterpret_cast<const unsigned char*>(word.c_str());
  uint32_t cp;
  bool hasCjk = false;
  bool hasNonCjkVisible = false;
  bool hasSpacingOnly = true;

  while ((cp = utf8NextCodepoint(&ptr))) {
    if (isCjkSpacingCodepoint(cp)) {
      continue;
    }
    hasSpacingOnly = false;
    if (isCjkCodepoint(cp)) {
      hasCjk = true;
    } else {
      hasNonCjkVisible = true;
    }
  }

  if (hasCjk && !hasNonCjkVisible) {
    return true;
  }
  if (hasSpacingOnly && !word.empty()) {
    return true;
  }
  return false;
}

bool isCjkParagraph(const std::vector<uint8_t>& wordIsCjk) {
  if (wordIsCjk.empty()) {
    return false;
  }

  size_t cjkCount = 0;
  for (const auto flag : wordIsCjk) {
    if (flag) {
      ++cjkCount;
    }
  }

  const size_t total = wordIsCjk.size();
  return cjkCount * 10 >= total * 6;
}

int baseGapWidth(const size_t leftIndex, const size_t rightIndex, const int spaceWidth,
                 const std::vector<uint8_t>& wordIsCjk) {
  if (leftIndex >= wordIsCjk.size() || rightIndex >= wordIsCjk.size()) {
    return spaceWidth;
  }

  if (wordIsCjk[leftIndex] || wordIsCjk[rightIndex]) {
    return 0;
  }

  return spaceWidth;
}

bool isCjkGap(const size_t leftIndex, const size_t rightIndex, const std::vector<uint8_t>& wordIsCjk) {
  if (leftIndex >= wordIsCjk.size() || rightIndex >= wordIsCjk.size()) {
    return false;
  }

  return wordIsCjk[leftIndex] || wordIsCjk[rightIndex];
}
} // namespace

void ParsedText::addWord(std::string word, const EpdFontFamily::Style fontStyle) {
  if (word.empty()) return;

  words.push_back(std::move(word));
  wordStyles.push_back(fontStyle);
}

// Consumes data to minimize memory usage
void ParsedText::layoutAndExtractLines(const GfxRenderer& renderer, const int fontId, const uint16_t viewportWidth,
                                       const std::function<void(std::shared_ptr<TextBlock>)>& processLine,
                                       const bool includeLastLine) {
  if (words.empty()) {
    return;
  }

  const int pageWidth = viewportWidth;
  const int spaceWidth = renderer.getSpaceWidth(fontId);
  std::vector<uint8_t> wordIsCjk;
  const auto wordWidths = calculateWordWidths(renderer, fontId, &wordIsCjk);
  const bool cjkParagraph = isCjkParagraph(wordIsCjk);
  const auto lineBreakIndices = computeLineBreaks(pageWidth, spaceWidth, wordWidths, wordIsCjk);
  const size_t lineCount = includeLastLine ? lineBreakIndices.size() : lineBreakIndices.size() - 1;

  for (size_t i = 0; i < lineCount; ++i) {
    extractLine(i, pageWidth, spaceWidth, wordWidths, wordIsCjk, lineBreakIndices, processLine, cjkParagraph);
  }
}

std::vector<uint16_t> ParsedText::calculateWordWidths(const GfxRenderer& renderer, const int fontId,
                                                      std::vector<uint8_t>* wordIsCjk) {
  const size_t totalWordCount = words.size();

  std::vector<uint16_t> wordWidths;
  wordWidths.reserve(totalWordCount);

  if (wordIsCjk) {
    wordIsCjk->clear();
    wordIsCjk->reserve(totalWordCount);
  }

  // Add two em-spaces at the beginning of first word in paragraph for CJK indentation
  // This ensures 2-character indentation for Chinese/Japanese text
  if ((style == TextBlock::LEFT_ALIGN || style == TextBlock::JUSTIFIED) && !words.empty()) {
    std::string& first_word = words.front();
    // Insert two em-spaces (U+2003) for 2-character indentation
    first_word.insert(0, "\xe2\x80\x83\xe2\x80\x83");
  }

  auto wordsIt = words.begin();
  auto wordStylesIt = wordStyles.begin();

  while (wordsIt != words.end()) {
    wordWidths.push_back(renderer.getTextWidth(fontId, wordsIt->c_str(), *wordStylesIt));
    if (wordIsCjk) {
      wordIsCjk->push_back(isCjkWord(*wordsIt) ? 1 : 0);
    }

    std::advance(wordsIt, 1);
    std::advance(wordStylesIt, 1);
  }

  return wordWidths;
}

std::vector<size_t> ParsedText::computeLineBreaks(const int pageWidth, const int spaceWidth,
                                                  const std::vector<uint16_t>& wordWidths,
                                                  const std::vector<uint8_t>& wordIsCjk) const {
  const size_t totalWordCount = words.size();

  // DP table to store the minimum badness (cost) of lines starting at index i
  std::vector<int> dp(totalWordCount);
  // 'ans[i]' stores the index 'j' of the *last word* in the optimal line starting at 'i'
  std::vector<size_t> ans(totalWordCount);

  // Base Case
  dp[totalWordCount - 1] = 0;
  ans[totalWordCount - 1] = totalWordCount - 1;

  for (int i = totalWordCount - 2; i >= 0; --i) {
    int currlen = 0;
    dp[i] = MAX_COST;

    for (size_t j = i; j < totalWordCount; ++j) {
      // Current line length: previous width + space + current word width
      if (j > static_cast<size_t>(i)) {
        currlen += baseGapWidth(j - 1, j, spaceWidth, wordIsCjk);
      }
      currlen += wordWidths[j];

      if (currlen > pageWidth) {
        break;
      }

      int cost;
      if (j == totalWordCount - 1) {
        cost = 0;  // Last line
      } else {
        const int remainingSpace = pageWidth - currlen;
        // Use long long for the square to prevent overflow
        const long long cost_ll = static_cast<long long>(remainingSpace) * remainingSpace + dp[j + 1];

        if (cost_ll > MAX_COST) {
          cost = MAX_COST;
        } else {
          cost = static_cast<int>(cost_ll);
        }
      }

      if (cost < dp[i]) {
        dp[i] = cost;
        ans[i] = j;  // j is the index of the last word in this optimal line
      }
    }

    // Handle oversized word: if no valid configuration found, force single-word line
    // This prevents cascade failure where one oversized word breaks all preceding words
    if (dp[i] == MAX_COST) {
      ans[i] = i;  // Just this word on its own line
      // Inherit cost from next word to allow subsequent words to find valid configurations
      if (i + 1 < static_cast<int>(totalWordCount)) {
        dp[i] = dp[i + 1];
      } else {
        dp[i] = 0;
      }
    }
  }

  // Stores the index of the word that starts the next line (last_word_index + 1)
  std::vector<size_t> lineBreakIndices;
  size_t currentWordIndex = 0;

  while (currentWordIndex < totalWordCount) {
    size_t nextBreakIndex = ans[currentWordIndex] + 1;

    // Safety check: prevent infinite loop if nextBreakIndex doesn't advance
    if (nextBreakIndex <= currentWordIndex) {
      // Force advance by at least one word to avoid infinite loop
      nextBreakIndex = currentWordIndex + 1;
    }

    lineBreakIndices.push_back(nextBreakIndex);
    currentWordIndex = nextBreakIndex;
  }

  return lineBreakIndices;
}

void ParsedText::extractLine(const size_t breakIndex, const int pageWidth, const int spaceWidth,
                             const std::vector<uint16_t>& wordWidths, const std::vector<uint8_t>& wordIsCjk,
                             const std::vector<size_t>& lineBreakIndices,
                             const std::function<void(std::shared_ptr<TextBlock>)>& processLine,
                             const bool cjkParagraph) {
  const size_t lineBreak = lineBreakIndices[breakIndex];
  const size_t lastBreakAt = breakIndex > 0 ? lineBreakIndices[breakIndex - 1] : 0;
  const size_t lineWordCount = lineBreak - lastBreakAt;

  // Calculate total word width for this line
  int lineWordWidthSum = 0;
  for (size_t i = lastBreakAt; i < lineBreak; i++) {
    lineWordWidthSum += wordWidths[i];
  }

  // Calculate spacing
  int baseGapSum = 0;
  if (lineWordCount >= 2) {
    for (size_t i = lastBreakAt; i + 1 < lineBreak; ++i) {
      baseGapSum += baseGapWidth(i, i + 1, spaceWidth, wordIsCjk);
    }
  }
  int spareSpace = pageWidth - (lineWordWidthSum + baseGapSum);
  if (spareSpace < 0) {
    spareSpace = 0;
  }

  const bool isLastLine = breakIndex == lineBreakIndices.size() - 1;

  int adjustableGapCount = 0;
  if (!isLastLine && lineWordCount >= 2) {
    if (style == TextBlock::JUSTIFIED) {
      adjustableGapCount = static_cast<int>(lineWordCount - 1);
    } else if (style == TextBlock::LEFT_ALIGN && cjkParagraph) {
      for (size_t i = lastBreakAt; i + 1 < lineBreak; ++i) {
        if (isCjkGap(i, i + 1, wordIsCjk)) {
          ++adjustableGapCount;
        }
      }
    }
  }

  int extraPerGap = 0;
  int extraRemainder = 0;
  if (adjustableGapCount > 0 && spareSpace > 0) {
    extraPerGap = spareSpace / adjustableGapCount;
    extraRemainder = spareSpace % adjustableGapCount;
  }

  // Calculate initial x position
  uint16_t xpos = 0;
  if (style == TextBlock::RIGHT_ALIGN) {
    xpos = static_cast<uint16_t>(spareSpace);
  } else if (style == TextBlock::CENTER_ALIGN) {
    xpos = static_cast<uint16_t>(spareSpace / 2);
  }

  // Pre-calculate X positions for words
  std::list<uint16_t> lineXPos;
  for (size_t i = lastBreakAt; i < lineBreak; i++) {
    const uint16_t currentWordWidth = wordWidths[i];
    lineXPos.push_back(xpos);
    xpos += currentWordWidth;
    if (i + 1 < lineBreak) {
      int gap = baseGapWidth(i, i + 1, spaceWidth, wordIsCjk);
      if (!isLastLine && lineWordCount >= 2) {
        if (style == TextBlock::JUSTIFIED ||
            (style == TextBlock::LEFT_ALIGN && cjkParagraph && isCjkGap(i, i + 1, wordIsCjk))) {
          gap += extraPerGap;
          if (extraRemainder > 0) {
            gap += 1;
            --extraRemainder;
          }
        }
      }
      xpos += gap;
    }
  }

  // Iterators always start at the beginning as we are moving content with splice below
  auto wordEndIt = words.begin();
  auto wordStyleEndIt = wordStyles.begin();
  std::advance(wordEndIt, lineWordCount);
  std::advance(wordStyleEndIt, lineWordCount);

  // *** CRITICAL STEP: CONSUME DATA USING SPLICE ***
  std::list<std::string> lineWords;
  lineWords.splice(lineWords.begin(), words, words.begin(), wordEndIt);
  std::list<EpdFontFamily::Style> lineWordStyles;
  lineWordStyles.splice(lineWordStyles.begin(), wordStyles, wordStyles.begin(), wordStyleEndIt);

  processLine(std::make_shared<TextBlock>(std::move(lineWords), std::move(lineXPos), std::move(lineWordStyles), style));
}

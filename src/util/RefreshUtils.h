#pragma once

namespace refresh_utils {
inline bool shouldUseHalfRefresh(int &pagesUntilFullRefresh,
                                 const int refreshFrequency) {
  if (pagesUntilFullRefresh <= 1) {
    pagesUntilFullRefresh = refreshFrequency;
    return true;
  }
  pagesUntilFullRefresh--;
  return false;
}
} // namespace refresh_utils

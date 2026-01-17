#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <functional>

#include "activities/ActivityWithSubactivity.h"

/**
 * Font selection page
 * Shows available external fonts and allows user to select
 */
class FontSelectActivity final : public ActivityWithSubactivity {
public:
  enum class SelectMode { Reader, UI };
  explicit FontSelectActivity(GfxRenderer &renderer,
                              MappedInputManager &mappedInput, SelectMode mode,
                              const std::function<void()> &onBack)
      : ActivityWithSubactivity("FontSelect", renderer, mappedInput),
        mode(mode), onBack(onBack) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;

private:
  SelectMode mode;
  TaskHandle_t displayTaskHandle = nullptr;
  SemaphoreHandle_t renderingMutex = nullptr;
  bool updateRequired = false;

  int selectedIndex = 0; // 0 = Built-in, 1+ = external fonts
  int totalItems = 1;    // At least "Built-in" option
  const std::function<void()> onBack;

  static void taskTrampoline(void *param);
  [[noreturn]] void displayTaskLoop();
  void render();
  void handleSelection();
};

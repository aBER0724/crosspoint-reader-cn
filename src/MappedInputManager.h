#pragma once

#include <InputManager.h>

class MappedInputManager {
 public:
  enum class Button { Back, Confirm, Left, Right, Up, Down, Power, PageBack, PageForward };

  struct Labels {
    const char* btn1;
    const char* btn2;
    const char* btn3;
    const char* btn4;
  };

  explicit MappedInputManager(InputManager& inputManager) : inputManager(inputManager) {}

  bool wasPressed(Button button) const;
  bool wasReleased(Button button) const;
  bool isPressed(Button button) const;
  bool wasAnyPressed() const;
  bool wasAnyReleased() const;
  unsigned long getHeldTime() const;
  Labels mapLabels(const char* back, const char* confirm, const char* previous, const char* next) const;
  void setReadingOrientationActive(bool active) { readingOrientationActive = active; }
  bool isReadingOrientationActive() const { return readingOrientationActive; }

 private:
  InputManager& inputManager;
  bool readingOrientationActive = false;
  decltype(InputManager::BTN_BACK) mapButton(Button button) const;
  bool shouldReverseFrontRow() const;
  bool shouldSwapLeftRight() const;
  bool shouldSwapSideButtons() const;
};

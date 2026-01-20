#include "MappedInputManager.h"

#include <cstdint>

#include "CrossPointSettings.h"

namespace {
using InputBtn = uint8_t;

enum class FrontAction { Back, Confirm, Left, Right };

void getFrontActionOrder(const CrossPointSettings::FRONT_BUTTON_LAYOUT layout,
                         FrontAction order[4]) {
  switch (layout) {
    case CrossPointSettings::LEFT_RIGHT_BACK_CONFIRM:
      order[0] = FrontAction::Left;
      order[1] = FrontAction::Right;
      order[2] = FrontAction::Back;
      order[3] = FrontAction::Confirm;
      break;
    case CrossPointSettings::LEFT_BACK_CONFIRM_RIGHT:
      order[0] = FrontAction::Left;
      order[1] = FrontAction::Back;
      order[2] = FrontAction::Confirm;
      order[3] = FrontAction::Right;
      break;
    case CrossPointSettings::BACK_CONFIRM_LEFT_RIGHT:
    default:
      order[0] = FrontAction::Back;
      order[1] = FrontAction::Confirm;
      order[2] = FrontAction::Left;
      order[3] = FrontAction::Right;
      break;
  }
}

int findFrontActionIndex(const FrontAction action, const FrontAction order[4]) {
  for (int i = 0; i < 4; ++i) {
    if (order[i] == action) {
      return i;
    }
  }
  return -1;
}

FrontAction toFrontAction(const MappedInputManager::Button button) {
  switch (button) {
    case MappedInputManager::Button::Back:
      return FrontAction::Back;
    case MappedInputManager::Button::Confirm:
      return FrontAction::Confirm;
    case MappedInputManager::Button::Left:
      return FrontAction::Left;
    case MappedInputManager::Button::Right:
      return FrontAction::Right;
    default:
      return FrontAction::Back;
  }
}

InputBtn mapFrontButton(const MappedInputManager::Button button,
                        const CrossPointSettings::FRONT_BUTTON_LAYOUT layout,
                        const bool reverseOrder) {
  FrontAction actionOrder[4];
  getFrontActionOrder(layout, actionOrder);
  const int actionIndex = findFrontActionIndex(toFrontAction(button), actionOrder);
  if (actionIndex < 0) {
    return InputManager::BTN_BACK;
  }
  const InputBtn physicalOrder[4] = {InputManager::BTN_BACK, InputManager::BTN_CONFIRM,
                                     InputManager::BTN_LEFT, InputManager::BTN_RIGHT};
  const int physicalIndex = reverseOrder ? 3 - actionIndex : actionIndex;
  return physicalOrder[physicalIndex];
}
}  // namespace

bool MappedInputManager::shouldReverseFrontRow() const {
  const auto orientation =
      static_cast<CrossPointSettings::ORIENTATION>(SETTINGS.orientation);
  return orientation == CrossPointSettings::INVERTED;
}

bool MappedInputManager::shouldSwapLeftRight() const {
  const auto orientation =
      static_cast<CrossPointSettings::ORIENTATION>(SETTINGS.orientation);
  if (!readingOrientationActive) {
    return false;
  }
  return orientation == CrossPointSettings::LANDSCAPE_CCW;
}

bool MappedInputManager::shouldSwapSideButtons() const {
  const auto orientation =
      static_cast<CrossPointSettings::ORIENTATION>(SETTINGS.orientation);
  if (orientation == CrossPointSettings::INVERTED) {
    return true;
  }
  if (!readingOrientationActive) {
    return false;
  }
  return orientation == CrossPointSettings::LANDSCAPE_CW;
}

decltype(InputManager::BTN_BACK) MappedInputManager::mapButton(const Button button) const {
  const auto frontLayout = static_cast<CrossPointSettings::FRONT_BUTTON_LAYOUT>(SETTINGS.frontButtonLayout);
  const auto sideLayout = static_cast<CrossPointSettings::SIDE_BUTTON_LAYOUT>(SETTINGS.sideButtonLayout);
  const bool reverseFrontRow = shouldReverseFrontRow();
  const bool swapLeftRight = shouldSwapLeftRight();
  const bool swapSideButtons = shouldSwapSideButtons();

  switch (button) {
    case Button::Back:
      return mapFrontButton(Button::Back, frontLayout, reverseFrontRow);
    case Button::Confirm:
      return mapFrontButton(Button::Confirm, frontLayout, reverseFrontRow);
    case Button::Left:
      return mapFrontButton(swapLeftRight ? Button::Right : Button::Left, frontLayout, reverseFrontRow);
    case Button::Right:
      return mapFrontButton(swapLeftRight ? Button::Left : Button::Right, frontLayout, reverseFrontRow);
    case Button::Up:
      return swapSideButtons ? InputManager::BTN_DOWN : InputManager::BTN_UP;
    case Button::Down:
      return swapSideButtons ? InputManager::BTN_UP : InputManager::BTN_DOWN;
    case Button::Power:
      return InputManager::BTN_POWER;
    case Button::PageBack: {
      InputBtn physical =
          (sideLayout == CrossPointSettings::NEXT_PREV) ? InputManager::BTN_DOWN : InputManager::BTN_UP;
      return swapSideButtons ? (physical == InputManager::BTN_UP ? InputManager::BTN_DOWN : InputManager::BTN_UP)
                             : physical;
    }
    case Button::PageForward: {
      InputBtn physical =
          (sideLayout == CrossPointSettings::NEXT_PREV) ? InputManager::BTN_UP : InputManager::BTN_DOWN;
      return swapSideButtons ? (physical == InputManager::BTN_UP ? InputManager::BTN_DOWN : InputManager::BTN_UP)
                             : physical;
    }
  }

  return InputManager::BTN_BACK;
}

bool MappedInputManager::wasPressed(const Button button) const { return inputManager.wasPressed(mapButton(button)); }

bool MappedInputManager::wasReleased(const Button button) const { return inputManager.wasReleased(mapButton(button)); }

bool MappedInputManager::isPressed(const Button button) const { return inputManager.isPressed(mapButton(button)); }

bool MappedInputManager::wasAnyPressed() const { return inputManager.wasAnyPressed(); }

bool MappedInputManager::wasAnyReleased() const { return inputManager.wasAnyReleased(); }

unsigned long MappedInputManager::getHeldTime() const { return inputManager.getHeldTime(); }

MappedInputManager::Labels MappedInputManager::mapLabels(const char* back, const char* confirm, const char* previous,
                                                         const char* next) const {
  const auto layout = static_cast<CrossPointSettings::FRONT_BUTTON_LAYOUT>(SETTINGS.frontButtonLayout);
  const bool swapLeftRight = shouldSwapLeftRight();
  FrontAction actionOrder[4];
  getFrontActionOrder(layout, actionOrder);

  const char* labelsByAction[4] = {back, confirm, previous, next};
  if (swapLeftRight) {
    labelsByAction[2] = next;
    labelsByAction[3] = previous;
  }
  const auto labelForAction = [&](const FrontAction action) -> const char* {
    switch (action) {
      case FrontAction::Back:
        return labelsByAction[0];
      case FrontAction::Confirm:
        return labelsByAction[1];
      case FrontAction::Left:
        return labelsByAction[2];
      case FrontAction::Right:
        return labelsByAction[3];
      default:
        return "";
    }
  };

  const char* ordered[4] = {
      labelForAction(actionOrder[0]),
      labelForAction(actionOrder[1]),
      labelForAction(actionOrder[2]),
      labelForAction(actionOrder[3]),
  };

  return {ordered[0], ordered[1], ordered[2], ordered[3]};
}

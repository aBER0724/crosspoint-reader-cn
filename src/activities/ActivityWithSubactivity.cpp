#include "ActivityWithSubactivity.h"

#include "OrientationHelper.h"

void ActivityWithSubactivity::exitActivity() {
  if (subActivity) {
    subActivity->onExit();
    subActivity.reset();
    // Re-apply orientation for the parent activity. This handles two cases:
    // 1. Returning from a landscape reader to a portrait UI activity.
    // 2. Returning from settings where the user changed orientation.
    OrientationHelper::applyOrientation(renderer, mappedInput, this);
  }
}

void ActivityWithSubactivity::enterNewActivity(Activity* activity) {
  subActivity.reset(activity);
  OrientationHelper::applyOrientation(renderer, mappedInput, subActivity.get());
  subActivity->onEnter();
}

void ActivityWithSubactivity::loop() {
  if (subActivity) {
    subActivity->loop();
  }
}

void ActivityWithSubactivity::onExit() {
  Activity::onExit();
  exitActivity();
}

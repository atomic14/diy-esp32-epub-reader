#pragma once

#include "Actions.h"

class ButtonControls
{
public:
  virtual bool did_wake_from_deep_sleep() = 0;
  virtual UIAction get_deep_sleep_action() = 0;
  virtual void setup_deep_sleep() = 0;
};
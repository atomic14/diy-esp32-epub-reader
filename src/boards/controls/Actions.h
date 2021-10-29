#pragma once

#include <functional>

typedef enum
{
  NONE,
  UP,
  DOWN,
  SELECT,
  LAST_INTERACTION
} UIAction;

typedef std::function<void(UIAction)> ActionCallback_t;

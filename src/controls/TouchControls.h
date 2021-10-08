#pragma once

#include "Actions.h"

class L58Touch;
class Renderer;
class TouchControls
{
public:
  L58Touch *ts;
  // Touch UI buttons (Top left and activated only when USE_TOUCH is defined)
  uint8_t ui_button_width = 120;
  uint8_t ui_button_height = 34;
  UIAction last_action = NONE;
  TouchControls(int width, int height, int rotation);
  void render(Renderer *renderer);
  void renderPressedState(Renderer *renderer, UIAction action, bool state = true);
  UIAction get_action(Renderer *renderer);
};
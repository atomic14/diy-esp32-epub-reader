#pragma once

#include "Actions.h"

class L58Touch;
class Renderer;
class TouchControls
{
public:
  // INTGPIO is touch interrupt, goes HI when it detects a touch, which coordinates are read by I2C
  L58Touch *ts;
  int eventX = 0;
  int eventY = 0;
  int lastX = 0;
  int lastY = 0;
  bool tapFlag = false;
  // Touch UI buttons (Top left and activated only when USE_TOUCH is defined)
  uint8_t ui_button_width = 120;
  uint8_t ui_button_height = 34;

  TouchControls(int width, int height, int rotation);
  void render(Renderer *renderer);
  UIAction get_action();
};
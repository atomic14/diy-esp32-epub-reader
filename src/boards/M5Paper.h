#pragma once

#include "Board.h"

class M5Paper : public Board
{
public:
  virtual void power_up();
  virtual void prepare_to_sleep();
  virtual Renderer *get_renderer();
  virtual void stop_filesystem();
};
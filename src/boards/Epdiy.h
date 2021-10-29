#pragma once

#include "Board.h"

class Epdiy : public Board
{
public:
  virtual void power_up();
  virtual void prepare_to_sleep();
  virtual Renderer *get_renderer();
};
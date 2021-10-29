#pragma once

class Battery
{
public:
  virtual void setup() = 0;
  virtual float get_voltage() = 0;
  virtual int get_percentage() = 0;
};
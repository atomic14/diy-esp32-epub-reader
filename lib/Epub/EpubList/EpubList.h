#pragma once

#include <vector>

#include "./State.h"

class Epub;
class Renderer;

class EpubList
{
private:
  Renderer *renderer;
  EpubListState &state;
  bool m_needs_redraw = false;

public:
  EpubList(Renderer *renderer, EpubListState &state) : renderer(renderer), state(state){};
  ~EpubList() {}
  bool load(const char *path);
  void set_needs_redraw() { m_needs_redraw = true; }
  void next();
  void prev();
  void render();
};
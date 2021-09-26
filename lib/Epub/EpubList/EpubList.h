#pragma once

#include <vector>

class Epub;
class Renderer;

typedef struct
{
  int previous_rendered_page = -1;
  int previous_selected_item = -1;
  int selected_item = 0;
} EpubListState;

class EpubList
{
private:
  std::vector<Epub *> epubs;
  EpubListState &state;
  Renderer *renderer;

public:
  EpubList(EpubListState &state, Renderer *renderer) : state(state), renderer(renderer) {}
  bool load(const char *path);
  void next();
  void prev();
  const char *get_current_epub_path();
  void render();
};
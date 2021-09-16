#pragma once

#include <vector>

class Epub;
class Renderer;

class EpubList
{
private:
  std::vector<Epub *> epubs;
  int last_rendered_page = -1;
  int last_selected_item = -1;

public:
  bool load(char *path);
  int get_num_epubs()
  {
    return epubs.size();
  }
  void render(int selected_item, Renderer *renderer);
};
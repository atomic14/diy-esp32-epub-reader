#pragma once

#include <vector>

class Epub;
class Renderer;

class EpubList
{
private:
  std::vector<Epub *> epubs;

public:
  bool load(char *path);
  int get_num_epubs()
  {
    return epubs.size();
  }
  void render(int selected_item, Renderer *renderer);
};
#pragma once

#include <vector>

class Epub;
class Renderer;

const int MAX_EPUB_LIST_SIZE = 100;
const int MAX_PATH_SIZE = 256;
const int MAX_TITLE_SIZE = 100;

// nice and simple state that can be persisted easily
typedef struct
{
  char path[MAX_PATH_SIZE];
  char title[MAX_TITLE_SIZE];
  char cover_path[MAX_PATH_SIZE];
} EpubListItem;

typedef struct
{
  int previous_rendered_page = -1;
  int previous_selected_item = -1;
  int selected_item = 0;
  int num_epubs = 0;
  bool is_loaded = false;
  EpubListItem epub_list[MAX_EPUB_LIST_SIZE];
} EpubListState;

class EpubList
{
private:
  EpubListState *state;
  Renderer *renderer;
  bool m_needs_redraw = false;

public:
  EpubList(Renderer *renderer) : renderer(renderer)
  {
    state = new EpubListState();
  }
  ~EpubList()
  {
    delete state;
  }
  bool load(const char *path);
  void set_needs_redraw() { m_needs_redraw = true; }
  bool hydrate();
  void dehydrate();
  void next();
  void prev();
  const char *get_current_epub_path();
  void render();
};
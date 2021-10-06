#pragma once

#include <vector>

class Epub;
class Renderer;

const int MAX_EPUB_LIST_SIZE = 20;
const int MAX_PATH_SIZE = 256;
const int MAX_TITLE_SIZE = 100;

// The epub list state is split between disk and RTC_DATA - we're limited on space in the RTC_DATA
// we store the list of books on disk and the current selection in RTC_DATA
// The reason for this split is that the list of books is too large to hold in RTC memory
// Unfortunately SPIFFS is proving to be very unreliable so we save the critical data in RTC_DATA.

// nice and simple state that can be persisted easily
typedef struct
{
  char path[MAX_PATH_SIZE];
  char title[MAX_TITLE_SIZE];
} EpubListItem;

// this is held in the RTC memory
typedef struct
{
  int previous_rendered_page;
  int previous_selected_item;
  int selected_item;
  int num_epubs;
  bool is_loaded;
  EpubListItem epub_list[MAX_EPUB_LIST_SIZE];
} EpubListState;

class EpubList
{
private:
  Renderer *renderer;
  EpubListState *state;
  bool m_needs_redraw = false;

public:
  EpubList(Renderer *renderer, EpubListState *state) : renderer(renderer), state(state){};
  ~EpubList() {}
  bool load(const char *path);
  void set_needs_redraw() { m_needs_redraw = true; }
  void next();
  void prev();
  const char *get_current_epub_path();
  void render();
};
#pragma once

const int MAX_EPUB_LIST_SIZE = 20;
const int MAX_PATH_SIZE = 256;
const int MAX_TITLE_SIZE = 100;

// nice and simple state that can be persisted easily
typedef struct
{
  char path[MAX_PATH_SIZE];
  char title[MAX_TITLE_SIZE];
  uint16_t current_section;
  uint16_t current_page;
  uint16_t pages_in_current_section;
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
#ifndef UNIT_TEST
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#else
#define vTaskDelay(t)
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#endif
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <algorithm>
#include "EpubList.h"
#include "Epub.h"
#include "Renderer/Renderer.h"

static const char *TAG = "PUBLIST";

#define PADDING 20
#define EPUBS_PER_PAGE 5

void EpubList::next()
{
  state.selected_item = (state.selected_item + 1) % epubs.size();
}

void EpubList::prev()
{
  state.selected_item = (state.selected_item - 1 + epubs.size()) % epubs.size();
}

const char *EpubList::get_current_epub_path()
{
  return epubs[state.selected_item]->get_path().c_str();
}

bool EpubList::load(const char *path)
{
  // list the file
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(path)) != NULL)
  {
    while ((ent = readdir(dir)) != NULL)
    {
      // vTaskDelay(1);
      // ignore any hidden files starting with "." and any directories
      if (ent->d_name[0] == '.' || ent->d_type == DT_DIR)
      {
        continue;
      }
      int name_length = strlen(ent->d_name);
      if (name_length < 5 || strcmp(ent->d_name + name_length - 5, ".epub") != 0)
      {
        continue;
      }
      ESP_LOGI(TAG, "Loading epub %s", ent->d_name);
      Epub *epub = new Epub(std::string("/sdcard/") + ent->d_name);
      if (epub->load())
      {
        epubs.push_back(epub);
      }
      else
      {
        ESP_LOGE(TAG, "Failed to load epub %s", ent->d_name);
        delete (epub);
      }
    }
    closedir(dir);
    std::sort(std::begin(epubs),
              std::end(epubs),
              [](Epub *a, Epub *b)
              { return a->get_title() < b->get_title(); });
  }
  else
  {
    /* could not open directory */
    perror("");
    ESP_LOGE(TAG, "Could not open directory %s", path);
    return false;
  }
  // sanity check our state
  if (state.selected_item >= epubs.size())
  {
    state.selected_item = 0;
    state.previous_rendered_page = -1;
    state.previous_selected_item = -1;
  }
  return true;
}

void EpubList::render(Renderer *renderer)
{
  ESP_LOGI(TAG, "Rendering EPUB list");
  // what page are we on?
  int current_page = state.selected_item / EPUBS_PER_PAGE;
  // draw a page of epubs
  int cell_height = renderer->get_page_height() / EPUBS_PER_PAGE;
  ESP_LOGI(TAG, "Cell height is %d", cell_height);
  int start_index = current_page * EPUBS_PER_PAGE;
  int ypos = 0;
  // starting a fresh page or rendering from scratch?
  ESP_LOGI(TAG, "Current page is %d, previous page %d", current_page, state.previous_rendered_page);
  if (current_page != state.previous_rendered_page)
  {
    renderer->clear_screen();
    renderer->flush_display();
    state.previous_selected_item = -1;
  }
  for (int i = start_index; i < start_index + EPUBS_PER_PAGE && i < epubs.size(); i++)
  {
    // do we need to draw a new page of items?
    if (current_page != state.previous_rendered_page || state.previous_selected_item == i || state.selected_item == i)
    {
      ESP_LOGI(TAG, "Rendering item %d", i);
      // draw the cover page
      int image_xpos = PADDING;
      int image_ypos = ypos + PADDING;
      int image_height = cell_height - PADDING * 2;
      int image_width = 2 * image_height / 3;
      renderer->draw_image(epubs[i]->get_cover_image_filename(), image_xpos, image_ypos, image_width, image_height);
      // draw the title
      int text_xpos = image_xpos + image_width + PADDING;
      int text_ypos = ypos + PADDING;
      int text_width = renderer->get_page_width() - (text_xpos + PADDING);
      int text_height = cell_height - PADDING * 2;
      renderer->draw_text_box(epubs[i]->get_title(), text_xpos, text_ypos, text_width, text_height);
    }
    // clear the selection box around the previous selected item
    if (state.previous_selected_item == i)
    {
      for (int i = 0; i < 5; i++)
      {
        renderer->draw_rect(PADDING / 2 + i, ypos + PADDING / 2 + i, renderer->get_page_width() - PADDING - 2 * i, cell_height - PADDING - 2 * i, 255);
      }
    }
    // draw the selection box around the current selection
    if (state.selected_item == i)
    {
      for (int i = 0; i < 5; i++)
      {
        renderer->draw_rect(PADDING / 2 + i, ypos + PADDING / 2 + i, renderer->get_page_width() - PADDING - 2 * i, cell_height - PADDING - 2 * i, 0);
      }
    }
    ypos += cell_height;
  }
  state.previous_selected_item = state.selected_item;
  state.previous_rendered_page = current_page;
}
#include <esp_log.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "EpubList.h"
#include "Epub.h"
#include "Renderer/Renderer.h"

static const char *TAG = "PUBLIST";

#define PADDING 40
#define EPUBS_PER_PAGE 5

bool EpubList::load(char *path)
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
  return true;
}

void EpubList::render(int selected_item, Renderer *renderer)
{
  ESP_LOGI(TAG, "Rendering EPUB list");
  // what page are we on?
  int current_page = selected_item / EPUBS_PER_PAGE;
  // draw a page of epubs
  int cell_height = renderer->get_page_height() / EPUBS_PER_PAGE;
  ESP_LOGI(TAG, "Cell height is %d", cell_height);
  int start_index = current_page * EPUBS_PER_PAGE;
  int ypos = 0;
  if (current_page != last_rendered_page)
  {
    renderer->clear_screen();
    last_selected_item = selected_item;
  }
  for (int i = start_index; i < start_index + EPUBS_PER_PAGE && i < epubs.size(); i++)
  {
    ESP_LOGI(TAG, "Rendering EPUB list %d", i);
    if (current_page != last_rendered_page)
    {
      // draw the cover page
      int image_xpos = PADDING;
      int image_ypos = ypos + PADDING;
      int image_height = cell_height - PADDING * 2;
      int image_width = 2 * image_height / 3; // draw the cover with a 1:2 aspect ratio
      ESP_LOGI(TAG, "Draw image %s at %d,%d,%d,%d", epubs[i]->get_cover_image_filename().c_str(), image_xpos, image_ypos, image_width, image_height);
      renderer->draw_image(epubs[i]->get_cover_image_filename(), image_xpos, image_ypos, image_width, image_height);
      int text_xpos = image_xpos + image_width + PADDING;
      int text_ypos = ypos + PADDING;
      int text_width = renderer->get_page_width() - (text_xpos + PADDING);
      int text_height = cell_height - PADDING * 2;
      ESP_LOGI(TAG, "Draw text %s at %d,%d,%d,%d", epubs[i]->get_title().c_str(), text_xpos, text_ypos, text_width, text_height);

      renderer->draw_text_box(epubs[i]->get_title(), text_xpos, text_ypos, text_width, text_height);
    }
    if (last_selected_item == i)
    {
      renderer->draw_rect(PADDING / 2, ypos + PADDING / 2, renderer->get_page_width() - PADDING, cell_height - PADDING, 255);
    }
    if (selected_item == i)
    {
      renderer->draw_rect(PADDING / 2, ypos + PADDING / 2, renderer->get_page_width() - PADDING, cell_height - PADDING);
    }
    ypos += cell_height;
  }
  last_selected_item = selected_item;
  last_rendered_page = current_page;
}
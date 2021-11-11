#include "EpubIndex.h"
static const char *TAG = "PUBINDEX";

#define PADDING 20
#define EPUBS_PER_PAGE 5

void EpubIndex::next()
{
  //state->selected_item = (state->selected_item + 1) % state->num_epubs;
}

void EpubIndex::prev()
{
  // Pending implementation
}

bool EpubIndex::load()
{
  ESP_LOGI(TAG, "load");
  // do we need to load the epub?
  if (!epub || epub->get_path() != state.path)
  {
    renderer->show_busy();
    delete epub;
    
    epub = new Epub(state.path);
    if (epub->loadIndex())
    {
      ESP_LOGI(TAG, "Epub loaded");
      return false;
    }
  }
  return true;
}

void EpubIndex::render()
{
  ESP_LOGD(TAG, "Rendering EPUB index (Pending)");
}
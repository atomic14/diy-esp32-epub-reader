#include "EpubIndex.h"
static const char *TAG = "PUBINDEX";

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
      ESP_LOGI(TAG, "Epub index loaded");
      return false;
    }
  }
  render();
  return true;
}

void EpubIndex::render()
{
  ESP_LOGD(TAG, "Rendering EPUB index (Pending)");
  renderer->clear_screen();
  int toc_size = (epub->toc_index.size()) ? epub->toc_index.size() : 1;
  int cell_height = renderer->get_page_height() / toc_size;
  int text_ypos = 10;
  int y_offset = 20;
  int idx = 0;
  
  printf("TOC index size:%d\n", epub->toc_index.size());
  TextBlock *index_block = new TextBlock(LEFT_ALIGN);

  for(auto iter = epub->toc_index.begin(); iter != epub->toc_index.end(); ++iter){
    // Correctly printing each index item
    index_block->add_span(iter->first.c_str(), false, false);
    index_block->layout(renderer, epub, renderer->get_page_width());
    printf("%d %s\n", idx, iter->first.c_str());

    index_block->render(renderer, idx, 10, text_ypos + y_offset);
    y_offset += renderer->get_line_height();
    idx++;
    // Chris I don't understand why printing more than 2 it fails:
    if (idx == 2) break;
  }
  delete index_block;
  
}
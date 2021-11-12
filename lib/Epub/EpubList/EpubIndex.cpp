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

  printf("TOC index size:%u\n", epub->toc_index.size());

  for (auto iter = epub->toc_index.begin(); iter != epub->toc_index.end(); ++iter)
  {
    // temporary index block for text rendering
    TextBlock *index_block = new TextBlock(LEFT_ALIGN);
    // Correctly printing each index item
    index_block->add_span(iter->first.c_str(), false, false);
    index_block->layout(renderer, epub, renderer->get_page_width());
    // draw each line of the index block making sure we don't run over the cell
    for (int i = 0; i < index_block->line_breaks.size(); i++)
    {
      index_block->render(renderer, i, 0, y_offset);
      y_offset += renderer->get_line_height();
    }
    y_offset += renderer->get_line_height();
    // clean up the temporary index block
    delete index_block;
  }
}
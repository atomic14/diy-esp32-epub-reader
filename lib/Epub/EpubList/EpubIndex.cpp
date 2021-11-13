#include "EpubIndex.h"

static const char *TAG = "PUBINDEX";

void EpubIndex::next()
{
  selected_item = (selected_item + 1) % toc_count;
}

void EpubIndex::prev()
{
  selected_item = (selected_item - 1 + toc_count) % toc_count;
}

bool EpubIndex::load()
{
  ESP_LOGI(TAG, "load");
  
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
  ESP_LOGD(TAG, "Rendering EPUB index");
  renderer->clear_screen();
  int toc_size = (epub->toc_index.size()) ? epub->toc_index.size() : 1;
  int cell_height = renderer->get_page_height() / toc_size;
  int y_offset = 10;
  toc_count = epub->toc_index.size();
  int iter_count = 0;
  int ypos = 10;
  uint8_t xpos = 20;

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
      index_block->render(renderer, i, xpos, y_offset);
      y_offset += renderer->get_line_height();
    }
    y_offset += renderer->get_line_height();
    // clean up the temporary index block
    delete index_block;

    // clear the selection box around the previous selected item
    if (previous_selected_item == iter_count)
    {
      for (int i = 0; i < 3; i++) {
        renderer->draw_rect(i, ypos+i, renderer->get_page_width(), 50, 255);
      }
    }
    // draw the selection box around the current selection
    if (selected_item == iter_count)
    {
      for (int i = 0; i < 3; i++) {
        renderer->draw_rect(i, ypos+i, renderer->get_page_width(), 50, 0);
      }
    }
    ypos += cell_height*1.25;
    iter_count++;
  }
}
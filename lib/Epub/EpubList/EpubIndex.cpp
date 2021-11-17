#include "EpubIndex.h"

static const char *TAG = "PUBINDEX";
#define PADDING 20
#define ITEMS_PER_PAGE 5

void EpubIndex::next()
{
  // must be loaded as we need the information from the epub
  if (!epub)
  {
    load();
  }
  state.selected_item = (state.selected_item + 1) % epub->get_toc_items_count();
}

void EpubIndex::prev()
{
  // must be loaded as we need the information from the epub
  if (!epub)
  {
    load();
  }
  state.selected_item = (state.selected_item - 1 + epub->get_toc_items_count()) % epub->get_toc_items_count();
}

bool EpubIndex::load()
{
  ESP_LOGI(TAG, "load");

  if (!epub || epub->get_path() != selected_epub.path)
  {
    renderer->show_busy();
    delete epub;

    epub = new Epub(selected_epub.path);
    if (epub->load())
    {
      ESP_LOGI(TAG, "Epub index loaded");
      return false;
    }
  }
  return true;
}

// TODO - this is currently pretty much a copy of the epub list rendering
// we can fit a lot more on the screen by allowing variable cell heights
// and a lot of the optimisations that are used for the list aren't really
// required as we're not rendering thumbnails
void EpubIndex::render()
{
  ESP_LOGD(TAG, "Rendering EPUB index");
  // what page are we on?
  int current_page = state.selected_item / ITEMS_PER_PAGE;
  // show five items per page
  int cell_height = renderer->get_page_height() / ITEMS_PER_PAGE;
  int start_index = current_page * ITEMS_PER_PAGE;
  int ypos = 0;
  // starting a fresh page or rendering from scratch?
  ESP_LOGI(TAG, "Current page is %d, previous page %d, redraw=%d", current_page, state.previous_rendered_page, m_needs_redraw);
  if (current_page != state.previous_rendered_page || m_needs_redraw)
  {
    m_needs_redraw = false;
    renderer->show_busy();
    renderer->clear_screen();
    state.previous_selected_item = -1;
    // trigger a redraw of the items
    state.previous_rendered_page = -1;
  }
  for (int i = start_index; i < start_index + ITEMS_PER_PAGE && i < epub->get_toc_items_count(); i++)
  {
    // do we need to draw a new page of items?
    if (current_page != state.previous_rendered_page)
    {

      // format the text using a text block
      TextBlock *index_block = new TextBlock(LEFT_ALIGN);
      index_block->add_span(epub->get_toc_item(i).title.c_str(), false, false);
      index_block->layout(renderer, epub, renderer->get_page_width());
      // draw each line of the index block making sure we don't run over the cell
      int height = 0;
      for (int i = 0; i < index_block->line_breaks.size() && height < cell_height; i++)
      {
        index_block->render(renderer, i, 0, ypos + height);
        height += renderer->get_line_height();
      }
      // clean up the temporary index block
      delete index_block;
    }
    // clear the selection box around the previous selected item
    if (state.previous_selected_item == i)
    {
      for (int line = 0; line < 3; line++)
      {
        renderer->draw_rect(line, ypos + PADDING / 2 + line, renderer->get_page_width() - 2 * line, cell_height - PADDING - 2 * line, 255);
      }
    }
    // draw the selection box around the current selection
    if (state.selected_item == i)
    {
      for (int line = 0; line < 3; line++)
      {
        renderer->draw_rect(line, ypos + PADDING / 2 + line, renderer->get_page_width() - 2 * line, cell_height - PADDING - 2 * line, 0);
      }
    }
    ypos += cell_height;
  }
  state.previous_rendered_page = current_page;
}

uint16_t EpubIndex::get_selected_toc()
{
  return epub->get_spine_index_for_toc_index(state.selected_item);
}
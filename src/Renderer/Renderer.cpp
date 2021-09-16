#include "Renderer.h"
#include "JPEGHelper.h"
#include "PNGHelper.h"

Renderer::~Renderer()
{
  delete png_helper;
  delete jpeg_helper;
}

ImageHelper *Renderer::get_image_helper(const std::string &filename)
{
  if (filename.find(".jpg") != std::string::npos ||
      filename.find(".jpeg") != std::string::npos)
  {
    if (!jpeg_helper)
    {
      jpeg_helper = new JPEGHelper();
    }
    return jpeg_helper;
  }
  if (filename.find(".png") != std::string::npos)
  {
    if (!png_helper)
    {
      png_helper = new PNGHelper();
    }
    return png_helper;
  }
  return nullptr;
}

// helper function to get text from the src
void Renderer::get_text(const char *src, int start_index, int end_index)
{
  // make a copy of the string from the src to the temp buffer
  // TODO handle html entities in the string
  int index = 0;
  for (int i = start_index; i < end_index && index < MAX_WORD_LENGTH - 1; i++)
  {
    buffer[index++] = src[i];
  }
  buffer[index] = 0;
}

void Renderer::draw_image(const std::string &filename, int x, int y, int width, int height)
{
  ImageHelper *helper = get_image_helper(filename);
  if (!helper ||
      !helper->render(filename, this, x, y, width, height))
  {
    // fall back to drawing a rectangle placeholder
    draw_rect(x + 20, y + 20, width - 40, height - 40);
  }
}

bool Renderer::get_image_size(const std::string &filename, int *width, int *height)
{
  ImageHelper *helper = get_image_helper(filename);
  if (helper && helper->get_size(filename, width, height, get_page_width(), get_page_height()))
  {
    return true;
  }
  // just provide a dummy height and width so we can do a placeholder
  // for this unknown image typew
  *width = std::min(get_page_width(), get_page_height());
  *height = *width;
  return false;
}

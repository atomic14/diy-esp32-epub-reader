#include "Renderer.h"
#include "JPEGHelper.h"
#include "PNGHelper.h"
#include <esp_log.h>

Renderer::~Renderer()
{
  delete png_helper;
  delete jpeg_helper;
}

ImageHelper *Renderer::get_image_helper(const std::string &filename, const uint8_t *data, size_t data_size)
{
  if (filename.find(".jpg") != std::string::npos ||
      filename.find(".jpeg") != std::string::npos ||
      (data_size > 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF))
  {
    if (!jpeg_helper)
    {
      jpeg_helper = new JPEGHelper();
    }
    return jpeg_helper;
  }
  if ((filename.find(".png") != std::string::npos) || (data_size > 4 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G'))
  {
    if (!png_helper)
    {
      png_helper = new PNGHelper();
    }
    return png_helper;
  }
  return nullptr;
}

void Renderer::draw_image(const std::string &filename, const uint8_t *data, size_t data_size, int x, int y, int width, int height)
{
  ImageHelper *helper = get_image_helper(filename, data, data_size);
  if (!helper ||
      !helper->render(data, data_size, this, x, y, width, height))
  {
    // fall back to drawing a rectangle placeholder
    draw_rect(x + 20, y + 20, width - 40, height - 40);
  }
}

bool Renderer::get_image_size(const std::string &filename, const uint8_t *data, size_t data_size, int *width, int *height)
{
  ImageHelper *helper = get_image_helper(filename, data, data_size);
  if (helper && helper->get_size(data, data_size, width, height))
  {
    return true;
  }
  // just provide a dummy height and width so we can do a placeholder
  // for this unknown image typew
  *width = std::min(get_page_width(), get_page_height());
  *height = *width;
  return false;
}

void Renderer::draw_text_box(const std::string &text, int x, int y, int width, int height, bool bold, bool italic)
{
  int length = text.length();
  // fit the text into the box
  int start = 0;
  int end = 1;
  int ypos = 0;
  while (start < length && ypos + get_line_height() < height)
  {
    while (end < length && get_text_width(text.substr(start, end - start).c_str(), bold, italic) < width)
    {
      end++;
    }
    if (get_text_width(text.substr(start, end - start).c_str(), bold, italic) > width)
    {
      end--;
    }
    draw_text(x, y + ypos, text.substr(start, end - start).c_str(), bold, italic);
    ypos += get_line_height();
    start = end;
    end = start + 1;
  }
}

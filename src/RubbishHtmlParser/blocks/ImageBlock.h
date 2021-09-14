#pragma once
#include "Block.h"
#include "../../Renderer/JPEGHelper.h"

class ImageBlock : public Block
{
private:
  JPEGHelper *m_helper = NULL;
  std::string m_src;

public:
  int y_pos;
  int x_pos;
  int width;
  int height;
  int scale_factor;

  ImageBlock(std::string src) : m_src(src)
  {
    if (src.find(".jpg") != std::string::npos ||
        src.find(".jpeg") != std::string::npos)
    {
      m_helper = new JPEGHelper("/sdcard/" + src);
    }
  }
  void layout(const char *html, Renderer *renderer)
  {
    if (
        !m_helper ||
        !m_helper->get_size(&width, &height))
    {
      // if we don't have a helper or the helper fails then
      // fall back to a default size
      width = renderer->get_page_width();
      height = renderer->get_page_width();
    }
    // fit the image onto the page
    // we use very simple scaling to fit in
    // with the jpeg decoder
    scale_factor = 0;
    while (scale_factor < 3 && (width > renderer->get_page_width() || height > renderer->get_page_height()))
    {
      width /= 2;
      height /= 2;
      scale_factor++;
    }
    // horizontal center
    x_pos = (renderer->get_page_width() - width) / 2;
  }
  void render(const char *html, Renderer *renderer, int y_pos)
  {
    if (
        !m_helper ||
        !m_helper->render(renderer, x_pos, y_pos, width, height, scale_factor))
    {
      // fall back to drawing a rectangle placeholder
      renderer->draw_rect(20, y_pos + 20, width - 40, height - 40);
    }
  }
  virtual void dump(const char *html)
  {
    printf("ImageBlock: %s\n", m_src.c_str());
  }
  virtual BlockType getType()
  {
    return IMAGE_BLOCK;
  }
};

#pragma once
#include "../../Renderer/Renderer.h"
#include "Block.h"
#include "../../Renderer/JPEGHelper.h"
#include "../../Renderer/PNGHelper.h"

class ImageBlock : public Block
{
private:
  ImageHelper *m_helper = NULL;
  std::string m_src;
  // this will get filled in when the block is layed out
  std::string m_image_path;

public:
  int y_pos;
  int x_pos;
  int width;
  int height;

  ImageBlock(std::string src) : m_src(src)
  {
    if (src.find(".jpg") != std::string::npos ||
        src.find(".jpeg") != std::string::npos)
    {
      m_helper = new JPEGHelper();
    }
    if (src.find(".png") != std::string::npos)
    {
      m_helper = new PNGHelper();
    }
  }
  void layout(const char *html, Renderer *renderer, Epub *epub)
  {
    m_image_path = epub->get_image_path(m_src);
    if (
        !m_helper ||
        !m_helper->get_size(m_image_path, &width, &height, renderer->get_page_width(), renderer->get_page_height()))
    {
      // if we don't have a helper or the helper fails then
      // fall back to a default size
      width = renderer->get_page_width();
      height = renderer->get_page_width();
    }
    // horizontal center
    x_pos = (renderer->get_page_width() - width) / 2;
  }
  void render(const char *html, Renderer *renderer, int y_pos)
  {
    if (m_image_path.empty())
    {
      ESP_LOGE("ImageBlock", "Image path is empty - have you called layout?");
      return;
    }
    if (
        !m_helper ||
        !m_helper->render(m_image_path, renderer, x_pos, y_pos, width, height))
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

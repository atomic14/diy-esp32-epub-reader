#pragma once
#include "../../Renderer/Renderer.h"
#include "Block.h"
#include "../../EpubList/Epub.h"
#ifndef UNIT_TEST
#include <esp_log.h>
#else
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#endif

class ImageBlock : public Block
{
private:
  // the src attribute from the image element
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
  }
  virtual bool isEmpty()
  {
    return m_src.empty();
  }
  void layout(Renderer *renderer, Epub *epub)
  {
    m_image_path = epub->get_image_path(m_src);
    renderer->get_image_size(m_image_path, &width, &height);
    if (width > renderer->get_page_width() || height > renderer->get_page_height())
    {
      float scale = std::min(
          float(renderer->get_page_width()) / float(width),
          float(renderer->get_page_height()) / float(height));
      width *= scale;
      height *= scale;
    }
    // horizontal center
    x_pos = (renderer->get_page_width() - width) / 2;
  }
  void render(Renderer *renderer, int y_pos)
  {
    if (m_image_path.empty())
    {
      ESP_LOGE("ImageBlock", "Image path is empty - have you called layout?");
      return;
    }
    renderer->draw_image(m_image_path, x_pos, y_pos, width, height);
  }
  virtual void dump()
  {
    printf("ImageBlock: %s\n", m_src.c_str());
  }
  virtual BlockType getType()
  {
    return IMAGE_BLOCK;
  }
};

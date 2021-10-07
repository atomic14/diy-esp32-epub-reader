#pragma once
#include "../../Renderer/Renderer.h"
#include "Block.h"
#include "../../EpubList/Epub.h"
#ifndef UNIT_TEST
#include <esp_log.h>
#else
#define ESP_LOGI(args...)
#define ESP_LOGE(args...)
#define ESP_LOGD(args...)
#define ESP_LOGW(args...)
#endif

class ImageBlock : public Block
{
public:
  // the src attribute from the image element
  std::string m_src;
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
  void layout(Renderer *renderer, Epub *epub, int max_width = -1)
  {
    size_t image_data_size = 0;
    uint8_t *image_data = epub->get_item_contents(m_src, &image_data_size);
    renderer->get_image_size(m_src, image_data, image_data_size, &width, &height);
    if (width > renderer->get_page_width() || height > renderer->get_page_height())
    {
      float scale = std::min(
          float(max_width != -1 ? max_width : renderer->get_page_width()) / float(width),
          float(renderer->get_page_height()) / float(height));
      width *= scale;
      height *= scale;
    }
    // horizontal center
    x_pos = (renderer->get_page_width() - width) / 2;
    free(image_data);
  }
  void render(Renderer *renderer, Epub *epub, int y_pos)
  {
    size_t image_data_size = 0;
    uint8_t *image_data = epub->get_item_contents(m_src, &image_data_size);
    renderer->draw_image(m_src, image_data, image_data_size, x_pos, y_pos, width, height);
    free(image_data);
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

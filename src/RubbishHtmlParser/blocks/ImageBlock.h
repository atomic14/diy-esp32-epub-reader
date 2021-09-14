#pragma once
#include <tjpgd.h>
#include "Block.h"

// size_t in_func(
//     JDEC *jdec,    /* Pointer to the decompression object */
//     uint8_t *buff, /* Pointer to buffer to store the read data */
//     size_t ndata   /* Number of bytes to read/remove */
// )
// {
// }

class ImageBlock : public Block
{
private:
  int m_src_start;
  int m_src_end;
  float scale = 1.0;

public:
  int width;
  int height;

  ImageBlock(int src_start, int src_end) : m_src_start(src_start), m_src_end(src_end) {}
  void layout(const char *html, Renderer *renderer)
  {
    // // is it a jpeg image?
    // if ((
    //         strncmp(html + m_src_end - 3, "jpg", 3) == 0) ||
    //     strncmp(html + m_src_end - 4, "jpeg", 4) == 0)
    // {
    //   // decode the jpeg and get its size
    //   JDEC dec;
    //   JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
    //   if (res == JDR_OK)
    //   {
    //     ESP_LOGI("IMG", "JPEG Decoded - size %d,%d", dec.width, dec.height);
    //     width = dec.width;
    //     height = dec.height;
    //   }
    //   else
    //   {
    //     ESP_LOGE("IMG", "JPEG Decode failed - %d", res);
    //     width = renderer->get_page_width();
    //     height = renderer->get_page_width();
    //   }
    // }
    // else
    {
      width = renderer->get_page_width();
      height = renderer->get_page_width();
    }
    // scale to the width of the page
    scale = (float)width / (float)renderer->get_page_width();
    width *= scale;
    height *= scale;
  }
  void render(const char *html, Renderer *renderer, int y_pos)
  {
    renderer->draw_rect(20, y_pos + 20, width - 40, height - 40);
    renderer->draw_text(20, y_pos + 20, html, m_src_start, m_src_end, false, false);
  }
  virtual void dump(const char *html)
  {
    printf("ImageBlock: %.*s\n", m_src_end - m_src_start, html + m_src_start);
  }
  virtual BlockType getType()
  {
    return IMAGE_BLOCK;
  }
};

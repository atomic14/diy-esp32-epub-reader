#pragma once

#include "Block.h"

class ImageBlock : public Block
{
private:
  int m_src_start;
  int m_src_end;

public:
  int width;
  int height;

  ImageBlock(int src_start, int src_end) : m_src_start(src_start), m_src_end(src_end) {}
  void layout(const char *html, Renderer *renderer)
  {
    // TODO: fetch the image size
    width = 100;
    height = 100;
  }
  void render(const char *html, Renderer *renderer, int y_pos)
  {
    renderer->draw_text(0, y_pos, html, m_src_start, m_src_end, false, false);
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

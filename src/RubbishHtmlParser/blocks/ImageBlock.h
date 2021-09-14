#pragma once
#include <tjpgd.h>
#include "Block.h"

#define POOL_SIZE 32768

size_t read_jpeg_data(
    JDEC *jdec,    /* Pointer to the decompression object */
    uint8_t *buff, /* Pointer to buffer to store the read data */
    size_t ndata   /* Number of bytes to read/remove */
);

int draw_jpeg_function(
    JDEC *jdec,   /* Pointer to the decompression object */
    void *bitmap, /* Bitmap to be output */
    JRECT *rect   /* Rectangular region to output */
);

class ImageBlock : public Block
{
private:
  int m_src_start;
  int m_src_end;

public:
  int y_pos;
  int x_pos;
  int width;
  int height;
  int scale_factor;

  // temporary vars used for the JPEG callbacks
  FILE *fp = NULL;
  Renderer *renderer = NULL;

  ImageBlock(int src_start, int src_end) : m_src_start(src_start), m_src_end(src_end)
  {
  }
  void layout(const char *html, Renderer *renderer)
  {
    // is it a jpeg image?
    if (
        strncmp(html + m_src_end - 3, "jpg", 3) == 0 ||
        strncmp(html + m_src_end - 4, "jpeg", 4) == 0)
    {
      ESP_LOGI("IMG", "Getting size of %.*s", m_src_end - m_src_start, html + m_src_start);
      void *pool = malloc(POOL_SIZE);
      if (!pool)
      {
        ESP_LOGE("IMG", "Failed to allocate memory for pool");
        width = renderer->get_page_width();
        height = renderer->get_page_width();
        return;
      }
      std::string file_name = std::string("/sdcard/") + std::string(html + m_src_start, m_src_end - m_src_start);
      fp = fopen(file_name.c_str(), "rb");
      if (!fp)
      {
        ESP_LOGE("IMG", "File not found: %s", file_name.c_str());
        width = renderer->get_page_width();
        height = renderer->get_page_width();
        return;
      }
      // decode the jpeg and get its size
      JDEC dec;
      JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
      if (res == JDR_OK)
      {
        ESP_LOGI("IMG", "JPEG Decoded - size %d,%d", dec.width, dec.height);
        width = dec.width;
        height = dec.height;
      }
      else
      {
        ESP_LOGE("IMG", "JPEG Decode failed - %d", res);
        width = renderer->get_page_width();
        height = renderer->get_page_width();
      }
      free(pool);
      fclose(fp);
      fp = NULL;
    }
    else
    {
      width = renderer->get_page_width();
      height = renderer->get_page_width();
    }
    // fit the image onto the page
    scale_factor = 0;
    while (scale_factor < 3 && (width > renderer->get_page_width() || height > renderer->get_page_height()))
    {
      width /= 2;
      height /= 2;
      scale_factor++;
    }
    x_pos = (renderer->get_page_width() - width) / 2;
  }
  void render(const char *html, Renderer *renderer, int y_pos)
  {
    this->renderer = renderer;
    this->y_pos = y_pos;
    if (
        strncmp(html + m_src_end - 3, "jpg", 3) == 0 ||
        strncmp(html + m_src_end - 4, "jpeg", 4) == 0)
    {
      ESP_LOGI("IMG", "Drawing %.*s", m_src_end - m_src_start, html + m_src_start);
      void *pool = malloc(POOL_SIZE);
      if (!pool)
      {
        ESP_LOGE("IMG", "Failed to allocate memory for pool");
        return;
      }
      std::string file_name = std::string("/sdcard/") + std::string(html + m_src_start, m_src_end - m_src_start);
      fp = fopen(file_name.c_str(), "rb");
      if (!fp)
      {
        ESP_LOGE("IMG", "File not found: %s", file_name.c_str());
        return;
      }
      // decode the jpeg and get its size
      JDEC dec;
      JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
      if (res == JDR_OK)
      {
        ESP_LOGI("IMG", "JPEG Decoded - size %d,%d", dec.width, dec.height);
        jd_decomp(&dec, draw_jpeg_function, scale_factor);
      }
      else
      {
        ESP_LOGE("IMG", "JPEG Decode failed - %d", res);
      }
      free(pool);
      fclose(fp);
      fp = NULL;
    }
    else
    {
      renderer->draw_rect(20, y_pos + 20, width - 40, height - 40);
    }
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

size_t read_jpeg_data(
    JDEC *jdec,    /* Pointer to the decompression object */
    uint8_t *buff, /* Pointer to buffer to store the read data */
    size_t ndata   /* Number of bytes to read/remove */
)
{
  ImageBlock *block = (ImageBlock *)jdec->device;
  FILE *fp = block->fp;
  if (!fp)
  {
    ESP_LOGE("IMG", "File is not open");
    return 0;
  }
  if (buff)
  {
    fread(buff, 1, ndata, fp);
  }
  else
  {
    fseek(fp, ndata, SEEK_CUR);
  }
  vTaskDelay(1);
  return ndata;
}

int draw_jpeg_function(
    JDEC *jdec,   /* Pointer to the decompression object */
    void *bitmap, /* Bitmap to be output */
    JRECT *rect   /* Rectangular region to output */
)
{
  ImageBlock *block = (ImageBlock *)jdec->device;
  Renderer *renderer = (Renderer *)block->renderer;
  uint8_t *grey = (uint8_t *)bitmap;
  for (int y = rect->top; y <= rect->bottom; y++)
  {
    for (int x = rect->left; x <= rect->right; x++)
    {
      renderer->draw_pixel(x + block->x_pos, y + block->y_pos, *grey);
      grey++;
    }
  }
  return 1;
}

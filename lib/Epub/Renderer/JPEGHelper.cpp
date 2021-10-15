
#ifndef UNIT_TEST
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#else
#define vTaskDelay(t)
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#endif
#include "JPEGHelper.h"
#include "Renderer.h"

static const char *TAG = "JPG";

#define POOL_SIZE 32768

bool JPEGHelper::get_size(const uint8_t *data, size_t data_size, int *width, int *height)
{
  void *pool = malloc(POOL_SIZE);
  if (!pool)
  {
    ESP_LOGE(TAG, "Failed to allocate memory for pool");
    return false;
  }
  m_data = data;
  m_data_pos = 0;
  // decode the jpeg and get its size
  JDEC dec;
  JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
  if (res == JDR_OK)
  {
    ESP_LOGI(TAG, "JPEG Decoded - size %d,%d", dec.width, dec.height);
    *width = dec.width;
    *height = dec.height;
  }
  else
  {
    ESP_LOGE(TAG, "JPEG Decode failed - %d", res);
    return false;
  }
  free(pool);
  m_data = nullptr;
  m_data_pos = 0;
  return true;
}
bool JPEGHelper::render(const uint8_t *data, size_t data_size, Renderer *renderer, int x_pos, int y_pos, int width, int height)
{
  this->renderer = renderer;
  this->y_pos = y_pos;
  this->x_pos = x_pos;
  void *pool = malloc(POOL_SIZE);
  if (!pool)
  {
    ESP_LOGE(TAG, "Failed to allocate memory for pool");
    return false;
  }
  m_data = data;
  m_data_pos = 0;
  // decode the jpeg and get its size
  JDEC dec;
  JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
  if (res == JDR_OK)
  {
    this->x_scale = std::min(1.0f, float(width) / float(dec.width));
    this->y_scale = std::min(1.0f, float(height) / float(dec.height));

    scale_factor = 0;
    // this doesn't seem to work very well...
    while (x_scale <= 1.0f && y_scale <= 1.0f && scale_factor <= 3)
    {
      scale_factor++;
      x_scale *= 2;
      y_scale *= 2;
    }
    scale_factor--;
    x_scale /= 2;
    y_scale /= 2;

    ESP_LOGI(TAG, "JPEG Decoded - size %d,%d, scale = %f, %f, %d", dec.width, dec.height, x_scale, y_scale, scale_factor);
    jd_decomp(&dec, draw_jpeg_function, scale_factor);
  }
  else
  {
    ESP_LOGE(TAG, "JPEG Decode failed - %d", res);
  }
  free(pool);
  m_data = nullptr;
  m_data_pos = 0;
  return res == JDR_OK;
}

size_t read_jpeg_data(
    JDEC *jdec,    /* Pointer to the decompression object */
    uint8_t *buff, /* Pointer to buffer to store the read data */
    size_t ndata   /* Number of bytes to read/remove */
)
{
  JPEGHelper *context = (JPEGHelper *)jdec->device;
  if (context->m_data == nullptr)
  {
    ESP_LOGE(TAG, "No image data");
    return 0;
  }
  if (buff)
  {
    memcpy(buff, context->m_data + context->m_data_pos, ndata);
  }
  context->m_data_pos += ndata;
  return ndata;
}

static int last_y = 0;

// this is not a very efficient way of doing this - could be improved considerably
int draw_jpeg_function(
    JDEC *jdec,   /* Pointer to the decompression object */
    void *bitmap, /* Bitmap to be output */
    JRECT *rect   /* Rectangular region to output */
)
{
  JPEGHelper *context = (JPEGHelper *)jdec->device;
  Renderer *renderer = (Renderer *)context->renderer;
  uint8_t *rgb = (uint8_t *)bitmap;
  // this is a bit of dirty hack to only delay every line to feed the watchdog
  if (rect->top != last_y)
  {
    last_y = rect->top;
    vTaskDelay(1);
  }

  for (int y = rect->top; y <= rect->bottom; y++)
  {
    for (int x = rect->left; x <= rect->right; x++)
    {
      uint8_t r = *rgb++;
      uint8_t g = *rgb++;
      uint8_t b = *rgb++;
      uint32_t gray = (r * 38 + g * 75 + b * 15) >> 7;
      renderer->draw_pixel(context->x_pos + x * context->x_scale, context->y_pos + y * context->y_scale, gray);
    }
  }
  return 1;
}

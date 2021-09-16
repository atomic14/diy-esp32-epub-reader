
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "JPEGHelper.h"
#include "Renderer.h"
#include <esp_log.h>

static const char *TAG = "JPG";

#define POOL_SIZE 32768

bool JPEGHelper::get_size(const std::string &path, int *width, int *height)
{
  ESP_LOGI(TAG, "Getting size of %s", path.c_str());
  void *pool = malloc(POOL_SIZE);
  if (!pool)
  {
    ESP_LOGE(TAG, "Failed to allocate memory for pool");
    return false;
  }
  fp = fopen(path.c_str(), "rb");
  if (!fp)
  {
    ESP_LOGE(TAG, "File not found: %s", path.c_str());
    return false;
  }
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
  fclose(fp);
  fp = NULL;
  return true;
}
bool JPEGHelper::render(const std::string &path, Renderer *renderer, int x_pos, int y_pos, int width, int height)
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
  fp = fopen(path.c_str(), "rb");
  if (!fp)
  {
    ESP_LOGE(TAG, "File not found: %s", path.c_str());
    free(pool);
    return false;
  }
  // decode the jpeg and get its size
  JDEC dec;
  JRESULT res = jd_prepare(&dec, read_jpeg_data, pool, POOL_SIZE, this);
  if (res == JDR_OK)
  {
    this->x_scale = std::min(1.0f, float(width) / float(dec.width));
    this->y_scale = std::min(1.0f, float(height) / float(dec.height));

    this->scale_factor = 0;
    while (x_scale <= 1.0f && y_scale <= 1.0f && scale_factor <= 3)
    {
      this->scale_factor++;
      x_scale *= 2;
      y_scale *= 2;
    }
    scale_factor--;
    x_scale /= 2;
    y_scale /= 2;

    ESP_LOGI(TAG, "JPEG Decoded - size %d,%d, scale = %f, %f", dec.width, dec.height, x_scale, y_scale);
    jd_decomp(&dec, draw_jpeg_function, scale_factor);
  }
  else
  {
    ESP_LOGE(TAG, "JPEG Decode failed - %d", res);
  }
  free(pool);
  fclose(fp);
  fp = NULL;
  return res == JDR_OK;
}

size_t read_jpeg_data(
    JDEC *jdec,    /* Pointer to the decompression object */
    uint8_t *buff, /* Pointer to buffer to store the read data */
    size_t ndata   /* Number of bytes to read/remove */
)
{
  JPEGHelper *context = (JPEGHelper *)jdec->device;
  FILE *fp = context->fp;
  if (!fp)
  {
    ESP_LOGE(TAG, "File is not open");
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
  return ndata;
}

int draw_jpeg_function(
    JDEC *jdec,   /* Pointer to the decompression object */
    void *bitmap, /* Bitmap to be output */
    JRECT *rect   /* Rectangular region to output */
)
{
  JPEGHelper *context = (JPEGHelper *)jdec->device;
  Renderer *renderer = (Renderer *)context->renderer;
  uint8_t *grey = (uint8_t *)bitmap;
  for (int y = rect->top; y <= rect->bottom; y++)
  {
    for (int x = rect->left; x <= rect->right; x++)
    {
      renderer->draw_pixel(context->x_pos + x * context->x_scale, context->y_pos + y * context->y_scale, *grey);
      grey++;
    }
  }
  return 1;
}

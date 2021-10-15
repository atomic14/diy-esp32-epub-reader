#ifndef UNIT_TEST
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#define vTaskDelay(t)
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#endif
#include <PNGdec.h>
#include "PNGHelper.h"
#include "Renderer.h"

static const char *TAG = "PNG";

void convert_rgb_565_to_rgb(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b)
{
  *r = ((rgb565 >> 11) & 0x1F) << 3;
  *g = ((rgb565 >> 5) & 0x3F) << 2;
  *b = (rgb565 & 0x1F) << 3;
}

bool PNGHelper::get_size(const uint8_t *data, size_t data_size, int *width, int *height)
{
  int rc = png.openRAM(const_cast<uint8_t *>(data), data_size, NULL);
  if (rc == PNG_SUCCESS)
  {
    ESP_LOGI(TAG, "image specs: (%d x %d), %d bpp, pixel type: %d", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
    *width = png.getWidth();
    *height = png.getHeight();
    return true;
    png.close();
  }
  else
  {
    ESP_LOGE(TAG, "failed to open png %d", rc);
    return false;
  }
}
bool PNGHelper::render(const uint8_t *data, size_t data_size, Renderer *renderer, int x_pos, int y_pos, int width, int height)
{
  this->renderer = renderer;
  this->y_pos = y_pos;
  this->x_pos = x_pos;
  int rc = png.openRAM(const_cast<uint8_t *>(data), data_size, png_draw_callback);
  if (rc == PNG_SUCCESS)
  {
    this->x_scale = std::min(1.0f, float(width) / float(png.getWidth()));
    this->y_scale = std::min(1.0f, float(height) / float(png.getHeight()));
    this->last_y = -1;
    this->tmp_rgb565_buffer = (uint16_t *)malloc(png.getWidth() * 2);

    png.decode(this, PNG_FAST_PALETTE);
    png.close();
    free(this->tmp_rgb565_buffer);
    return true;
  }
  else
  {
    ESP_LOGE(TAG, "failed to parse png %d", rc);
    return false;
  }
}
void PNGHelper::draw_callback(PNGDRAW *draw)
{
  // work out where we should be drawing this line
  int y = y_pos + draw->y * y_scale;
  // only bother to draw if we haven't already drawn to this destination line
  if (y != last_y)
  {
    // feed the watchdog
    vTaskDelay(1);
    // get the rgb 565 pixel values
    png.getLineAsRGB565(draw, tmp_rgb565_buffer, 0, 0);
    for (int x = 0; x < png.getWidth() * x_scale; x++)
    {
      uint8_t r, g, b;
      convert_rgb_565_to_rgb(tmp_rgb565_buffer[int(x / x_scale)], &r, &g, &b);
      uint32_t gray = (r * 38 + g * 75 + b * 15) >> 7;
      renderer->draw_pixel(x_pos + x, y, gray);
    }
    last_y = y;
  }
};

void png_draw_callback(PNGDRAW *draw)
{
  PNGHelper *helper = (PNGHelper *)draw->pUser;
  helper->draw_callback(draw);
}

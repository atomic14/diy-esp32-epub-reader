#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "SPIFFS.h"

static const char *TAG = "SPIFFS";

#define SPI_DMA_CHAN 1

SPIFFS::SPIFFS(const char *mount_point)
{
  m_mount_point = mount_point;

  esp_vfs_spiffs_conf_t mount_cofig = {
      .base_path = mount_point,
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};

  ESP_LOGI(TAG, "Initializing SPIFFS");

  esp_err_t ret = esp_vfs_spiffs_register(&mount_cofig);

  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize SPIFFS");
    return;
  }
}

SPIFFS::~SPIFFS()
{
  esp_vfs_spiffs_unregister(NULL);
  ESP_LOGI(TAG, "SPIFFS unmounted");
}
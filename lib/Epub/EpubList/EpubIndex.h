#pragma once

#include <vector>
#ifndef UNIT_TEST
#include <esp_log.h>
#else
#define vTaskDelay(t)
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#define ESP_LOGD(args...)
#endif
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <algorithm>

#include "Epub.h"
#include "Renderer/Renderer.h"
#include "../RubbishHtmlParser/blocks/TextBlock.h"
#include "./State.h"

class Epub;
class Renderer;

class EpubIndex
{
private:
  Renderer *renderer;
  Epub *epub = nullptr;
  EpubListItem &state;

public:
  EpubIndex(EpubListItem &state, Renderer *renderer) : renderer(renderer), state(state){};
  ~EpubIndex() {}
  bool load();
  void next();
  void prev();
  void render();
};
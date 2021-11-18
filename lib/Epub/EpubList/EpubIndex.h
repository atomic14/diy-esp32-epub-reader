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
  EpubListItem &selected_epub;
  EpubIndexState &state;
  bool m_needs_redraw = false;

public:
  EpubIndex(EpubListItem &selected_epub, EpubIndexState &state, Renderer *renderer) : renderer(renderer), selected_epub(selected_epub), state(state){};
  ~EpubIndex() {}
  bool load();
  void next();
  void prev();
  void render();
  void set_needs_redraw() { m_needs_redraw = true; }
  uint16_t get_selected_toc();
};
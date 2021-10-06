#pragma once

class Epub;
class Renderer;
class RubbishHtmlParser;

// this state data is saved as RTC_DATA and injected in - this means that we don't lose our place when going in to deep sleep
typedef struct
{
  char epub_path[256];
  int current_section;
  int current_page;
  int pages_in_current_section;
} EpubReaderState;

class EpubReader
{
private:
  EpubReaderState &state;
  Epub *epub = nullptr;
  Renderer *renderer = nullptr;
  RubbishHtmlParser *parser = nullptr;

  void parse_and_layout_current_section();

public:
  EpubReader(EpubReaderState &state, Renderer *renderer) : state(state), renderer(renderer){};
  ~EpubReader() {}
  bool load();
  void next();
  void prev();
  void render();
};
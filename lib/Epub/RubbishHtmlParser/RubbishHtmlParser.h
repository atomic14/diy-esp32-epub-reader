#pragma once

#include <string>
#include <list>
#include <vector>
#include <tinyxml2.h>
#include "blocks/TextBlock.h"

class Page;
class Renderer;
class Epub;

// a very stupid xhtml parser - it will probably work for very simple cases
// but will probably fail for complex ones
class RubbishHtmlParser : public tinyxml2::XMLVisitor
{
private:
  bool is_bold = false;
  bool is_italic = false;

  std::list<Block *> blocks;
  TextBlock *currentTextBlock = nullptr;
  std::vector<Page *> pages;

  std::string m_base_path;

  // start a new text block if needed
  void startNewTextBlock(BLOCK_STYLE style);

public:
  RubbishHtmlParser(const char *html, int length, const std::string &base_path);
  ~RubbishHtmlParser();

  // xml parser callbacks
  bool VisitEnter(const tinyxml2::XMLElement &element, const tinyxml2::XMLAttribute *firstAttribute);
  bool Visit(const tinyxml2::XMLText &text);
  bool VisitExit(const tinyxml2::XMLElement &element);
  // xml parser callbacks

  void parse(const char *html, int length);
  void addText(const char *text, bool is_bold, bool is_italic);
  void layout(Renderer *renderer, Epub *epub);
  int get_page_count()
  {
    return pages.size();
  }
  const std::list<Block *> &get_blocks()
  {
    return blocks;
  }
  void render_page(int page_index, Renderer *renderer, Epub *epub);
};

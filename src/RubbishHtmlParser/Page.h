#pragma once

// a line of a page
class PageLine
{
public:
  // the block the line comes from
  Block *block;
  // the line break index
  int line_break_index;
  // the start position in the page
  int y_pos;

  PageLine(Block *block, int line_break_index, int y_pos)
      : block(block), line_break_index(line_break_index), y_pos(y_pos)
  {
  }
  void render(const char *html, Renderer *renderer)
  {
    block->render(html, renderer, line_break_index, y_pos);
  }
};

// a layed out page ready to be rendered
class Page
{
public:
  // the list of block index and line numbers on this page
  std::vector<PageLine *> lines;
  void render(const char *html, Renderer *renderer)
  {
    for (auto line : lines)
    {
      line->render(html, renderer);
    }
  }
  ~Page()
  {
    for (auto line : lines)
    {
      delete line;
    }
  }
};

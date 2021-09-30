#pragma once

#include "blocks/TextBlock.h"
#include "blocks/ImageBlock.h"

// represents something that has been added to a page
class PageElement
{
public:
  int y_pos;
  PageElement(int y_pos) : y_pos(y_pos) {}
  virtual ~PageElement() {}
  virtual void render(Renderer *renderer, Epub *epub) = 0;
};

// a line from a block element
class PageLine : public PageElement
{
public:
  // the block the line comes from
  TextBlock *block;
  // the line break index
  int line_break_index;

  PageLine(TextBlock *block, int line_break_index, int y_pos)
      : PageElement(y_pos), block(block), line_break_index(line_break_index)
  {
  }
  void render(Renderer *renderer, Epub *epub)
  {
    block->render(renderer, line_break_index, y_pos);
  }
};

// an image
class PageImage : public PageElement
{
public:
  // the block the image comes from
  ImageBlock *block;

  PageImage(ImageBlock *block, int y_pos) : PageElement(y_pos), block(block)
  {
  }
  void render(Renderer *renderer, Epub *epub)
  {
    block->render(renderer, epub, y_pos);
  }
};

// a layed out page ready to be rendered
class Page
{
public:
  // the list of block index and line numbers on this page
  std::vector<PageElement *> elements;
  void render(Renderer *renderer, Epub *epub)
  {
    for (auto element : elements)
    {
      element->render(renderer, epub);
    }
  }
  ~Page()
  {
    for (auto element : elements)
    {
      delete element;
    }
  }
};

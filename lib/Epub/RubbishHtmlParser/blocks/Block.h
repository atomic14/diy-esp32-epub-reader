#pragma once

class Renderer;
class Epub;

typedef enum
{
  TEXT_BLOCK,
  IMAGE_BLOCK
} BlockType;

// a block of content in the html - either a paragraph or an image
class Block
{
public:
  virtual ~Block() {}
  virtual void layout(const char *html, Renderer *renderer, Epub *epub) = 0;
  virtual void dump(const char *html) = 0;
  virtual BlockType getType() = 0;
  virtual bool isEmpty() = 0;
  virtual void finish() {};
};

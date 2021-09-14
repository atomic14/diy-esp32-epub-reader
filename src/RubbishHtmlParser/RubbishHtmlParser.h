#pragma once

#include <stdio.h>
#include <string>
#include <list>
#include <vector>
#include <exception>
#include "ZipFile.h"
#include "Renderer/Renderer.h"
#include "htmlEntities.h"
#include "blocks/TextBlock.h"
#include "blocks/ImageBlock.h"
#include "Page.h"

// thrown if we get into an unexpected state
class ParseException : public std::exception
{
private:
  std::string message;

public:
  ParseException(int index)
  {
    message = "Parse error at index " + std::to_string(index);
  }
  const char *what() const throw()
  {
    return message.c_str();
  }
};

// a very stupid xhtml parser - it will probably work for very simple cases
// but will probably fail for complex ones
class RubbishHtmlParser
{
private:
  const char *m_html;
  int m_length;

  std::list<Block *> blocks;
  TextBlock *currentTextBlock = nullptr;
  std::vector<Page *> pages;

public:
  // is there any more whitespace we should consider?
  bool isWhiteSpace(char c)
  {
    return (c == ' ' || c == '\r' || c == '\n');
  }
  // we've got a limited set of tags that we treat as block tags
  // block tags contain a paragraph of text
  bool isBlockTag(const char *html, int index, int length)
  {
    // skip past the '<'
    index++;
    if (index >= length)
    {
      throw ParseException(index);
    }
    // find the end of the tag name
    int start = index;
    while (index < length && html[index] != ' ' && html[index] != '>')
    {
      index++;
    }
    bool isBlock =
        (strncmp(html + start, "h1", index - start) == 0) ||
        (strncmp(html + start, "h2", index - start) == 0) ||
        (strncmp(html + start, "h3", index - start) == 0) ||
        (strncmp(html + start, "h4", index - start) == 0) ||
        (strncmp(html + start, "p", index - start) == 0) ||
        (strncmp(html + start, "div", index - start) == 0) ||
        (strncmp(html + start, "li", index - start) == 0);
    return isBlock;
  }
  bool isClosingBlockTag(const char *html, int index, int length)
  {
    // are we a closing block?
    if (index + 2 > length || html[index] != '<' || html[index + 1] != '/')
    {
      return false;
    }
    // skip past the '</'
    index += 2;
    if (index >= length)
    {
      throw ParseException(index);
    }
    // find the end of the tag name
    int start = index;
    while (index < length && html[index] != ' ' && html[index] != '>')
    {
      index++;
    }
    bool isBlock =
        (strncmp(html + start, "h1", index - start) == 0) ||
        (strncmp(html + start, "h2", index - start) == 0) ||
        (strncmp(html + start, "h3", index - start) == 0) ||
        (strncmp(html + start, "h4", index - start) == 0) ||
        (strncmp(html + start, "p", index - start) == 0) ||
        (strncmp(html + start, "div", index - start) == 0) ||
        (strncmp(html + start, "li", index - start) == 0);
    return isBlock;
  }
  // move past an html tag - basically move forward until we hit '>'
  int skipTag(const char *html, int index, int length)
  {
    // skip past the '<'
    index++;
    while (index < length && html[index] != '>')
    {
      index++;
    }
    return index + 1;
  }
  // move past anything that should be considered part of a work
  int skipAlphaNum(const char *html, int index, int length)
  {
    while (index < length && !isWhiteSpace(html[index]) && html[index] != '<')
    {
      index++;
    }
    return index;
  }
  //    bool isItalicTag(char *html, int start, int end) {
  //        return (strncmp(html+start, "i", end-start) == 0);
  //    }
  //    bool isBoldTag(char *html, int start, int end) {
  //        return (strncmp(html+start, "b", end-start) == 0);
  //    }
  bool isImageTag(const char *html, int index, int length)
  {
    if (index + 4 < length)
    {
      return (strncmp(html + index, "<img", 4) == 0);
    }
    return false;
  }
  // start a new text block if needed
  void startNewTextBlock()
  {
    if (currentTextBlock && currentTextBlock->words.size() == 0)
    {
      return;
    }
    currentTextBlock = new TextBlock();
    blocks.push_back(currentTextBlock);
  }
  // skip past any white space characters
  int skipWhiteSpace(const char *html, int index, int length)
  {
    while (index < length && isWhiteSpace(html[index]))
    {
      index++;
    }
    return index;
  }
  // find the location of an attribute within a tag
  bool findAttribute(const char *html, int index, int length, const char *name, int *src_start, int *src_end)
  {
    // attribute name length
    int name_length = strlen(name);
    // skip forward until we find the name of the attribute
    while (index < length - name_length && strncmp(html + index, name, name_length) != 0 && html[index] != '>')
    {
      index++;
    }
    // did we actually fund the attribute?
    if (index < length - name_length && strncmp(html + index, name, name_length) == 0)
    {
      // skip past the name of the attribute
      index += name_length;
      // skip past any white space
      index = skipWhiteSpace(html, index, length);
      // skip past the "="
      index++;
      // skip past any white space
      index = skipWhiteSpace(html, index, length);
      // skip past the "
      index++;
      if (index >= length)
      {
        return false;
      }
      // find the end of the attribute value
      int start = index;
      while (index < length && html[index] != '"')
      {
        index++;
      }
      if (index >= length)
      {
        return false;
      }
      *src_start = start;
      *src_end = index;
      return true;
    }
    return false;
  }
  // a closing tag will have a '/' character immediately following the '<'
  bool isClosingTag(const char *html, int index, int length)
  {
    if (index + 1 >= length)
    {
      throw ParseException(index);
    }
    return html[index] == '<' && html[index + 1] == '/';
  }
  // self closing tags have a / before we hit the '>'
  // this assumes that we have already eliminated the possibility of a closing tag
  bool isSelfClosing(const char *html, int index, int length)
  {
    while (html[index] != '>' && index < length)
    {
      if (html[index] == '/')
      {
        return true;
      }
      index++;
    }
    if (index == length)
    {
      throw ParseException(index);
    }
    return false;
  }
  void parse(const char *html, int index, int length)
  {
    // keep track of inline tag depth
    while (index < length)
    {
      if (index % 1000 == 0)
      {
        vTaskDelay(1);
      }
      // skip past any whitespace
      index = skipWhiteSpace(html, index, length);
      // TODO
      // // are we on a tag that we should ignore - e.g. <head>
      // if (ignoreTag(html, index, length))
      // {
      //   index = skipTag(html, index, length);
      // }
      // have we hit a tag?
      if (html[index] == '<')
      {
        if (isClosingTag(html, index, length))
        {
          if (isClosingBlockTag(html, index, length))
          {
            startNewTextBlock();
          }
          else
          {
            // TODO handle </b>, </i> etc..
          }
        }
        else if (isSelfClosing(html, index, length))
        {
          if (isImageTag(html, index, length))
          {
            int src_start, src_end;
            if (findAttribute(html, index, length, "src", &src_start, &src_end))
            {
              // don't leave an empty text block in the list
              if (currentTextBlock->words.size() == 0)
              {
                blocks.pop_back();
                delete currentTextBlock;
                currentTextBlock = nullptr;
              }
              blocks.push_back(new ImageBlock(src_start, src_end));
              // start a new text block
              startNewTextBlock();
            }
            else
            {
              ESP_LOGE(TAG, "Could not find src attribute");
            }
          }
          // TODO handle <br/>
        }
        else if (isBlockTag(html, index, length))
        {
          startNewTextBlock();
        }
        else
        {
          // TODO handle </b>, </i> etc...
        }
        index = skipTag(html, index, length);
      }
      else
      {
        // add the word to the current block
        int wordStart = index;
        index = skipAlphaNum(html, index, length);
        if (index > wordStart)
        {
          currentTextBlock->words.push_back(new Word(wordStart, index));
        }
      }
    }
  }
  RubbishHtmlParser(const char *html, int length)
  {
    m_html = html;
    m_length = length;
    int index = 0;
    // skip to the html tag
    while (strncmp(html + index, "<html", 5) != 0)
    {
      index++;
    }
    startNewTextBlock();
    parse(html, index, length);
    // for (auto block : blocks)
    // {
    //   block->dump(html);
    // }
  }
  ~RubbishHtmlParser()
  {
    for (auto block : blocks)
    {
      delete block;
    }
  }

  void layout(Renderer *renderer)
  {
    const int line_height = renderer->get_line_height();
    const int page_height = renderer->get_page_height();
    // first ask the blocks to work out where they should have
    // line breaks based on the page width
    for (auto block : blocks)
    {
      block->layout(m_html, renderer);
      vTaskDelay(1);
    }
    // now we need to allocate the lines to pages
    // we'll run through each block and the lines within each block and allocate
    // them to pages. When we run out of space on a page we'll start a new page
    // and continue
    int y = 0;
    pages.push_back(new Page());
    for (auto block : blocks)
    {
      vTaskDelay(1);
      if (block->getType() == BlockType::TEXT_BLOCK)
      {
        TextBlock *textBlock = (TextBlock *)block;
        for (int line_break_index = 0; line_break_index < textBlock->line_breaks.size(); line_break_index++)
        {
          if (y + line_height > page_height)
          {
            pages.push_back(new Page());
            y = 0;
          }
          pages.back()->elements.push_back(new PageLine(textBlock, line_break_index, y));
          y += line_height;
        }
        // add an extra line between blocks
        y += line_height;
      }
      if (block->getType() == BlockType::IMAGE_BLOCK)
      {
        ImageBlock *imageBlock = (ImageBlock *)block;
        if (y + imageBlock->height > page_height)
        {
          pages.push_back(new Page());
          y = 0;
        }
        pages.back()->elements.push_back(new PageImage(imageBlock, y));
        y += imageBlock->height;
      }
    }
  }
  int get_page_count()
  {
    return pages.size();
  }
  void render_page(int page_index, Renderer *renderer)
  {
    if (page_index < 0 || page_index >= pages.size())
    {
      throw std::runtime_error("Invalid page index");
    }
    pages[page_index]->render(m_html, renderer);
  }
};

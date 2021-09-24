#ifndef UNIT_TEST
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#else
#define vTaskDelay(t)
#define ESP_LOGE(args...)
#define ESP_LOGI(args...)
#endif
#include <stdio.h>
#include <string.h>
#include <string>
#include <list>
#include <vector>
#include <exception>
#include "../ZipFile/ZipFile.h"
#include "../Renderer/Renderer.h"
#include "htmlEntities.h"
#include "blocks/TextBlock.h"
#include "blocks/ImageBlock.h"
#include "Page.h"
#include "RubbishHtmlParser.h"
#include "../EpubList/Epub.h"

static const char *TAG = "HTML";

const char *HEADER_TAGS[] = {"h1", "h2", "h3", "h4", "h5", "h6"};
const int NUM_HEADER_TAGS = sizeof(HEADER_TAGS) / sizeof(HEADER_TAGS[0]);

const char *BLOCK_TAGS[] = {"p", "li", "div"};
const int NUM_BLOCK_TAGS = sizeof(BLOCK_TAGS) / sizeof(BLOCK_TAGS[0]);

const char *BOLD_TAGS[] = {"b"};
const int NUM_BOLD_TAGS = sizeof(BOLD_TAGS) / sizeof(BOLD_TAGS[0]);

const char *ITALIC_TAGS[] = {"i"};
const int NUM_ITALIC_TAGS = sizeof(ITALIC_TAGS) / sizeof(ITALIC_TAGS[0]);

const char *IMAGE_TAGS[] = {"img"};
const int NUM_IMAGE_TAGS = sizeof(IMAGE_TAGS) / sizeof(IMAGE_TAGS[0]);

const char *SKIP_TAGS[] = {"head"};
const int NUM_SKIP_TAGS = sizeof(SKIP_TAGS) / sizeof(SKIP_TAGS[0]);

// given the start and end of a tag, check to see if it matches a known tag
bool matches(const char *html, int start, int end, const char *possible_tags[], int possible_tag_count)
{
  for (int i = 0; i < possible_tag_count; i++)
  {
    if (end - start == strlen(possible_tags[i]) &&
        strncmp(html + start, possible_tags[i], end - start) == 0)
    {
      return true;
    }
  }
  return false;
}

RubbishHtmlParser::RubbishHtmlParser(const char *html, int length)
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
}

RubbishHtmlParser::~RubbishHtmlParser()
{
  for (auto block : blocks)
  {
    delete block;
  }
}

// assuming we are at the start of a tag, work out where the tag name is
void RubbishHtmlParser::getTagName(const char *html, int index, int length, int &start, int &end)
{
  start = index;
  end = index;
  while (end < length && html[end] != ' ' && html[end] != '>')
  {
    end++;
  }
}

// move past an html tag - basically move forward until we hit '>'
int RubbishHtmlParser::skipToEndOfTagMarker(const char *html, int index, int length)
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
int RubbishHtmlParser::skipAlphaNum(const char *html, int index, int length)
{
  while (index < length && !isWhiteSpace(html[index]) && html[index] != '<')
  {
    index++;
  }
  return index;
}

// start a new text block if needed
void RubbishHtmlParser::startNewTextBlock()
{
  if (!currentTextBlock || currentTextBlock->words.size() > 0)
  {
    if (currentTextBlock)
    {
      currentTextBlock->finish();
    }
    currentTextBlock = new TextBlock();
    blocks.push_back(currentTextBlock);
  }
}

// skip past any white space characters
int RubbishHtmlParser::skipWhiteSpace(const char *html, int index, int length)
{
  while (index < length && isWhiteSpace(html[index]))
  {
    index++;
  }
  return index;
}

// find the location of an attribute within a tag
bool RubbishHtmlParser::findAttribute(const char *html, int index, int length, const char *name, int *src_start, int *src_end)
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
bool RubbishHtmlParser::isClosingTag(const char *html, int index, int length)
{
  return html[index] == '<' && html[index + 1] == '/';
}

// self closing tags have a / before we hit the '>'
bool RubbishHtmlParser::isSelfClosing(const char *html, int index, int length)
{
  // is it just a closing tag?
  if (isClosingTag(html, index, length))
  {
    return false;
  }
  // do we have a slash before the end of the tag?
  while (html[index] != '>' && index < length)
  {
    if (html[index] == '/')
    {
      return true;
    }
    index++;
  }
  return false;
}

int RubbishHtmlParser::processClosingTag(const char *html, int index, int length, bool &is_bold, bool &is_italic)
{
  // skip past the '</'
  index += 2;
  index = skipWhiteSpace(html, index, length);
  // get the tag name
  int tag_start = index;
  int tag_end = index;
  getTagName(html, index, length, tag_start, tag_end);
  if (matches(html, tag_start, tag_end, HEADER_TAGS, NUM_HEADER_TAGS))
  {
    is_bold = false;
    startNewTextBlock();
  }
  else if (matches(html, tag_start, tag_end, BLOCK_TAGS, NUM_BLOCK_TAGS))
  {
    startNewTextBlock();
  }
  else if (matches(html, tag_start, tag_end, BOLD_TAGS, NUM_BOLD_TAGS))
  {
    is_bold = false;
  }
  else if (matches(html, tag_start, tag_end, ITALIC_TAGS, NUM_ITALIC_TAGS))
  {
    is_italic = false;
  }
  return index;
}

int RubbishHtmlParser::processSelfClosingTag(const char *html, int index, int length)
{
  // skip past the '<'
  index++;
  // skip past any white space
  index = skipWhiteSpace(html, index, length);
  // get the tag name
  int tag_start = index;
  int tag_end = index;
  getTagName(html, index, length, tag_start, tag_end);
  // we only handle image tags
  if (matches(html, tag_start, tag_end, IMAGE_TAGS, NUM_IMAGE_TAGS))
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
      blocks.push_back(new ImageBlock(std::string(html + src_start, src_end - src_start)));
      // start a new text block
      startNewTextBlock();
    }
    else
    {
      ESP_LOGE(TAG, "Could not find src attribute");
    }
  }
  return index;
}

int RubbishHtmlParser::processOpeningTag(const char *html, int index, int length, bool &is_bold, bool &is_italic)
{
  // skip past the '<'
  index++;
  index = skipWhiteSpace(html, index, length);
  // get the tag name
  int tag_start = index;
  int tag_end = index;
  getTagName(html, index, length, tag_start, tag_end);
  // is this a tag that should be skipped?
  if (matches(html, tag_start, tag_end, SKIP_TAGS, NUM_SKIP_TAGS))
  {
    printf("Hit a skip tag\n");
    // move forward until we find a matching end tag - we assume that skip tags are not nested
    while (index < length)
    {
      // have we found a closing tag?
      if (isClosingTag(html, index, length))
      {
        // get the closing tag name
        index += 2;
        index = skipWhiteSpace(html, index, length);
        int close_tag_start = index;
        int close_tag_end = index;
        getTagName(html, index, length, close_tag_start, close_tag_end);
        // does it match the opening tag?
        if (close_tag_end - close_tag_start == tag_end - tag_start &&
            strncmp(html + close_tag_start, html + tag_start, close_tag_end - close_tag_start) == 0)
        {
          printf("found a matching close tag\n");
          // we found the matching end tag
          break;
        }
      }
      // keep moving forward
      index++;
    }
    // skip to the end of the closing tag
    index = skipToEndOfTagMarker(html, index, length);
  }
  else if (matches(html, tag_start, tag_end, HEADER_TAGS, NUM_HEADER_TAGS))
  {
    is_bold = true;
    startNewTextBlock();
  }
  else if (matches(html, tag_start, tag_end, BLOCK_TAGS, NUM_BLOCK_TAGS))
  {
    startNewTextBlock();
  }
  else if (matches(html, tag_start, tag_end, BOLD_TAGS, NUM_BOLD_TAGS))
  {
    is_bold = true;
  }
  else if (matches(html, tag_start, tag_end, ITALIC_TAGS, NUM_ITALIC_TAGS))
  {
    is_italic = true;
  }
  return index;
}

void RubbishHtmlParser::parse(const char *html, int index, int length)
{
  bool is_bold = false;
  bool is_italic = false;
  // keep track of inline tag depth
  while (index < length)
  {
    if (index % 1000 == 0)
    {
      vTaskDelay(1);
    }
    // skip past any whitespace
    index = skipWhiteSpace(html, index, length);
    // have we hit a tag?
    if (html[index] == '<')
    {
      if (isClosingTag(html, index, length))
      {
        index = processClosingTag(html, index, length, is_bold, is_italic);
      }
      else if (isSelfClosing(html, index, length))
      {
        index = processSelfClosingTag(html, index, length);
      }
      else
      {
        index = processOpeningTag(html, index, length, is_bold, is_italic);
      }
      index = skipToEndOfTagMarker(html, index, length);
    }
    else
    {
      // add the word to the current block
      int wordStart = index;
      index = skipAlphaNum(html, index, length);
      if (index > wordStart)
      {
        currentTextBlock->words.push_back(new Word(std::string(html, wordStart, index - wordStart), is_bold, is_italic));
      }
    }
  }
  if (blocks.back()->isEmpty())
  {
    delete blocks.back();
    blocks.pop_back();
  }
  int total_words = 0;
  for (auto block : blocks)
  {
    if (block->getType() == BlockType::TEXT_BLOCK)
    {
      total_words += ((TextBlock *)block)->words.size();
    }
  }
  ESP_LOGI(TAG, "Parsed %d blocks with %d words", blocks.size(), total_words);
}

void RubbishHtmlParser::layout(Renderer *renderer, Epub *epub)
{
  const int line_height = renderer->get_line_height();
  const int page_height = renderer->get_page_height();
  // first ask the blocks to work out where they should have
  // line breaks based on the page width
  for (auto block : blocks)
  {
    block->layout(renderer, epub);
    // feed the watchdog
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
    // feed the watchdog
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

void RubbishHtmlParser::render_page(int page_index, Renderer *renderer)
{
  renderer->clear_screen();
  pages[page_index]->render(m_html, renderer);
}

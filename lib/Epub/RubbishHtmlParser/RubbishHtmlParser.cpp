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

const char *BLOCK_TAGS[] = {"p", "li", "div", "br"};
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
bool matches(const char *tag_name, const char *possible_tags[], int possible_tag_count)
{
  for (int i = 0; i < possible_tag_count; i++)
  {
    if (strcmp(tag_name, possible_tags[i]) == 0)
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
  startNewTextBlock();
  parse(html, length);
}

RubbishHtmlParser::~RubbishHtmlParser()
{
  for (auto block : blocks)
  {
    delete block;
  }
}

bool RubbishHtmlParser::VisitEnter(const tinyxml2::XMLElement &element, const tinyxml2::XMLAttribute *firstAttribute)
{
  const char *tag_name = element.Name();
  // we only handle image tags
  if (matches(tag_name, IMAGE_TAGS, NUM_IMAGE_TAGS))
  {
    const char *src = element.Attribute("src");
    if (src)
    {
      // don't leave an empty text block in the list
      if (currentTextBlock->is_empty())
      {
        blocks.pop_back();
        delete currentTextBlock;
        currentTextBlock = nullptr;
      }
      blocks.push_back(new ImageBlock(src));
      // start a new text block
      startNewTextBlock();
    }
    else
    {
      ESP_LOGE(TAG, "Could not find src attribute");
    }
  }
  else if (matches(tag_name, SKIP_TAGS, NUM_SKIP_TAGS))
  {
    return false;
  }
  else if (matches(tag_name, HEADER_TAGS, NUM_HEADER_TAGS))
  {
    is_bold = true;
    startNewTextBlock();
  }
  else if (matches(tag_name, BLOCK_TAGS, NUM_BLOCK_TAGS))
  {
    startNewTextBlock();
  }
  else if (matches(tag_name, BOLD_TAGS, NUM_BOLD_TAGS))
  {
    is_bold = true;
  }
  else if (matches(tag_name, ITALIC_TAGS, NUM_ITALIC_TAGS))
  {
    is_italic = true;
  }
  return true;
}
/// Visit a text node.
bool RubbishHtmlParser::Visit(const tinyxml2::XMLText &text)
{
  addText(text.Value(), is_bold, is_italic);
  return true;
}
bool RubbishHtmlParser::VisitExit(const tinyxml2::XMLElement &element)
{
  const char *tag_name = element.Name();
  if (matches(tag_name, HEADER_TAGS, NUM_HEADER_TAGS))
  {
    is_bold = false;
    startNewTextBlock();
  }
  else if (matches(tag_name, BLOCK_TAGS, NUM_BLOCK_TAGS))
  {
    startNewTextBlock();
  }
  else if (matches(tag_name, BOLD_TAGS, NUM_BOLD_TAGS))
  {
    is_bold = false;
  }
  else if (matches(tag_name, ITALIC_TAGS, NUM_ITALIC_TAGS))
  {
    is_italic = false;
  }
  return true;
}

// start a new text block if needed
void RubbishHtmlParser::startNewTextBlock()
{
  if (!currentTextBlock || !currentTextBlock->is_empty())
  {
    if (currentTextBlock)
    {
      currentTextBlock->finish();
    }
    currentTextBlock = new TextBlock();
    blocks.push_back(currentTextBlock);
  }
}

void RubbishHtmlParser::parse(const char *html, int length)
{
  tinyxml2::XMLDocument doc(false, tinyxml2::COLLAPSE_WHITESPACE);
  doc.Parse(html, length);
  doc.Accept(this);
}

void RubbishHtmlParser::addText(const char *text, bool is_bold, bool is_italic)
{
  currentTextBlock->add_span(text, is_bold, is_italic);
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

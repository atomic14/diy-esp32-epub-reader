#pragma once

#include "../../Renderer/Renderer.h"
#include <limits.h>
#include "Block.h"
#ifndef UNIT_TEST
#include <esp_log.h>
#else
#define ESP_LOGI(args...)
#define ESP_LOGE(args...)
#define ESP_LOGD(args...)
#define ESP_LOGW(args...)
#endif

typedef enum
{
  BOLD_SPAN = 1,
  ITALIC_SPAN = 2,
} SPAN_STYLES;

// TODO - is there any more whitespace we should consider?
static bool is_whitespace(char c)
{
  return (c == ' ' || c == '\r' || c == '\n');
}

// move past anything that should be considered part of a work
static int skip_word(const char *text, int index, int length)
{
  while (index < length && !is_whitespace(text[index]))
  {
    index++;
  }
  return index;
}

// skip past any white space characters
static int skip_whitespace(const char *html, int index, int length)
{
  while (index < length && is_whitespace(html[index]))
  {
    index++;
  }
  return index;
}

// represents a block of words in the html document
class TextBlock : public Block
{
private:
  // the spans of text in this block
  std::vector<const char *> spans;
  // pointer to each word
  std::vector<const char *> words;
  // width of each word
  std::vector<uint16_t> word_widths;
  // x position of each word
  std::vector<uint16_t> word_xpos;
  // the styles of each word
  std::vector<uint8_t> word_styles;

public:
  // where do we want to break the words into lines
  std::vector<uint16_t> line_breaks;

  void add_span(const char *span, bool is_bold, bool is_italic)
  {
    // adding a span to text block
    // make a copy of the text as we'll modify it
    int length = strlen(span);
    char *text = new char[length + 1];
    strcpy(text, span);
    spans.push_back(text);
    // work out where each word is in the span
    int index = 0;
    while (index < length)
    {
      // skip past any whitespace to the start of a word
      index = skip_whitespace(span, index, length);
      int word_start = index;
      // find the end of the word
      index = skip_word(span, index, length);
      int word_length = index - word_start;
      if (word_length > 0)
      {
        // null terminate the word
        text[word_start + word_length] = '\0';
        // store the information about the word for later
        words.push_back(text + word_start);
        // store the style for the word
        word_styles.push_back((is_bold ? BOLD_SPAN : 0) | (is_italic ? ITALIC_SPAN : 0));
      }
    }
  }
  ~TextBlock()
  {
    for (auto span : spans)
    {
      delete[] span;
    }
  }
  bool isEmpty()
  {
    return spans.empty();
  }
  // given a renderer works out where to break the words into lines
  void layout(Renderer *renderer, Epub *epub)
  {
    // measure each word
    for (int i = 0; i < words.size(); i++)
    {
      // measure the word
      int width = renderer->get_text_width(words[i], word_styles[i] & BOLD_SPAN, word_styles[i] & ITALIC_SPAN);
      word_widths.push_back(width);
    }

    int page_width = renderer->get_page_width();
    int space_width = renderer->get_space_width();

    // now apply the dynamic programming algorithm to find the best line breaks
    int n = word_widths.size();

    // DP table in which dp[i] represents cost of line starting with word words[i]
    int dp[n];

    // Array in which ans[i] store index of last word in line starting with word word[i]
    size_t ans[n];

    // If only one word is present then only one line is required. Cost of last line is zero. Hence cost
    // of this line is zero. Ending point is also n-1 as single word is present
    dp[n - 1] = 0;
    ans[n - 1] = n - 1;

    // Make each word first word of line by iterating over each index in arr.
    for (int i = n - 2; i >= 0; i--)
    {
      int currlen = -1;
      dp[i] = INT_MAX;

      // Variable to store possible minimum cost of line.
      int cost;

      // Keep on adding words in current line by iterating from starting word upto last word in arr.
      for (int j = i; j < n; j++)
      {
        // Update the width of the words in current line + the space between two words.
        currlen += (word_widths[j] + space_width);

        // If we're bigger than the current pagewidth then we can't add more words
        if (currlen > page_width)
          break;

        // if we've run out of words then this is last line and the cost should be 0
        // Otherwise the cost is the sqaure of the left over space + the costs of all the previous lines
        if (j == n - 1)
          cost = 0;
        else
          cost = (page_width - currlen) * (page_width - currlen) + dp[j + 1];

        // Check if this arrangement gives minimum cost for line starting with word words[i].
        if (cost < dp[i])
        {
          dp[i] = cost;
          ans[i] = j;
        }
      }
    }
    // We can now iterate through the answer to find the line break positions
    size_t i = 0;
    while (i < n)
    {
      i = ans[i] + 1;
      if (i > n)
      {
        ESP_LOGI("TextBlock", "fallen off the end of the words");
        dump();

        for (int x = 0; x < n; x++)
        {
          ESP_LOGI("TextBlock", "line break %d=>%d", x, ans[x]);
        }
        break;
      }
      line_breaks.push_back(i);
      if (line_breaks.size() > 1000)
      {
        ESP_LOGE("TextBlock", "too many line breaks");
        dump();

        for (int x = 0; x < n; x++)
        {
          ESP_LOGI("TextBlock", "line break %d=>%d", x, ans[x]);
        }
        break;
      }
    }
    // With the page breaks calculated we can now position the words along the line
    int start_word = 0;
    for (int i = 0; i < line_breaks.size(); i++)
    {
      int total_word_width = 0;
      for (int word_index = start_word; word_index < line_breaks[i]; word_index++)
      {
        total_word_width += word_widths[word_index];
      }
      float actual_spacing = space_width;
      // don't add space if we are on the last line
      if (i != line_breaks.size() - 1)
      {
        if (line_breaks[i] - start_word > 2)
        {
          actual_spacing = float(page_width - total_word_width) / float(line_breaks[i] - start_word - 1);
        }
      }
      float xpos = 0;
      word_xpos.resize(words.size());
      for (int word_index = start_word; word_index < line_breaks[i]; word_index++)
      {
        word_xpos[word_index] = xpos;
        xpos += word_widths[word_index] + actual_spacing;
      }
      start_word = line_breaks[i];
    }
    spans.shrink_to_fit();
    words.shrink_to_fit();
    word_widths.shrink_to_fit();
    word_xpos.shrink_to_fit();
    word_styles.shrink_to_fit();
  }
  void render(Renderer *renderer, int line_break_index, int y_pos)
  {
    int start = line_break_index == 0 ? 0 : line_breaks[line_break_index - 1];
    int end = line_breaks[line_break_index];
    for (int i = start; i < end; i++)
    {
      // get the style
      uint8_t style = word_styles[i];
      // render the word
      renderer->draw_text(word_xpos[i], y_pos, words[i], style & BOLD_SPAN, style & ITALIC_SPAN);
    }
  }
  // debug helper - dumps out the contents of the block with line breaks
  void dump()
  {
    for (int i = 0; i < words.size(); i++)
    {
      printf("##%d#%s## ", word_widths[i], words[i]);
    }
  }
  bool is_empty()
  {
    return words.empty();
  }
  virtual BlockType getType()
  {
    return TEXT_BLOCK;
  }
};

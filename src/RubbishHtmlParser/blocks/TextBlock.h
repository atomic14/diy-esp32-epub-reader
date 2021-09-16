#pragma once

#include "../../Renderer/Renderer.h"
#include <limits.h>
#include "Block.h"
// represents a single word in the html
// to reduce memory use, the word is stored as a reference
// to the start and end indexes in the html
class Word
{
public:
  bool bold;
  bool italic;
  int start;
  int end;
  int xpos = 0;
  int width = 0;
  Word(int start, int end, bool bold = false, bool italic = false)
  {
    this->start = start;
    this->end = end;
    this->bold = bold;
    this->italic = italic;
  }
  void layout(const char *html, Renderer *renderer)
  {
    width = renderer->get_text_width(html, start, end, bold, italic);
  }
  void render(const char *html, Renderer *renderer, int y)
  {
    renderer->draw_text(xpos, y, html, start, end, bold, italic);
  }
};

// represents a block of words in the html document
class TextBlock : public Block
{
public:
  // the words in the block
  std::vector<Word *> words;
  // where do we want to break the words into lines
  std::vector<int> line_breaks;
  ~TextBlock()
  {
    for (auto word : words)
    {
      delete word;
    }
  }
  // given a renderer works out where to break the words into lines
  void layout(const char *html, Renderer *renderer, Epub *epub)
  {
    // make sure all the words have been measured
    for (auto word : words)
    {
      word->layout(html, renderer);
    }
    int page_width = renderer->get_page_width();
    int space_width = renderer->get_space_width();

    // now apply the dynamic programming algorithm to find the best line breaks
    int n = words.size();

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
        currlen += (words[j]->width + space_width);

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
      line_breaks.push_back(ans[i] + 1);
      i = ans[i] + 1;
    }
    // With the page breaks calculated we can now position the words along the line
    int start_word = 0;
    for (int i = 0; i < line_breaks.size(); i++)
    {
      int total_word_width = 0;
      for (int word_index = start_word; word_index < line_breaks[i]; word_index++)
      {
        total_word_width += words[word_index]->width;
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
      for (int word_index = start_word; word_index < line_breaks[i]; word_index++)
      {
        words[word_index]->xpos = xpos;
        xpos += words[word_index]->width + actual_spacing;
      }
      start_word = line_breaks[i];
    }
  }
  void render(const char *html, Renderer *renderer, int line_break_index, int y_pos)
  {
    int start = line_break_index == 0 ? 0 : line_breaks[line_break_index - 1];
    int end = line_breaks[line_break_index];
    for (int i = start; i < end; i++)
    {
      words[i]->render(html, renderer, y_pos);
    }
  }
  // debug helper - dumps out the contents of the block with line breaks
  void dump(const char *html)
  {
    int start = 0;
    for (auto lineBreak : line_breaks)
    {
      for (int i = start; i < lineBreak; i++)
      {
        for (int j = words[i]->start; j < words[i]->end; j++)
        {
          printf("%c", html[j]);
        }
        if (i < words.size() - 1 && html[words[i + 1]->start] != '.')
        {
          printf(" ");
        }
      }
      printf("\n");
      start = lineBreak;
    }
    printf("\n--\n");
  }
  virtual BlockType getType()
  {
    return TEXT_BLOCK;
  }
};

#pragma once

#include <stdio.h>
#include <string>
#include <list>
#include <vector>
#include <exception>
#include "htmlEntities.h"

// represents a single word in the html document
class Word
{
public:
  bool bold;
  bool italic;
  int start;
  int end;
  Word(int start, int end, bool bold = false, bool italic = false)
  {
    this->start = start;
    this->end = end;
    this->bold = bold;
    this->italic = italic;
  }
  int width()
  {
    // TODO - calculate the width in pixels
    return end - start;
  }
};

// represents a block of text in the html document
class Block
{
public:
  // the words in the block
  std::vector<Word *> words;
  // where do we want to break the words into lines
  std::vector<int> lineBreaks;
  // given the width of a page, work out the linebreaks
  void layout(int pageWidth)
  {
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
        currlen += (words[j]->width() + 1);

        // If we're bigger than the current pagewidth then we can't add more words
        if (currlen > pageWidth)
          break;

        // if we've run out of words then this is last line and the cost should be 0
        // Otherwise the cost is the sqaure of the left over space + the costs of all the previous lines
        if (j == n - 1)
          cost = 0;
        else
          cost = (pageWidth - currlen) * (pageWidth - currlen) + dp[j + 1];

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
      lineBreaks.push_back(ans[i] + 1);
      i = ans[i] + 1;
    }
  }
  // debug helper - dumps out the contents of the block with line breaks
  void dump(const char *html)
  {
    layout(80);
    int start = 0;
    for (auto lineBreak : lineBreaks)
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
};

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

// a very stupid xhtml parser - it will probably work for most simple cases
// but will probably fail for complex ones
class RubbishHtmlParser
{
private:
  std::list<Block *> blocks;

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
  // start a new text block if needed
  void startNewBlock()
  {
    // only create a new block if the current block has some content in
    // this prevents lots of blank lines
    if (blocks.back()->words.size() > 0)
    {
      blocks.push_back(new Block());
    }
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
            startNewBlock();
          }
          else
          {
            // TODO handle </b>, </i> etc..
          }
        }
        else if (isSelfClosing(html, index, length))
        {
          // TODO handle <br/>
        }
        else if (isBlockTag(html, index, length))
        {
          // we're assuming that self closing is <br/> or <hr/>
          startNewBlock();
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
          blocks.back()->words.push_back(new Word(wordStart, index));
        }
      }
    }
  }
  RubbishHtmlParser(const char *html, int length)
  {
    int index = 0;
    // skip to the html tag
    while (strncmp(html + index, "<html", 5) != 0)
    {
      index++;
    }
    blocks.push_back(new Block());
    parse(html, index, length);
    for (auto block : blocks)
    {
      block->dump(html);
    }
  }
};

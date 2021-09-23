#include <unity.h>
#include <string.h>
#include <RubbishHtmlParser/RubbishHtmlParser.h>
#include <RubbishHtmlParser/blocks/Block.h>

void test_parser(void)
{
  const char *html =
      "<html>"
      "<head>"
      "<title>Test</title>"
      "</head>"
      "<body>"
      "<h1>This is a title</h1>"
      "<p>Text</p>"
      "<p>Some more text</p>"
      "<div>A block of text</div>"
      "<h2>A sub heading</h2>"
      "<p>Bananas!</p>"
      "</body>"
      "</html>";
  RubbishHtmlParser parser(html, strlen(html));
  TEST_ASSERT_EQUAL(6, parser.get_blocks().size());
}

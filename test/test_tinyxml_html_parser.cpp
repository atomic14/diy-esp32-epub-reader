#include <unity.h>
#include <string.h>
#include <tinyxml2.h>

void test_xml_parser(void)
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
      "<div>A block of text"
      "<p>A Paragraph in a div</p>"
      "Some more text"
      "</div>"
      "<h2>A sub heading</h2>"
      "<p>Bananas! <b>Bold</b> <i>italic text</i> in pyjamas</p>"
      "</body>"
      "</html>";
  tinyxml2::XMLDocument doc;
  auto result = doc.Parse(html);
  TEST_ASSERT_EQUAL(tinyxml2::XML_SUCCESS, result);
  doc.Print();
  auto body = doc.FirstChildElement("html")->FirstChildElement("body");
  TEST_ASSERT_NOT_NULL_MESSAGE(body, "Body element not found");
  auto tag = body->FirstChild();
  while (tag)
  {
    printf("%s\n", tag->Value());
    printf("%s\n", tag->ToElement()->GetText());
    if (!tag->ToElement()->NoChildren())
    {
      printf("**** has children...\n");
    }
    tag = tag->NextSibling();
  }
}

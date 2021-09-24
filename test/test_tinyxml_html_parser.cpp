#include <unity.h>
#include <string.h>
#include <stdio.h>
#include <tinyxml2.h>
#include <list>

class MyVisitor : public tinyxml2::XMLVisitor
{
public:
  virtual bool VisitEnter(const tinyxml2::XMLElement &element, const tinyxml2::XMLAttribute *firstAttribute)
  {
    printf("**** Entering element %s\n", element.Name());
    return true;
  }
  /// Visit a text node.
  virtual bool Visit(const tinyxml2::XMLText &text)
  {
    printf("+++ Text\n");
    printf("%s\n", text.Value());
    printf("--- Text\n");
    return true;
  }
  virtual bool VisitExit(const tinyxml2::XMLElement &element)
  {
    printf("**** Exiting element %s\n", element.Name());
    return true;
  }
};

void test_xml_parser(void)
{
  FILE *fp = fopen("test/fixtures/test.html", "rb");
  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  fseek(fp, 0, SEEK_SET); //same as rewind(f);

  char *html = (char *)malloc(fsize + 1);
  fread(html, fsize, 1, fp);
  html[fsize] = 0;

  tinyxml2::XMLDocument doc(false, tinyxml2::COLLAPSE_WHITESPACE);
  auto result = doc.Parse(html);
  TEST_ASSERT_EQUAL(tinyxml2::XML_SUCCESS, result);
  auto body = doc.FirstChildElement("html")->FirstChildElement("body");
  TEST_ASSERT_NOT_NULL_MESSAGE(body, "Body element not found");
  body->Accept(new MyVisitor());
  // auto tag = body->FirstChild();
  // std::list<tinyxml2::XMLNode *> stack;
  // while (tag)
  // {
  //   printf("%s\n", tag->Value());
  //   if (!tag->FirstChild())
  //   {
  //     auto child = tag->FirstChild();
  //     printf("Has Childdren '%s'\n", child->Value());
  //   }
  //   // {
  //   //   printf("Got a child");
  //   //   stack.push_back(tag);
  //   //   tag = tag->FirstChild();
  //   // }
  //   // else
  //   // {
  //   tag = tag->NextSibling();
  //   // }
  //   // if (!tag && !stack.empty())
  //   // {
  //   //   printf("Finished with child");
  //   //   tag = stack.back();
  //   //   stack.pop_back();
  //   // }
  // }
  printf("Finished XML parse\n");
}

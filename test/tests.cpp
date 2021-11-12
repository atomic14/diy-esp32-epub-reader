#include <unity.h>

void test_xml_parser(void);
void test_parser(void);
void test_epub_no_oebps_load(void);
void test_epub_load(void);
void test_epub_relative_image_paths(void);
void test_html_entity_replacement(void);
void test_epub_index_load(void);

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_xml_parser);
  RUN_TEST(test_parser);
  RUN_TEST(test_epub_no_oebps_load);
  RUN_TEST(test_epub_load);
  RUN_TEST(test_epub_relative_image_paths);
  RUN_TEST(test_html_entity_replacement);
  RUN_TEST(test_epub_index_load);
  UNITY_END();

  return 0;
}
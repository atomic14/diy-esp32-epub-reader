#include <unity.h>

void test_xml_parser(void);
void test_parser(void);
void test_epub_no_oebps_load(void);
void test_epub_load(void);
void test_epub_relative_image_paths(void);

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_xml_parser);
  RUN_TEST(test_parser);
  RUN_TEST(test_epub_no_oebps_load);
  RUN_TEST(test_epub_load);
  RUN_TEST(test_epub_relative_image_paths);
  UNITY_END();

  return 0;
}
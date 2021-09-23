#include <unity.h>

void test_xml_parser(void);
void test_parser(void);

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_xml_parser);
  RUN_TEST(test_parser);
  UNITY_END();

  return 0;
}
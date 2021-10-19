#include <unity.h>
#include <RubbishHtmlParser/htmlEntities.h>

void test_html_entity_replacement(void)
{
  TEST_ASSERT_EQUAL_STRING("", replace_html_entities("").c_str());
  TEST_ASSERT_EQUAL_STRING("This is a string with no entities", replace_html_entities("This is a string with no entities").c_str());
  TEST_ASSERT_EQUAL_STRING("This is a string with & and &", replace_html_entities("This is a string with & and &").c_str());
  TEST_ASSERT_EQUAL_STRING("This is a string with &bogus; and &dude;", replace_html_entities("This is a string with &bogus; and &dude;").c_str());
  TEST_ASSERT_EQUAL_STRING("This is a string with © and €", replace_html_entities("This is a string with &copy; and &euro;").c_str());
  TEST_ASSERT_EQUAL_STRING("numeric codes # # Ú Ú र र °", replace_html_entities("numeric codes &#35; &#x23; &#XDA; &#218; &#x0930; &#2352; &deg;").c_str());
  TEST_ASSERT_EQUAL_STRING("This is a nonbreaking space and numeric space", replace_html_entities("This is a nonbreaking&nbsp;space and numeric&#xA0;nbsp").c_str());
  TEST_ASSERT_EQUAL_STRING("frasl ⁄ test", replace_html_entities("frasl &frasl; test").c_str());
}

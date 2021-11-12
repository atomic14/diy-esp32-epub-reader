#include <unity.h>
#include <EpubList/Epub.h>

void test_epub_index_load(void)
{
  Epub *epub = new Epub("fixtures/oebps.epub");
  bool result = epub->loadIndex();
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL(epub->toc_index.size(), 12);
  TEST_ASSERT_EQUAL_STRING(epub->toc_index[0].first.c_str(), "The Strange Case Of Dr. Jekyll And Mr. Hyde");
  TEST_ASSERT_EQUAL_STRING(epub->toc_index[0].second.c_str(), "@public@vhost@g@gutenberg@html@files@43@43-h@43-h-0.htm.html#pgepubid00000");
  TEST_ASSERT_EQUAL_STRING(epub->toc_index[1].first.c_str(), "Contents");
  TEST_ASSERT_EQUAL_STRING(epub->toc_index[1].second.c_str(), "@public@vhost@g@gutenberg@html@files@43@43-h@43-h-0.htm.html#pgepubid00001");
  TEST_ASSERT_EQUAL_STRING(epub->toc_index[11].first.c_str(), "INCIDENT OF THE LETTER");
  TEST_ASSERT_EQUAL_STRING(epub->toc_index[11].second.c_str(), "@public@vhost@g@gutenberg@html@files@43@43-h@43-h-5.htm.html#pgepubid00006");
}

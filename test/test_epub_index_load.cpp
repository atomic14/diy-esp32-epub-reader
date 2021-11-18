#include <unity.h>
#include <EpubList/Epub.h>

void test_epub_toc_load(void)
{
  Epub *epub = new Epub("fixtures/oebps.epub");
  bool result = epub->load();
  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL(epub->get_toc_items_count(), 12);
  TEST_ASSERT_EQUAL_STRING("The Strange Case Of Dr. Jekyll And Mr. Hyde", epub->get_toc_item(0).title.c_str());
  TEST_ASSERT_EQUAL_STRING("OEBPS/@public@vhost@g@gutenberg@html@files@43@43-h@43-h-0.htm.html", epub->get_toc_item(0).href.c_str());
  TEST_ASSERT_EQUAL_STRING("pgepubid00000", epub->get_toc_item(0).anchor.c_str());

  TEST_ASSERT_EQUAL_STRING("Contents", epub->get_toc_item(1).title.c_str());
  TEST_ASSERT_EQUAL_STRING("OEBPS/@public@vhost@g@gutenberg@html@files@43@43-h@43-h-0.htm.html", epub->get_toc_item(1).href.c_str());
  TEST_ASSERT_EQUAL_STRING("pgepubid00001", epub->get_toc_item(1).anchor.c_str());

  TEST_ASSERT_EQUAL_STRING("HENRY JEKYLL\xE2\x80\x99S FULL STATEMENT OF THE CASE", epub->get_toc_item(11).title.c_str());
  TEST_ASSERT_EQUAL_STRING("OEBPS/@public@vhost@g@gutenberg@html@files@43@43-h@43-h-10.htm.html", epub->get_toc_item(11).href.c_str());
  TEST_ASSERT_EQUAL_STRING("pgepubid00011", epub->get_toc_item(11).anchor.c_str());
}

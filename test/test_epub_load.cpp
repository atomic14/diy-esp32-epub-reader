#include <unity.h>
#include <EpubList/Epub.h>

void test_epub_no_oebps_load(void)
{
  Epub *epub = new Epub("fixtures/no_oebps.epub");
  bool result = epub->load();
  TEST_ASSERT_TRUE_MESSAGE(result, "Epub load failed");
  TEST_ASSERT_EQUAL_STRING("El Aleph", epub->get_title().c_str());
  TEST_ASSERT_EQUAL(2, epub->get_spine_items_count());
  TEST_ASSERT_NOT_NULL_MESSAGE(epub->get_item_contents(epub->get_spine_item(0)), "No content for first chapter");
  TEST_ASSERT_NOT_NULL_MESSAGE(epub->get_item_contents(epub->get_spine_item(1)), "No content for second chapter");
}

void test_epub_relative_image_paths(void)
{
  Epub *epub = new Epub("fixtures/relative_paths.epub");
  bool result = epub->load();
  TEST_ASSERT_TRUE_MESSAGE(result, "Epub load failed");
  TEST_ASSERT_EQUAL(373, epub->get_spine_items_count());
  for (int i = 0; i < 373; i++)
  {
    TEST_ASSERT_NOT_NULL_MESSAGE(epub->get_item_contents(epub->get_spine_item(i)), "No content for chapter");
  }
  TEST_ASSERT_EQUAL_STRING("Images/cover.jpg", epub->get_cover_image_item().c_str());
  TEST_ASSERT_NOT_NULL(epub->get_item_contents(epub->get_cover_image_item()));
}

void test_epub_load(void)
{
  Epub *epub = new Epub("fixtures/oebps.epub");
  bool result = epub->load();
  TEST_ASSERT_TRUE_MESSAGE(result, "Epub load failed");
  TEST_ASSERT_EQUAL_STRING("The Strange Case of Dr. Jekyll and Mr. Hyde", epub->get_title().c_str());
  TEST_ASSERT_EQUAL(13, epub->get_spine_items_count());
  for (int i = 0; i < 13; i++)
  {
    TEST_ASSERT_NOT_NULL_MESSAGE(epub->get_item_contents(epub->get_spine_item(i)), "No content for chapter");
  }
  TEST_ASSERT_EQUAL_STRING("@public@vhost@g@gutenberg@html@files@43@43-h@images@cover.jpg", epub->get_cover_image_item().c_str());
  TEST_ASSERT_NOT_NULL(epub->get_item_contents(epub->get_cover_image_item()));
}

#include <unity.h>
#include <string.h>
#include <RubbishHtmlParser/blocks/Block.h>
#include <RubbishHtmlParser/RubbishHtmlParser.h>
#include <RubbishHtmlParser/blocks/ImageBlock.h>
#include <Renderer/Renderer.h>
#include <EpubList/Epub.h>
#include <iterator>

class TestRenderer : public Renderer
{
public:
  virtual void draw_image(const std::string &filename, int x, int y, int width, int height) {}
  virtual bool get_image_size(const std::string &filename, int *width, int *height) { return false; }
  virtual void draw_pixel(int x, int y, uint8_t color) {}
  virtual int get_text_width(const char *text, bool bold = false, bool italic = false)
  {
    return strlen(text);
  }
  virtual void draw_text(int x, int y, const char *text, bool bold = false, bool italic = false) {}
  virtual void draw_text_box(const std::string &text, int x, int y, int width, int height, bool bold = false, bool italic = false) {}
  virtual void draw_rect(int x, int y, int width, int height, uint8_t color = 0){};
  virtual void draw_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color){};
  virtual void draw_circle(int x, int y, int r, uint8_t color = 0){};
  virtual void fill_rect(int x, int y, int width, int height, uint8_t color = 0){};
  virtual void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color){};
  virtual void fill_circle(int x, int y, int r, uint8_t color = 0){};
  virtual void show_busy() {}
  virtual void clear_screen() {}
  virtual int get_page_width() { return 100; }
  virtual int get_page_height() { return 100; }
  virtual int get_space_width() { return 1; }
  virtual int get_line_height() { return 1; }
  virtual void needs_gray(uint8_t color) {}
};

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
      "<img src=\"test.png\" />"
      "<p>Bananas!</p>"
      "</body>"
      "</html>";
  {
    RubbishHtmlParser parser(html, strlen(html), "");
    parser.layout(new TestRenderer(), new Epub("test"));
    TEST_ASSERT_EQUAL(7, parser.get_blocks().size());
    auto iterator = parser.get_blocks().begin();
    std::advance(iterator, 5);
    Block *img_block = *iterator;
    TEST_ASSERT_EQUAL(BlockType::IMAGE_BLOCK, img_block->getType());
    TEST_ASSERT_EQUAL_STRING("test.png", reinterpret_cast<ImageBlock *>(img_block)->m_src.c_str());
  }
  {
    RubbishHtmlParser parser(html, strlen(html), "HTML/");
    parser.layout(new TestRenderer(), new Epub("test"));
    TEST_ASSERT_EQUAL(7, parser.get_blocks().size());
    auto iterator = parser.get_blocks().begin();
    std::advance(iterator, 5);
    Block *img_block = *iterator;
    TEST_ASSERT_EQUAL(BlockType::IMAGE_BLOCK, img_block->getType());
    TEST_ASSERT_EQUAL_STRING("HTML/test.png", reinterpret_cast<ImageBlock *>(img_block)->m_src.c_str());
  }
}

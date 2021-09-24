#include <unity.h>
#include <string.h>
#include <RubbishHtmlParser/RubbishHtmlParser.h>
#include <RubbishHtmlParser/blocks/Block.h>
#include <Renderer/Renderer.h>

class TestRenderer : public Renderer
{
public:
  virtual void draw_image(const std::string &filename, int x, int y, int width, int height) {}
  virtual bool get_image_size(const std::string &filename, int *width, int *height) { return false; }
  virtual void draw_pixel(int x, int y, uint8_t color) {}
  virtual int get_text_width(const char *text, bool bold = false, bool italic = false)
  {
    printf("Measuring text: %s\n", text);
    return strlen(text);
  }
  virtual void draw_text(int x, int y, const char *text, bool bold = false, bool italic = false) {}
  virtual void draw_text_box(const std::string &text, int x, int y, int width, int height, bool bold = false, bool italic = false) {}
  virtual void draw_rect(int x, int y, int width, int height, uint8_t color = 0){};
  virtual void clear_screen() {}
  virtual int get_page_width() { return 100; }
  virtual int get_page_height() { return 100; }
  virtual int get_space_width() { return 1; }
  virtual int get_line_height() { return 1; }
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
      "<p>Bananas!</p>"
      "</body>"
      "</html>";
  RubbishHtmlParser parser(html, strlen(html));
  parser.layout(new TestRenderer(), nullptr);
  TEST_ASSERT_EQUAL(6, parser.get_blocks().size());
}

# ESP32 Based ePub Reader

You can watch a video of the build [here](https://youtu.be/VLiCgB0odOQ)

[![Demo Video](https://img.youtube.com/vi/VLiCgB0odOQ/0.jpg)](https://www.youtube.com/watch?v=VLiCgB0odOQ)

What is it? It's a DIY ePub reader for the ESP32.

It will parse ePub files that can be downloaded from places such as [Project Gutenberg](https://www.gutenberg.org/).

It has limited support for formating - the CSS content of the ePub file is not parsed, so we just use the standard HTML tags such as `<h1>`,`<h2>` etc.. and `<b>` and `<i>`.

I've only included 4 font styles - regular, bold, italic and bold-italic. I've also only generated glyphs for Latin characters and punctuation.

# Why did you build it?

It seemed like a nice challenge - ePub files are not the most friendly format to process on an embedded device. Making it work in a constrained environment is good fun.

# Can you contribute/help?

Yes - please try the project out on any e-paper boards that you have and open up pull requests if you get it working with any fixes.

And if you find bugs, feel free to report (or better yet, fix!) them :)

# How to get it?

Make sure you clone recursively - the code uses git submodules.

```
git clone --recursive git@github.com:atomic14/esp32-ereader.git
```

# What boards does it work on?

I've tested it on the LilyGo EPD47, but it should work on any eInk display provided it has:

- PSRAM - parsing the ePub files needs a fair amount of memory
- 3 Buttons - these buttons can be active high or low
  - UP - moves up in the list of ePubs or to the previous page when reading
  - DOWN - moves down in the list of ePubs or to the next page when reading
  - SELECT - opens the ePub currently selected ePub file or goes back to the ePub list from reading mode
- An SD Card - you can jury rig an SPI sd card using the instructions here:
- [Optional] A battery if you want it to be portable

# Porting to other boards

All the configuration is in `platformio.ini` using pre-processor directives. If you do add a new board then please create a new section in platofmrio.ini with the appropriate pre-processor directives for your board and open a pull request to add it to the project - I'm happy to answer any questions on this.

The important settings are the following:

The first two settings come from the [vroland/epdiy](https://github.com/vroland/epdiy) library and defined the ePaper display that is being used.

```
; Setup display format and model via build flags
-DCONFIG_EPD_DISPLAY_TYPE_ED047TC1
-DCONFIG_EPD_BOARD_REVISION_LILYGO_T5_47
```

The second three settings are the pins that are used for the buttons. Change these to match your board.

```
; setup the pins to use for navigation
-DBUTTON_UP_GPIO_NUM=GPIO_NUM_34
-DBUTTON_DOWN_GPIO_NUM=GPIO_NUM_39
-DBUTTON_SELECT_GPIO_NUM=GPIO_NUM_35
```

There is also a setting to tell the code if the buttons are active high or low.

```
; buttons are low when pressed
-DBUTONS_ACTIVE_LEVEL=0
```

We have the pins for the SD card. I've got a video on how to hack an SD Card and connect it as a SPI device [here](https://youtu.be/bVru6M862HY)

```
; setup the pins for the SDCard
-DSD_CARD_PIN_NUM_MISO=GPIO_NUM_14
-DSD_CARD_PIN_NUM_MOSI=GPIO_NUM_13
-DSD_CARD_PIN_NUM_CLK=GPIO_NUM_15
-DSD_CARD_PIN_NUM_CS=GPIO_NUM_12
```

And finally we have the ADC channel that the battery voltage divider is connected to:

```
; the adc channel that is connected to the battery voltage divider - this is GPIO_NUM_35
-DBATTERY_ADC_CHANNEL=ADC1_CHANNEL_0
```

# How does it work?

Epub files are a bit of a pain to parse. Despite the file extension `epub`, they are actually zip archives containing multiple files. To read the file I'm using a nice zip library from here: [lbernstrone/miniz](https://github.com/lbernstone/miniz-esp32). This library has been modified to work on the ESP32 with PSRAM. Miniz is actually built into the ESP32 ROM, but support for multifile archives is disabled.

## Parsing the ePub file

I've encapsulated the needed ZIP function in a small wrapper class which can be found here: [ZipFile](https://github.com/atomic14/esp32-ereader/tree/main/lib/Epub/ZipFile)

The most interesting file in the epub archive is the `OEBPS/content.opf` file. This file contains the list of files in the epub archive. The `OEBPS/content.opf` file is a simple XML file so we need an XML parser to parse it.

To do the heavy lifting of parsing the XML we're using [TinyXML2](https://github.com/leethomason/tinyxml2). This library is a very small and simple XML parser. The `content.opf` contains three sections, `metadata`, `manifest` and `spine`. The `metadata` section contains the title of the book, the author and the cover image. The `manifest` section contains the list of files in the epub archive. The `spine` section tells you what order to read the files in.

The parsing of the epub file and reading in the contents is done by the [Epub class](https://github.com/atomic14/esp32-ereader/blob/main/lib/Epub/EpubList/Epub.h). This uses the [ZipFile](https://github.com/atomic14/esp32-ereader/tree/main/lib/Epub/ZipFile) class and the [TinyXML2](https://github.com/leethomason/tinyxml2) library to parse the epub file and read the contents.

## Parsing the ePub contents

Each logical section of the book (typically chapters) is one HTML file in the zip archive.

These are all XHTML files - this means that once again we can parse them using the TinyXML2 library.

I've limited our parsing to a set of minimum tags that are enough to give us the basic structure of the book without making things too complicated.

- Block tags `<div>`, `<p>`, `<h1>`, `<h2>` etc...
- Inline tags `<b>`, `<i>`
- Images `<img>`
- Line breaks `<br>`

You can see the details in the [RubbishHtmlParser](https://github.com/atomic14/esp32-ereader/tree/main/lib/Epub/RubbishHtmlParser) code.

For each block tag we extract the text and add the block to a list of blocks. For inline tags we add these to the current block along with any style information (e.g. bold, italic).

Whenever we hit a new block tag we start a new block.

Image tags are also treated as blocks.

After parsing the HTML we end up with a list of `blocks` containing either text or an image. For header tags, we just set the style of the block to bold. You could get more sophisticated here with multiple fonts and different sizes if you wanted to.

## Laying out a section of the book

With the blocks extracted, we can now layout the blocks of the section onto individual pages.

Image blocks are easy - we just need to read the width and height of the image and scale it to fit on the screen. This gives us the height the image will be when it is rendered.

For text blocks we need to calculate the height of the text once it has been split over multiple lines. To do this we measure the width of each word in the block and then use some dynamic programming to break the words up into lines - you can watch a great video from MIT here on how this algorithm works: [20. Dynamic Programming II: Text Justification, Blackjack](https://www.youtube.com/watch?v=ENyox7kNKeY).

I copied the solution for this problem from [here](https://www.geeksforgeeks.org/word-wrap-problem-dp-19/) with minor modifications. This implementation will work for most cases but could be improved considerably.

With the heights of the blocks all computed and the text blocks broken up into lines, we can now assign the content to pages.

We create a page and then start adding content to it - either images or lines of text. Everytime we run out of space we create a new page.

## Rendering

Rendering each page is trivial - we know the y position of each element on the page.

Images are simply drawn at the correct y position centred on the screen.

Text is drawn word by word and a side effect of the text justification and line-breaking is that we have already computed the x position of each word on a line.

## Deep sleep

The code will go to sleep after 30 seconds of inactivity. The current state of the ePaper display is saved to the SD Card - this is needed so that the display updates correctly. Other state is held in RTC memory.

For wake up there are two options, ULP for buttons that are active low and EXT1 for buttons that are active high. I've got a good video on deep sleep here if you are interested in this kind of thing: [ESP32 Deep Dive into Deep Sleep](https://www.youtube.com/watch?v=YOjgZUg_skU)

# How well does it work?

Surprisingly, it works pretty well. Layout is reasonable, but there are a lot of improvements that could be made. The code makes no attempt to break pages at suitable places - there are hints that can be extracted from the XHTML files and there are also CSS files that could be used.

Depending on the images that have been used for the book covers displaying the list of ePub files on the SD Card can take a few sections and moving through the pages of ePub files can be a bit slow. Rendering the actual pages is pretty reasonable, even when they have images on them.

# Improvements:

There's a lot of room for improvement. This is a very basic e-reader and I'm more than happy for people to contribute to this project.

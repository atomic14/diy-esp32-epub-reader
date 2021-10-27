#include <esp_log.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "M5EPD_Driver.h"

m5epd_err_t __epdret__;
#define CHECK(x)                \
    __epdret__ = x;             \
    if (__epdret__ != M5EPD_OK) \
    {                           \
        return __epdret__;      \
    }

uint32_t M5EPD_Driver::write32(uint32_t data)
{
    spi_transaction_t t = {};
    // memset(&t, 0, sizeof(t)); //Zero out the transaction
    t.length = 32;
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.tx_data[0] = (data >> 24) & 0xFF;
    t.tx_data[1] = (data >> 16) & 0xFF;
    t.tx_data[2] = (data >> 8) & 0xFF;
    t.tx_data[3] = data & 0xFF;
    spi_device_polling_transmit(spi, &t);
    return t.rx_data[0] << 24 | t.rx_data[1] << 16 | t.rx_data[2] << 8 | t.rx_data[3];
}

uint16_t M5EPD_Driver::write16(uint16_t data)
{
    spi_transaction_t t = {};
    // memset(&t, 0, sizeof(t)); //Zero out the transaction
    t.length = 16;
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.tx_data[0] = data >> 8;
    t.tx_data[1] = data & 0xFF;
    spi_device_polling_transmit(spi, &t);
    return t.rx_data[0] << 8 | t.rx_data[1];
}

M5EPD_Driver::M5EPD_Driver()
{
    _pin_cs = GPIO_NUM_NC;
    _pin_busy = GPIO_NUM_NC;
    _pin_sck = GPIO_NUM_NC;
    _pin_mosi = GPIO_NUM_NC;
    _pin_rst = GPIO_NUM_NC;

    _spi_freq = 10000000;

    _rotate = IT8951_ROTATE_0;
    _direction = 1;

    _update_count = false;
    _is_reverse = false;
}

M5EPD_Driver::~M5EPD_Driver()
{
}

m5epd_err_t M5EPD_Driver::begin(gpio_num_t sck, gpio_num_t mosi, gpio_num_t miso, gpio_num_t cs, gpio_num_t busy, gpio_num_t rst)
{
    // setup SPI output
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0};
    spi_device_interface_config_t devcfg = {.command_bits = 0, ///< Default amount of bits in command phase (0-16), used when ``SPI_TRANS_VARIABLE_CMD`` is not used, otherwise ignored.
                                            .address_bits = 0, ///< Default amount of bits in address phase (0-64), used when ``SPI_TRANS_VARIABLE_ADDR`` is not used, otherwise ignored.
                                            .dummy_bits = 0,   ///< Amount of dummy bits to insert between address and data phase
                                            .mode = 0,         //SPI mode 0
                                            .duty_cycle_pos = 0,
                                            .cs_ena_pretrans = 0,
                                            .cs_ena_posttrans = 0,
                                            .clock_speed_hz = 80000000,
                                            .input_delay_ns = 0,
                                            .spics_io_num = -1, //CS pin
                                            .flags = SPI_DEVICE_NO_DUMMY,
                                            .queue_size = 2,
                                            .pre_cb = NULL,
                                            .post_cb = NULL};
    ret = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    if (ret != ESP_OK)
    {
        ESP_LOGI("M5P", "spi_bus_initialize failed %d", ret);
    }
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi);
    if (ret != ESP_OK)
    {
        ESP_LOGI("M5P", "spi_bus_add_device Failed %d", ret);
    }

    _pin_cs = cs;
    _pin_busy = busy;
    _pin_sck = sck;
    _pin_mosi = mosi;
    _pin_miso = miso;
    _pin_rst = rst;
    if (_pin_rst != -1)
    {
        gpio_set_direction(_pin_rst, GPIO_MODE_OUTPUT);
        ResetDriver();
    }
    gpio_reset_pin(_pin_cs);
    gpio_reset_pin(_pin_busy);
    gpio_set_direction(_pin_cs, GPIO_MODE_OUTPUT);
    gpio_set_direction(_pin_busy, GPIO_MODE_INPUT);
    gpio_set_level(_pin_cs, 1);

    _tar_memaddr = 0x001236E0;
    _dev_memaddr_l = 0x36E0;
    _dev_memaddr_h = 0x0012;
    CHECK(WriteCommand(IT8951_TCON_SYS_RUN));
    CHECK(WriteReg(IT8951_I80CPCR, 0x0001)); //enable pack write

    //set vcom to -2.30v
    CHECK(WriteCommand(0x0039)); //tcon vcom set command
    CHECK(WriteWord(0x0001));
    CHECK(WriteWord(2300));

    ESP_LOGI("M5P", "Init SUCCESS.");

    return M5EPD_OK;
}

/** @brief Invert display colors
  * @param is_reverse 1, reverse color; 0, default
  */
void M5EPD_Driver::SetColorReverse(bool is_reverse)
{
    _is_reverse = is_reverse;
}

/** @brief Set panel rotation
  * @param rotate direction to rotate.
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::SetRotation(uint16_t rotate)
{
    if (rotate < 4)
    {
        this->_rotate = rotate;
    }
    else if (rotate < 90)
    {
        this->_rotate = IT8951_ROTATE_0;
    }
    else if (rotate < 180)
    {
        this->_rotate = IT8951_ROTATE_90;
    }
    else if (rotate < 270)
    {
        this->_rotate = IT8951_ROTATE_180;
    }
    else
    {
        this->_rotate = IT8951_ROTATE_270;
    }

    if (_rotate == IT8951_ROTATE_0 || _rotate == IT8951_ROTATE_180)
    {
        _direction = 1;
    }
    else
    {
        _direction = 0;
    }
    return M5EPD_OK;
}

/** @brief Clear graphics buffer
  * @param init Screen initialization, If is 0, clear the buffer without initializing
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::Clear(bool init)
{
    _endian_type = IT8951_LDIMG_L_ENDIAN;
    _pix_bpp = IT8951_4BPP;

    CHECK(SetTargetMemoryAddr(_tar_memaddr));
    if (_direction)
    {
        CHECK(SetArea(0, 0, M5EPD_PANEL_W, M5EPD_PANEL_H));
    }
    else
    {
        CHECK(SetArea(0, 0, M5EPD_PANEL_H, M5EPD_PANEL_W));
    }
    if (_is_reverse)
    {
        for (uint32_t x = 0; x < ((M5EPD_PANEL_W * M5EPD_PANEL_H) >> 2); x++)
        {
            gpio_set_level(_pin_cs, 0);
            write32(0x00000000);
            gpio_set_level(_pin_cs, 1);
        }
    }
    else
    {
        for (uint32_t x = 0; x < ((M5EPD_PANEL_W * M5EPD_PANEL_H) >> 2); x++)
        {
            gpio_set_level(_pin_cs, 0);
            write32(0x0000FFFF);
            gpio_set_level(_pin_cs, 1);
        }
    }

    CHECK(WriteCommand(IT8951_TCON_LD_IMG_END));

    if (init)
    {
        CHECK(UpdateFull(UPDATE_MODE_INIT));
    }

    return M5EPD_OK;
}

/** @brief Write full (960 * 540) 4-bit (16 levels grayscale) image to panel.
  * @param gram pointer to image data.
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::WriteFullGram4bpp(const uint8_t *gram)
{
    if (_direction)
    {
        return WritePartGram4bpp(0, 0, M5EPD_PANEL_W, M5EPD_PANEL_H, gram);
    }
    else
    {
        return WritePartGram4bpp(0, 0, M5EPD_PANEL_H, M5EPD_PANEL_W, gram);
    }
}

/** @brief Write the image at the specified location, Partial update
  * @param x Update X coordinate, >>> Must be a multiple of 4 <<<
  * @param y Update Y coordinate
  * @param w width of gram, >>> Must be a multiple of 4 <<<
  * @param h height of gram
  * @param gram 4bpp garm data
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::WritePartGram4bpp(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *gram)
{
    _endian_type = IT8951_LDIMG_B_ENDIAN;
    _pix_bpp = IT8951_4BPP;

    // rounded up to be multiple of 4
    if (_direction)
    {
        x = (x + 3) & 0xFFFC;
    }
    else
    {
        x = (x + 3) & 0xFFFC;
        y = (y + 3) & 0xFFFC;
    }

    if (w & 0x03)
    {
        ESP_LOGE("M5P", "Gram width %d not a multiple of 4.", w);
        return M5EPD_NOTMULTIPLE4;
    }

    if (_direction)
    {
        if (x > M5EPD_PANEL_W || y > M5EPD_PANEL_H)
        {
            ESP_LOGW("M5P", "Pos (%d, %d) out of bounds.", x, y);
            return M5EPD_OUTOFBOUNDS;
        }
    }
    else
    {
        if (x > M5EPD_PANEL_H || y > M5EPD_PANEL_W)
        {
            ESP_LOGW("M5P", "Pos (%d, %d) out of bounds.", x, y);
            return M5EPD_OUTOFBOUNDS;
        }
    }

    uint32_t pos = 0;
    // uint64_t length = (w / 2) * h;

    uint16_t word = 0;
    CHECK(SetTargetMemoryAddr(_tar_memaddr));
    CHECK(SetArea(x, y, w, h));
    if (_is_reverse)
    {
        for (uint32_t x = 0; x < ((w * h) >> 2); x++)
        {
            word = gram[pos] << 8 | gram[pos + 1];

            gpio_set_level(_pin_cs, 0);
            write32(word);
            gpio_set_level(_pin_cs, 1);
            pos += 2;
        }
    }
    else
    {
        for (uint32_t x = 0; x < ((w * h) >> 2); x++)
        {
            word = gram[pos] << 8 | gram[pos + 1];
            word = 0xFFFF - word;

            gpio_set_level(_pin_cs, 0);
            write32(word);
            gpio_set_level(_pin_cs, 1);
            pos += 2;
        }
    }
    CHECK(WriteCommand(IT8951_TCON_LD_IMG_END));

    return M5EPD_OK;
}

/** @brief Fill the color at the specified location, Partial update
  * @param x Update X coordinate, >>> Must be a multiple of 4 <<<
  * @param y Update Y coordinate
  * @param w width of gram, >>> Must be a multiple of 4 <<<
  * @param h height of gram
  * @param data 4bpp color
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::FillPartGram4bpp(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t data)
{
    _endian_type = IT8951_LDIMG_B_ENDIAN;
    _pix_bpp = IT8951_4BPP;

    // rounded up to be multiple of 4
    // rounded up to be multiple of 4
    if (_direction)
    {
        x = (x + 3) & 0xFFFC;
    }
    else
    {
        x = (x + 3) & 0xFFFC;
        y = (y + 3) & 0xFFFC;
    }

    if (w & 0x03)
    {
        ESP_LOGD("MP5", "Gram width %d not a multiple of 4.", w);
        return M5EPD_NOTMULTIPLE4;
    }

    if (_direction)
    {
        if (x > M5EPD_PANEL_W || y > M5EPD_PANEL_H)
        {
            ESP_LOGD("MP5", "Pos (%d, %d) out of bounds.", x, y);
            return M5EPD_OUTOFBOUNDS;
        }
    }
    else
    {
        if (x > M5EPD_PANEL_H || y > M5EPD_PANEL_W)
        {
            ESP_LOGD("MP5", "Pos (%d, %d) out of bounds.", x, y);
            return M5EPD_OUTOFBOUNDS;
        }
    }

    // uint64_t length = (w / 2) * h;

    CHECK(SetTargetMemoryAddr(_tar_memaddr));
    CHECK(SetArea(x, y, w, h));
    for (uint32_t x = 0; x < ((w * h) >> 2); x++)
    {
        gpio_set_level(_pin_cs, 0);
        write32(data);
        gpio_set_level(_pin_cs, 1);
    }
    CHECK(WriteCommand(IT8951_TCON_LD_IMG_END));

    return M5EPD_OK;
}

/** @brief Full panel update
  * @param mode update mode
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::UpdateFull(m5epd_update_mode_t mode)
{
    if (_direction)
    {
        CHECK(UpdateArea(0, 0, M5EPD_PANEL_W, M5EPD_PANEL_H, mode));
    }
    else
    {
        CHECK(UpdateArea(0, 0, M5EPD_PANEL_H, M5EPD_PANEL_W, mode));
    }

    return M5EPD_OK;
}

/** @brief Check if the device is busy
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::CheckAFSR(void)
{
    int64_t start_time = esp_timer_get_time();
    while (1)
    {
        uint16_t infobuf[1];
        CHECK(WriteCommand(IT8951_TCON_REG_RD));
        CHECK(WriteWord(IT8951_LUTAFSR));
        CHECK(ReadWords(infobuf, 1));
        if (infobuf[0] == 0)
        {
            break;
        }

        if ((esp_timer_get_time() - start_time) / 1000 > 30000)
        {
            ESP_LOGE("M5P", "Device response timeout.");
            return M5EPD_BUSYTIMEOUT;
        }
    }
    return M5EPD_OK;
}

/** @brief Partial panel update
  * @param x Update X coordinate, >>> Must be a multiple of 4 <<<
  * @param y Update Y coordinate
  * @param w width of gram, >>> Must be a multiple of 4 <<<
  * @param h height of gram
  * @param mode update mode
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::UpdateArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h, m5epd_update_mode_t mode)
{
    if (mode == UPDATE_MODE_NONE)
    {
        return M5EPD_OTHERERR;
    }

    // rounded up to be multiple of 4
    if (_direction)
    {
        x = (x + 3) & 0xFFFC;
    }
    else
    {
        x = (x + 3) & 0xFFFC;
        y = (y + 3) & 0xFFFC;
    }

    CHECK(CheckAFSR());

    if (_direction)
    {
        if (x + w > M5EPD_PANEL_W)
        {
            w = M5EPD_PANEL_W - x;
        }
        if (y + h > M5EPD_PANEL_H)
        {
            h = M5EPD_PANEL_H - y;
        }
    }
    else
    {
        if (x + w > M5EPD_PANEL_H)
        {
            w = M5EPD_PANEL_H - x;
        }
        if (y + h > M5EPD_PANEL_W)
        {
            h = M5EPD_PANEL_W - y;
        }
    }

    uint16_t args[7];
    switch (_rotate)
    {
    case IT8951_ROTATE_0:
    {
        args[0] = x;
        args[1] = y;
        args[2] = w;
        args[3] = h;
        break;
    }
    case IT8951_ROTATE_90:
    {
        args[0] = y;
        args[1] = M5EPD_PANEL_H - w - x;
        args[2] = h;
        args[3] = w;
        break;
    }
    case IT8951_ROTATE_180:
    {
        args[0] = M5EPD_PANEL_W - w - x;
        args[1] = M5EPD_PANEL_H - h - y;
        args[2] = w;
        args[3] = h;
        break;
    }
    case IT8951_ROTATE_270:
    {
        args[0] = M5EPD_PANEL_W - h - y;
        args[1] = x;
        args[2] = h;
        args[3] = w;
        break;
    }
    }

    args[4] = mode;
    args[5] = _dev_memaddr_l;
    args[6] = _dev_memaddr_h;

    CHECK(WriteArgs(IT8951_I80_CMD_DPY_BUF_AREA, args, 7));

    _update_count++;

    return M5EPD_OK;
}

/** @brief  Set write area
  * @param x Update X coordinate, >>> Must be a multiple of 4 <<<
  * @param y Update Y coordinate
  * @param w width of gram, >>> Must be a multiple of 4 <<<
  * @param h height of gram
  * @retval m5epd_err_t
  */
m5epd_err_t M5EPD_Driver::SetArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t args[5];
    args[0] = (_endian_type << 8 | _pix_bpp << 4 | _rotate);
    args[1] = x;
    args[2] = y;
    args[3] = w;
    args[4] = h;
    CHECK(WriteArgs(IT8951_TCON_LD_IMG_AREA, args, 5));

    return M5EPD_OK;
}

/** @brief  Write image data to the set address
  * @param data pointer to 4-bpp gram data
  * @retval m5epd_err_t
  */
void M5EPD_Driver::WriteGramData(uint16_t data)
{
    gpio_set_level(_pin_cs, 0);
    write32(data);
    // write16(0x0000);
    // write16(data);
    gpio_set_level(_pin_cs, 1);
}

m5epd_err_t M5EPD_Driver::SetTargetMemoryAddr(uint32_t tar_addr)
{
    uint16_t h = (uint16_t)((tar_addr >> 16) & 0x0000FFFF);
    uint16_t l = (uint16_t)(tar_addr & 0x0000FFFF);

    CHECK(WriteReg(IT8951_LISAR + 2, h));
    CHECK(WriteReg(IT8951_LISAR, l));

    return M5EPD_OK;
}

m5epd_err_t M5EPD_Driver::WriteReg(uint16_t addr, uint16_t data)
{
    CHECK(WriteCommand(0x0011)); //tcon write reg command
    CHECK(WriteWord(addr));
    CHECK(WriteWord(data));
    return M5EPD_OK;
}

m5epd_err_t M5EPD_Driver::GetSysInfo(void)
{
    uint16_t infobuf[20];
    CHECK(WriteCommand(IT8951_I80_CMD_GET_DEV_INFO));
    CHECK(ReadWords(infobuf, 20));
    _dev_memaddr_l = infobuf[2];
    _dev_memaddr_h = infobuf[3];
    _tar_memaddr = (_dev_memaddr_h << 16) | _dev_memaddr_l;
    ESP_LOGD("M5P", "memory addr = %04X%04X", _dev_memaddr_h, _dev_memaddr_l);
    return M5EPD_OK;
}

void M5EPD_Driver::ResetDriver(void)
{
    gpio_set_level(_pin_rst, 1);
    gpio_set_level(_pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(_pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}

m5epd_err_t M5EPD_Driver::WaitBusy(uint32_t timeout)
{
    int64_t start_time = esp_timer_get_time();
    while (1)
    {
        if (gpio_get_level(_pin_busy) == 1)
        {
            return M5EPD_OK;
        }

        if ((esp_timer_get_time() - start_time) / 1000 > timeout)
        {
            ESP_LOGE("M5P", "Device response timeout.");
            return M5EPD_BUSYTIMEOUT;
        }
    }
}

m5epd_err_t M5EPD_Driver::WriteCommand(uint16_t cmd)
{
    CHECK(WaitBusy());
    gpio_set_level(_pin_cs, 0);
    write16(0x6000);
    CHECK(WaitBusy());
    write16(cmd);
    gpio_set_level(_pin_cs, 1);

    return M5EPD_OK;
}

m5epd_err_t M5EPD_Driver::WriteWord(uint16_t data)
{
    CHECK(WaitBusy());
    gpio_set_level(_pin_cs, 0);
    write16(0x0000);
    CHECK(WaitBusy());
    write16(data);
    gpio_set_level(_pin_cs, 1);

    return M5EPD_OK;
}

m5epd_err_t M5EPD_Driver::ReadWords(uint16_t *buf, uint32_t length)
{
    // uint16_t dummy;
    CHECK(WaitBusy());
    gpio_set_level(_pin_cs, 0);
    write16(0x1000);
    CHECK(WaitBusy());

    //dummy
    write16(0);
    CHECK(WaitBusy());

    for (size_t i = 0; i < length; i++)
    {
        buf[i] = write16(0);
    }

    gpio_set_level(_pin_cs, 1);
    return M5EPD_OK;
}

m5epd_err_t M5EPD_Driver::WriteArgs(uint16_t cmd, uint16_t *args, uint16_t length)
{
    CHECK(WriteCommand(cmd));
    for (uint16_t i = 0; i < length; i++)
    {
        CHECK(WriteWord(args[i]));
    }
    return M5EPD_OK;
}

uint16_t M5EPD_Driver::UpdateCount(void)
{
    return _update_count;
}

void M5EPD_Driver::ResetUpdateCount(void)
{
    _update_count = 0;
}

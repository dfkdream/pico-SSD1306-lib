#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define SSD1306_SET_MEM_MODE        _u(0x20)
#define SSD1306_SET_COL_ADDR        _u(0x21)
#define SSD1306_SET_PAGE_ADDR       _u(0x22)
#define SSD1306_SET_HORIZ_SCROLL    _u(0x26)
#define SSD1306_SET_SCROLL          _u(0x2E)

#define SSD1306_SET_DISP_START_LINE _u(0x40)

#define SSD1306_SET_CONTRAST        _u(0x81)
#define SSD1306_SET_CHARGE_PUMP     _u(0x8D)

#define SSD1306_SET_SEG_REMAP       _u(0xA0)
#define SSD1306_SET_ENTIRE_ON       _u(0xA4)
#define SSD1306_SET_ALL_ON          _u(0xA5)
#define SSD1306_SET_NORM_DISP       _u(0xA6)
#define SSD1306_SET_INV_DISP        _u(0xA7)
#define SSD1306_SET_MUX_RATIO       _u(0xA8)
#define SSD1306_SET_DISP            _u(0xAE)
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0)

#define SSD1306_SET_DISP_OFFSET     _u(0xD3)
#define SSD1306_SET_DISP_CLK_DIV    _u(0xD5)
#define SSD1306_SET_PRECHARGE       _u(0xD9)
#define SSD1306_SET_COM_PIN_CFG     _u(0xDA)
#define SSD1306_SET_VCOM_DESEL      _u(0xDB)

#define SSD1306_WRITE_MODE         _u(0xFE)
#define SSD1306_READ_MODE          _u(0xFF)

#if SSD1306_I2C_INSTANCE == 0
#define _SSD1306_I2C_INSTANCE i2c0
#else
#define _SSD1306_I2C_INSTANCE i2c1
#endif

void SSD1306_init(){
    // I2C is "open drain", pull ups to keep signal high when no data is being
    // sent
    i2c_init(_SSD1306_I2C_INSTANCE, SSD1306_I2C_CLK * 1000);
    gpio_set_function(SSD1306_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SSD1306_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SSD1306_I2C_SDA_PIN);
    gpio_pull_up(SSD1306_I2C_SCL_PIN);

    uint8_t cmds[] = {
        SSD1306_SET_DISP,               // set display off
        /* memory mapping */
        SSD1306_SET_MEM_MODE,           // set memory address mode 0 = horizontal, 1 = vertical, 2 = page
        0x00,                           // horizontal addressing mode
        /* resolution and layout */
        SSD1306_SET_DISP_START_LINE,    // set display start line to 0
#if (SSD1306_UPSIDE_DOWN == 0)
        SSD1306_SET_SEG_REMAP | 0x00,
        SSD1306_SET_COM_OUT_DIR | 0x00,
#else
        SSD1306_SET_SEG_REMAP | 0x01,   // set segment re-map, column address 127 is mapped to SEG0
        SSD1306_SET_COM_OUT_DIR | 0x08, // set COM (common) output scan direction. Scan from bottom up, COM[N-1] to COM0
#endif
        SSD1306_SET_MUX_RATIO,          // set multiplex ratio
        SSD1306_HEIGHT - 1,             // Display height - 1
        SSD1306_SET_DISP_OFFSET,        // set display offset
        0x00,                           // no offset
        SSD1306_SET_COM_PIN_CFG,        // set COM (common) pins hardware configuration. Board specific magic number. 
                                        // 0x02 Works for 128x32, 0x12 Possibly works for 128x64. Other options 0x22, 0x32
#if ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 32))
        0x02,                           
#elif ((SSD1306_WIDTH == 128) && (SSD1306_HEIGHT == 64))
        0x12,
#else
        0x02,
#endif
        /* timing and driving scheme */
        SSD1306_SET_DISP_CLK_DIV,       // set display clock divide ratio
        0x80,                           // div ratio of 1, standard freq
        SSD1306_SET_PRECHARGE,          // set pre-charge period
        0xF1,                           // Vcc internally generated on our board
        SSD1306_SET_VCOM_DESEL,         // set VCOMH deselect level
        0x30,                           // 0.83xVcc
        /* display */
        SSD1306_SET_CONTRAST,           // set contrast control
        0xFF,
        SSD1306_SET_ENTIRE_ON,          // set entire display on to follow RAM content
        SSD1306_SET_NORM_DISP,           // set normal (not inverted) display
        SSD1306_SET_CHARGE_PUMP,        // set charge pump
        0x14,                           // Vcc internally generated on our board
        SSD1306_SET_SCROLL | 0x00,      // deactivate horizontal scrolling if set. This is necessary as memory writes will corrupt if scrolling was enabled
        SSD1306_SET_DISP | 0x01, // turn display on
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_send_cmd(uint8_t cmd) {
    // I2C write process expects a control byte followed by data
    // this "data" can be a command or data to follow up a command
    // Co = 1, D/C = 0 => the driver expects a command
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(_SSD1306_I2C_INSTANCE, SSD1306_I2C_ADDR, buf, 2, false);
}

void SSD1306_send_cmd_list(uint8_t *list, int len) {
    for (int i=0;i<len;i++)
        SSD1306_send_cmd(list[i]);
}

void SSD1306_send_buf(uint8_t buf[], int len) {
    // in horizontal addressing mode, the column address pointer auto-increments
    // and then wraps around to the next page, so we can send the entire frame
    // buffer in one gooooooo!

    // copy our frame buffer into a new buffer because we need to add the control byte
    // to the beginning

    uint8_t *temp_buf = malloc(len + 1);

    temp_buf[0] = 0x40;
    memcpy(temp_buf+1, buf, len);

    i2c_write_blocking(_SSD1306_I2C_INSTANCE, SSD1306_I2C_ADDR, temp_buf, len + 1, false);

    free(temp_buf);
}

void SSD1306_render(uint8_t *buf, struct render_area *area) {
    // update a portion of the display with a render area
    uint8_t cmds[] = {
        SSD1306_SET_COL_ADDR,
        area->start_col,
        area->end_col,
        SSD1306_SET_PAGE_ADDR,
        area->start_page,
        area->end_page
    };
    
    SSD1306_send_cmd_list(cmds, count_of(cmds));
    SSD1306_send_buf(buf, area->buflen);
}

void SSD1306_scroll(bool enabled, enum SSD1306_scroll_interval interval) {
    // configure horizontal scrolling
    uint8_t cmds[] = {
        SSD1306_SET_HORIZ_SCROLL | 0x00,
        0x00, // dummy byte
        0x00, // start page 0
        interval, // time interval
        //0x03, // end page 3 SSD1306_NUM_PAGES ??
        SSD1306_NUM_PAGES-1,
        0x00, // dummy byte
        0xFF, // dummy byte
        SSD1306_SET_SCROLL | (enabled ? 0x01 : 0) // Start/stop scrolling
    };

    SSD1306_send_cmd_list(cmds, count_of(cmds));
}

void SSD1306_invert(bool enabled){
    SSD1306_send_cmd(enabled?SSD1306_SET_INV_DISP:SSD1306_SET_NORM_DISP);
}

void SSD1306_all_pixels_on(bool enabled){
    SSD1306_send_cmd(enabled?SSD1306_SET_ALL_ON:SSD1306_SET_ENTIRE_ON);
}

void SSD1306_set_contrast(uint8_t contrast){
    SSD1306_send_cmd(SSD1306_SET_CONTRAST);
    SSD1306_send_cmd(contrast);
}

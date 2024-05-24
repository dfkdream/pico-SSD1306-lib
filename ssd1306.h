#ifndef SSD1306_H
#define SSD1306_H

#include "ssd1306_config.h"
#include <stdbool.h>

#define SSD1306_PAGE_HEIGHT         _u(8)
#define SSD1306_NUM_PAGES           (SSD1306_HEIGHT / SSD1306_PAGE_HEIGHT)
#define SSD1306_BUF_LEN             (SSD1306_NUM_PAGES * SSD1306_WIDTH)

struct render_area {
    uint8_t start_col;
    uint8_t end_col;
    uint8_t start_page;
    uint8_t end_page;

    int buflen;
};

void SSD1306_init();
void SSD1306_send_cmd(uint8_t cmd);
void SSD1306_send_cmd_list(uint8_t *list, int len);
void SSD1306_send_buf(uint8_t buf[], int len);

void SSD1306_render(uint8_t *buf, struct render_area *area);

enum SSD1306_scroll_interval{
    SSD1306_SCROLL_INTERVAL_5,
    SSD1306_SCROLL_INTERVAL_64,
    SSD1306_SCROLL_INTERVAL_128,
    SSD1306_SCROLL_INTERVAL_256,
    SSD1306_SCROLL_INTERVAL_3,
    SSD1306_SCROLL_INTERVAL_4,
    SSD1306_SCROLL_INTERVAL_25,
    SSD1306_SCROLL_INTERVAL_2,
};

void SSD1306_scroll(bool enabled, enum SSD1306_scroll_interval interval);
void SSD1306_invert(bool enabled);
void SSD1306_all_pixels_on(bool enabled);
void SSD1306_set_contrast(uint8_t contrast);

#endif
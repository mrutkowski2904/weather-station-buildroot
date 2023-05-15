#ifndef _DISPLAY_IO_H
#define _DISPLAY_IO_H

#include <linux/i2c.h>

#define SSD1306_COLUMNS 128
#define SSD1306_PAGES 8
#define SSD1306_DISPLAY_BUFFER_SIZE (SSD1306_PAGES * SSD1306_COLUMNS)
#define SSD1306_LINE_LENGTH (SSD1306_WIDTH / 8)

#define SSD1306_WIDTH SSD1306_COLUMNS
#define SSD1306_HEIGHT (SSD1306_PAGES * 8)

#define SSD1306_MAX_X (SSD1306_COLUMNS - 1)
#define SSD1306_MAX_Y (SSD1306_HEIGHT - 1)

/* memory map / command list */
/* display off */
#define SSD1306_DISPLAY_OFF_CMD 0xAE
/* memory addressing */
#define SSD1306_MEM_ADDR_MODE_CMD 0x20
#define SSD1306_MEM_ADDR_MODE_HORIZONTAL 0x00
/* color normal/inverse */
#define SSD1306_NON_INVERSE_COLORS_CMD 0xA6
/* multiplexing ratio */
#define SSD1306_MULTIPLEX_RATIO_CMD 0xA8
#define SSD1306_MULTIPLEX_RATIO_VALUE 0x3F
/* display clock divide ratio */
#define SSD1306_CLK_DIVIDE_RATIO_CMD 0xD5
#define SSD1306_CLK_DIVIDE_RATIO_VALUE 0x80
/* precharge period */
#define SSD1306_PRECHARGE_PERIOD_CMD 0xD9
#define SSD1306_PRECHARGE_PERIOD_VALUE 0x22
/* COM hardware config */
#define SSD1306_COM_CMD 0xDA
#define SSD1306_COM_VALUE 0x12
/* Vcomh regulator output */
#define SSD1306_VCOMH_OUTPUT_CMD 0xDB
#define SSD1306_VCOMH_OUTPUT_VALUE 0x40
/* charge pump */
#define SSD1306_CHARGE_PUMP_CMD 0x8D
#define SSD1306_CHARGE_PUMP_VALUE 0x14
/* display offset */
#define SSD1306_DISPLAY_OFFSET_CMD 0xD3
#define SSD1306_DISPLAY_OFFSET_VALUE 0x00
/* column start and end address */
#define SSD1306_COL_START_END_ADDR_CMD 0x21
#define SSD1306_COL_START_END_ADDR_VALUE_1 0
#define SSD1306_COL_START_END_ADDR_VALUE_2 127
/* page start and end address */
#define SSD1306_PAGE_START_END_ADDR_CMD 0x22
#define SSD1306_PAGE_START_END_ADDR_VALUE_1 0
#define SSD1306_PAGE_START_END_ADDR_VALUE_2 7
/* display on */
#define SSD1306_DISPLAY_ON_CMD1 0xA4
#define SSD1306_DISPLAY_ON_CMD2 0xAF

#define SSD1306_COMMAND 0x00
#define SSD1306_DATA 0x40

int ssd1306_configure_hardware(struct i2c_client *i2c_client);
int ssd1306_write_command(struct i2c_client *i2c_client, u8 *buff, u16 len);
int ssd1306_write_data(struct i2c_client *i2c_client, u8 *buff, u16 len);
void ssd1306_set_pixel(u8 *display_buffer, u8 x, u8 y, u8 value);
void ssd1306_map_fb_to_display_buffer(u8 *display_buffer, u8 *fb);

#endif /* _DISPLAY_IO_H */
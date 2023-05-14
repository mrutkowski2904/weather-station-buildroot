#include <linux/types.h>

#include "display_io.h"

static void ssd1306_modify_at(u8 *display_buffer, u8 column, u8 page, u8 value);
static u8 ssd1306_get_at(u8 *display_buffer, u8 column, u8 page);

/* it's possible to send all settings in one transaction */
static u8 init_commands[] = {
    SSD1306_DISPLAY_OFF_CMD,

    SSD1306_MEM_ADDR_MODE_CMD,
    SSD1306_MEM_ADDR_MODE_HORIZONTAL,

    SSD1306_NON_INVERSE_COLORS_CMD,

    SSD1306_MULTIPLEX_RATIO_CMD,
    SSD1306_MULTIPLEX_RATIO_VALUE,

    SSD1306_CLK_DIVIDE_RATIO_CMD,
    SSD1306_CLK_DIVIDE_RATIO_VALUE,

    SSD1306_PRECHARGE_PERIOD_CMD,
    SSD1306_PRECHARGE_PERIOD_VALUE,

    SSD1306_COM_CMD,
    SSD1306_COM_VALUE,

    SSD1306_VCOMH_OUTPUT_CMD,
    SSD1306_VCOMH_OUTPUT_VALUE,

    SSD1306_CHARGE_PUMP_CMD,
    SSD1306_CHARGE_PUMP_VALUE,

    SSD1306_DISPLAY_OFFSET_CMD,
    SSD1306_DISPLAY_OFFSET_VALUE,

    SSD1306_COL_START_END_ADDR_CMD,
    SSD1306_COL_START_END_ADDR_VALUE_1,
    SSD1306_COL_START_END_ADDR_VALUE_2,

    SSD1306_PAGE_START_END_ADDR_CMD,
    SSD1306_PAGE_START_END_ADDR_VALUE_1,
    SSD1306_PAGE_START_END_ADDR_VALUE_2,

    SSD1306_DISPLAY_ON_CMD1,
    SSD1306_DISPLAY_ON_CMD2,
};

int ssd1306_configure_hardware(struct i2c_client *i2c_client)
{
    return ssd1306_write_command(i2c_client, init_commands, sizeof(init_commands));
}

int ssd1306_write_command(struct i2c_client *i2c_client, u8 *buff, u16 len)
{
    int status;
    status = i2c_smbus_write_i2c_block_data(i2c_client, SSD1306_COMMAND, len, buff);
    if (status < 0)
        return status;
    return 0;
}

int ssd1306_write_data(struct i2c_client *i2c_client, u8 *buff, u16 len)
{
    int status;
    /* i2c_smbus_write_i2c_block_data returns correct value, but doesn't write everyting to i2c bus (bug ?) */
    for (int i = 0; i < len; i++)
    {
        status = i2c_smbus_write_byte_data(i2c_client, SSD1306_DATA, buff[i]);
        if (status)
            return status;
    }
    return 0;
}

void ssd1306_set_pixel(u8 *display_buffer, u8 x, u8 y, u8 value)
{
    if ((x < 0 || x > SSD1306_MAX_X) || (y < 0 || y > SSD1306_MAX_Y))
    {
        return;
    }

    u8 data = ssd1306_get_at(display_buffer, x, y / 8);

    if (value)
    {
        data |= (1 << (7 - (y % SSD1306_PAGES)));
    }
    else
    {
        data &= ~(1 << (7 - (y % SSD1306_PAGES)));
    }
    ssd1306_modify_at(display_buffer, x, y / 8, data);
}

static void ssd1306_modify_at(u8 *display_buffer, u8 column, u8 page, u8 value)
{
    page = (SSD1306_PAGES - 1) - page;
    column = (SSD1306_COLUMNS - 1) - column;
    display_buffer[(SSD1306_COLUMNS * page) + column] = value;
}

static u8 ssd1306_get_at(u8 *display_buffer, u8 column, u8 page)
{
    page = (SSD1306_PAGES - 1) - page;
    column = (SSD1306_COLUMNS - 1) - column;
    return display_buffer[(SSD1306_COLUMNS * page) + column];
}
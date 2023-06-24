#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#define NUM_OF_GPIOS 8

#define MCP23008_IODIR 0x00
#define MCP23008_IPOL 0x01
#define MCP23008_GPINTEN 0x02
#define MCP23008_DEFVAL 0x03
#define MCP23008_INTCON 0x04
#define MCP23008_IOCON 0x05
#define MCP23008_GPPU 0x06
#define MCP23008_INTF 0x07
#define MCP23008_INTCAP 0x08
#define MCP23008_GPIO 0x09
#define MCP23008_OLAT 0x0A

struct mcp23008_context
{
    struct i2c_client *client;
    struct regmap *regmap;
    struct regmap_config regmap_cfg;
    struct gpio_chip gpiochip;

    u8 irq_enable_mask;
    struct semaphore irq_enable_sem;
    struct work_struct irq_set_enable_work;

    u8 irq_types_mask;
    struct semaphore irq_types_sem;
    struct work_struct irq_set_types_work;
};

/* chip operations */
static int mcp23008_get_value(struct gpio_chip *chip, unsigned int offset);
static void mcp23008_set_value(struct gpio_chip *chip, unsigned int offset, int val);
static int mcp23008_direction_output(struct gpio_chip *chip, unsigned int offset, int val);
static int mcp23008_direction_input(struct gpio_chip *chip, unsigned int offset);
static void mcp23008_irq_mask(struct irq_data *data);
static void mcp23008_irq_unmask(struct irq_data *data);
static int mcp23008_irq_set_type(struct irq_data *data, unsigned int flow_type);

static void mcp23008_irq_set_enable_work_cb(struct work_struct *work);
static void mcp23008_irq_set_types_work_cb(struct work_struct *work);

static irqreturn_t mcp23008_irq_handler(int irq, void *data);

static int mcp23008_direction(struct gpio_chip *chip, unsigned int offset, bool is_input, int val);

static bool mcp23008_regmap_is_precious_reg(struct device *dev, unsigned int reg);

static int mcp23008_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int mcp23008_remove(struct i2c_client *client);

/* dt match */
struct of_device_id mcp23008_dt_match[] = {
    {.compatible = "mr,custmcp23008"},
    {},
};
MODULE_DEVICE_TABLE(of, mcp23008_dt_match);

struct i2c_device_id mcp23008_ids[] = {
    {"custmcp23008", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, mcp23008_ids);

struct i2c_driver mcp23008_i2c_driver = {
    .probe = mcp23008_probe,
    .remove = mcp23008_remove,
    .id_table = mcp23008_ids,
    .driver = {
        .name = "custmcp23008",
        .of_match_table = of_match_ptr(mcp23008_dt_match),
    },
};

/* writeable ranges */
static const struct regmap_range mcp23008_wr_range[] = {
    {
        .range_min = MCP23008_IODIR,
        .range_max = MCP23008_GPPU,
    },
    {
        .range_min = MCP23008_GPIO,
        .range_max = MCP23008_OLAT,
    },
};

struct regmap_access_table mcp23008_wr_table = {
    .yes_ranges = mcp23008_wr_range,
    .n_yes_ranges = ARRAY_SIZE(mcp23008_wr_range),
};

/* readable range */
static const struct regmap_range mcp23008_rd_range[] = {
    {
        .range_min = MCP23008_IODIR,
        .range_max = MCP23008_OLAT,
    },
};

static struct regmap_access_table mcp23008_rd_table = {
    .yes_ranges = mcp23008_rd_range,
    .n_yes_ranges = ARRAY_SIZE(mcp23008_rd_range),
};

static struct irq_chip mcp23008_irq_chip = {
    .name = "mcp23008",
    .flags = IRQCHIP_SET_TYPE_MASKED,
    .irq_mask = mcp23008_irq_mask,
    .irq_unmask = mcp23008_irq_unmask,
    .irq_set_type = mcp23008_irq_set_type,
};

static int mcp23008_get_value(struct gpio_chip *chip, unsigned int offset)
{
    int status;
    unsigned int val;
    struct mcp23008_context *ctx;

    ctx = gpiochip_get_data(chip);
    status = regmap_read(ctx->regmap, MCP23008_GPIO, &val);
    if (status)
        return status;

    return (val >> offset) & 0x01;
}

static void mcp23008_set_value(struct gpio_chip *chip, unsigned int offset, int val)
{
    int status;
    struct mcp23008_context *ctx;

    ctx = gpiochip_get_data(chip);
    status = regmap_update_bits(ctx->regmap, MCP23008_GPIO, (1 << offset), (val << offset));
    if (status)
        dev_err(&ctx->client->dev, "error while setting gpio value\n");
}

static int mcp23008_direction_output(struct gpio_chip *chip, unsigned int offset, int val)
{
    return mcp23008_direction(chip, offset, false, val);
}

static int mcp23008_direction_input(struct gpio_chip *chip, unsigned int offset)
{
    return mcp23008_direction(chip, offset, true, 0);
}

static void mcp23008_irq_mask(struct irq_data *data)
{
    struct gpio_chip *gc;
    struct mcp23008_context *ctx;
    irq_hw_number_t irq_num;

    gc = irq_data_get_irq_chip_data(data);
    ctx = gpiochip_get_data(gc);
    irq_num = irqd_to_hwirq(data);

    if (down_interruptible(&ctx->irq_enable_sem))
        return;
    ctx->irq_enable_mask &= ~(1 << irq_num);
    up(&ctx->irq_enable_sem);

    schedule_work(&ctx->irq_set_enable_work);
}

static void mcp23008_irq_unmask(struct irq_data *data)
{
    struct gpio_chip *gc;
    struct mcp23008_context *ctx;
    irq_hw_number_t irq_num;

    gc = irq_data_get_irq_chip_data(data);
    ctx = gpiochip_get_data(gc);
    irq_num = irqd_to_hwirq(data);

    if (down_interruptible(&ctx->irq_enable_sem))
        return;
    ctx->irq_enable_mask |= (1 << irq_num);
    up(&ctx->irq_enable_sem);

    schedule_work(&ctx->irq_set_enable_work);
}

static int mcp23008_irq_set_type(struct irq_data *data, unsigned int flow_type)
{
    int status;
    struct gpio_chip *gc;
    struct mcp23008_context *ctx;
    irq_hw_number_t irq_num;

    gc = irq_data_get_irq_chip_data(data);
    ctx = gpiochip_get_data(gc);
    irq_num = irqd_to_hwirq(data);
    status = 0;

    status = down_interruptible(&ctx->irq_types_sem);
    if (status)
        return status;

    switch (flow_type)
    {
    case IRQ_TYPE_LEVEL_LOW:
    case IRQ_TYPE_EDGE_FALLING:
        ctx->irq_types_mask |= (1 << irq_num);
        break;
    case IRQ_TYPE_LEVEL_HIGH:
    case IRQ_TYPE_EDGE_RISING:
        ctx->irq_types_mask &= ~(1 << irq_num);
        break;
    default:
        up(&ctx->irq_types_sem);
        return -EINVAL;
    }

    up(&ctx->irq_types_sem);
    schedule_work(&ctx->irq_set_types_work);
    return status;
}

static void mcp23008_irq_set_enable_work_cb(struct work_struct *work)
{
    u8 enable_mask;
    struct mcp23008_context *ctx;
    ctx = container_of(work, struct mcp23008_context, irq_set_enable_work);

    if (down_interruptible(&ctx->irq_enable_sem))
        return;
    enable_mask = ctx->irq_enable_mask;
    up(&ctx->irq_enable_sem);

    if (regmap_write(ctx->regmap, MCP23008_GPINTEN, enable_mask))
        dev_err(&ctx->client->dev, "error while applying interrupt mask\n");
}

static void mcp23008_irq_set_types_work_cb(struct work_struct *work)
{
    u8 types_mask;
    struct mcp23008_context *ctx;
    ctx = container_of(work, struct mcp23008_context, irq_set_types_work);

    if (down_interruptible(&ctx->irq_types_sem))
        return;
    types_mask = ctx->irq_types_mask;
    up(&ctx->irq_types_sem);

    if (regmap_write(ctx->regmap, MCP23008_DEFVAL, types_mask))
        dev_err(&ctx->client->dev, "error while setting interrupt types\n");
}

static irqreturn_t mcp23008_irq_handler(int irq, void *data)
{
    int read_value;
    int status;
    unsigned long i, irq_flags, child_irq;
    struct mcp23008_context *ctx;

    ctx = data;
    status = regmap_read(ctx->regmap, MCP23008_INTF, &read_value);
    if (status)
    {
        dev_err_ratelimited(&ctx->client->dev, "error while accessing the device in interrupt handler\n");
        return IRQ_HANDLED;
    }

    irq_flags = read_value;
    for_each_set_bit(i, &irq_flags, NUM_OF_GPIOS)
    {
        child_irq = irq_find_mapping(ctx->gpiochip.irq.domain, i);
        handle_nested_irq(child_irq);
    }

    /* reset the interrupt flags */
    status = regmap_read(ctx->regmap, MCP23008_INTCAP, &read_value);
    if (status)
    {
        dev_err_ratelimited(&ctx->client->dev, "error while resetting the interrupt\n");
        return IRQ_HANDLED;
    }

    return IRQ_HANDLED;
}

static int mcp23008_direction(struct gpio_chip *chip, unsigned int offset, bool is_input, int val)
{
    int status;
    u8 direction_bit;
    struct mcp23008_context *ctx;

    ctx = gpiochip_get_data(chip);
    direction_bit = (u8)is_input;
    status = regmap_update_bits(ctx->regmap, MCP23008_IODIR, (1 << offset), (direction_bit << offset));
    if (status)
        return status;

    if (!is_input)
        mcp23008_set_value(chip, offset, val);

    return 0;
}

static bool mcp23008_regmap_is_precious_reg(struct device *dev, unsigned int reg)
{
    if ((reg == MCP23008_INTCAP) || (reg == MCP23008_GPIO))
        return true;
    return false;
}

static int mcp23008_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int status;
    struct mcp23008_context *ctx;

    ctx = devm_kzalloc(&client->dev, sizeof(struct mcp23008_context), GFP_KERNEL);
    if (ctx == NULL)
        return -ENOMEM;

    i2c_set_clientdata(client, ctx);
    ctx->client = client;

    sema_init(&ctx->irq_enable_sem, 1);
    sema_init(&ctx->irq_types_sem, 1);

    INIT_WORK(&ctx->irq_set_enable_work, mcp23008_irq_set_enable_work_cb);
    INIT_WORK(&ctx->irq_set_types_work, mcp23008_irq_set_types_work_cb);

    ctx->regmap_cfg.reg_bits = 8;
    ctx->regmap_cfg.val_bits = 8;
    ctx->regmap_cfg.max_register = MCP23008_OLAT;
    ctx->regmap_cfg.wr_table = &mcp23008_wr_table;
    ctx->regmap_cfg.rd_table = &mcp23008_rd_table;
    ctx->regmap_cfg.precious_reg = mcp23008_regmap_is_precious_reg;
    ctx->regmap_cfg.use_single_read = true;
    ctx->regmap_cfg.use_single_write = true;
    ctx->regmap_cfg.can_sleep = true;
    ctx->regmap_cfg.cache_type = REGCACHE_NONE;
    ctx->regmap = devm_regmap_init_i2c(client, &ctx->regmap_cfg);
    if (IS_ERR(ctx->regmap))
        return PTR_ERR(ctx->regmap);

    /* configure the expander */
    /* interrupt-on-change mode */
    status = regmap_write(ctx->regmap, MCP23008_INTCON, 0xff);
    if (status)
        return status;

    /* interrupt pin as open-drain */
    status = regmap_write(ctx->regmap, MCP23008_IOCON, (1 << 2));
    if (status)
        return status;

    ctx->gpiochip.label = client->name;
    ctx->gpiochip.base = -1;
    ctx->gpiochip.owner = THIS_MODULE;
    ctx->gpiochip.can_sleep = true;
    ctx->gpiochip.ngpio = NUM_OF_GPIOS;
    ctx->gpiochip.get = mcp23008_get_value;
    ctx->gpiochip.set = mcp23008_set_value;
    ctx->gpiochip.direction_input = mcp23008_direction_input;
    ctx->gpiochip.direction_output = mcp23008_direction_output;

    ctx->gpiochip.irq.chip = &mcp23008_irq_chip;
    ctx->gpiochip.irq.parent_handler = NULL;
    ctx->gpiochip.irq.num_parents = 0;
    ctx->gpiochip.irq.parents = NULL;
    ctx->gpiochip.irq.default_type = IRQ_TYPE_NONE;
    ctx->gpiochip.irq.handler = handle_level_irq;
    ctx->gpiochip.irq.threaded = true;

    status = devm_request_threaded_irq(&client->dev,
                                       client->irq,
                                       NULL,
                                       mcp23008_irq_handler,
                                       IRQF_ONESHOT,
                                       dev_name(&client->dev),
                                       ctx);
    if (status)
        return status;

    status = devm_gpiochip_add_data(&client->dev, &ctx->gpiochip, ctx);
    if (status)
        return status;

    dev_info(&client->dev, "mcp23008 gpio expander inserted\n");

    return 0;
}

static int mcp23008_remove(struct i2c_client *client)
{
    struct mcp23008_context *ctx;
    ctx = i2c_get_clientdata(client);
    cancel_work_sync(&ctx->irq_set_enable_work);
    cancel_work_sync(&ctx->irq_set_types_work);
    dev_info(&client->dev, "mcp23008 gpio expander removed\n");
    return 0;
}

module_i2c_driver(mcp23008_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maciej Rutkowski");
MODULE_DESCRIPTION("Driver for MCP23008 GPIO expander");
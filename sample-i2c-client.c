/* drivers/input/touch/sample-i2c-client.c
 *
 * Copyright (C) 2011 PointChips, inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that is will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABLILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Genernal Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

struct sample_device {
	struct i2c_client *client;
};

static int sample_send(struct i2c_client *client)
{
	struct i2c_msg msg;
	static char sample_data[] = {0xaf, 0x66, 0x11, 0x22};

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = sizeof(sample_data);
	msg.buf = sample_data;

	return i2c_transfer(client->adapter, &msg, 1);
}

static int sample_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct sample_device *dev;

	if (!i2c_check_functionality(client->adapter,
		I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA |
		I2C_FUNC_SMBUS_I2C_BLOCK)) {
		printk(KERN_ERR "%s: needed i2c functionality is not supported\n", __func__);
		return -ENODEV;
	}

	dev = devm_kzalloc(&client->dev, sizeof(struct sample_device), GFP_KERNEL);

	if (dev == NULL) {
		printk(KERN_ERR "%s: no memory\n", __func__);
		return -ENOMEM;
	}

	dev->client = client;
	i2c_set_clientdata(client, dev);

	if (sample_send(client) < 0) {
		dev_err(&client->dev, "sample_send failed\n");
	}

	return 0;
}

static int sample_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id sample_i2c_id[] = {
	{ "sample_i2c_client", 0 },
	{ }
};

static struct i2c_driver sample_i2c_driver = {
	.probe    = sample_i2c_probe,
	.remove   = sample_i2c_remove,
	.id_table = sample_i2c_id,
	.driver   = {
		.name = "sample_i2c_client",
	},
};

static int __init sample_i2c_init_driver(void)
{
	return i2c_add_driver(&sample_i2c_driver);
}

static void __exit sample_i2c_exit_driver(void)
{
	i2c_del_driver(&sample_i2c_driver);
}

module_init(sample_i2c_init_driver);
module_exit(sample_i2c_exit_driver);

MODULE_DESCRIPTION("Sample I2C client driver");
MODULE_LICENSE("GPL");

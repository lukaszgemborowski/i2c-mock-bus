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

static ssize_t
test1_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct sample_device *data = dev_get_drvdata(dev);

	struct i2c_msg msg;

	msg.addr = buf[0];
	msg.flags = 0;
	msg.len = count - 1;
	msg.buf = (char *)buf + 1;

	if (i2c_transfer(data->client->adapter, &msg, 1) > 0)
		return count;
	else
		return -1;
}

static DEVICE_ATTR(test1, S_IWUSR, NULL, test1_w);

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
	dev_set_drvdata(&client->dev, dev);

	if (device_create_file(&client->dev, &dev_attr_test1) != 0) {
		dev_err(&client->dev, "failure creating sysfs\n");
		return -1;
	}

	return 0;
}

static int sample_i2c_remove(struct i2c_client *client)
{
	device_remove_file(&client->dev, &dev_attr_test1);
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

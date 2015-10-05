/*
 * Copyright (C) 2015 Lukasz Gemborowski, lukasz.gemborowski@gmail.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that is will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABLILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Genernal Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/list.h>

struct i2c_operation {
    struct list_head list;

    u16 addr;
    u16 len;
    u8 data[0];
};

LIST_HEAD(i2c_operaions_list);

static ssize_t datastream_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 size;
    struct i2c_operation *op = list_first_entry(&i2c_operaions_list, struct i2c_operation, list);

    if (list_empty(&i2c_operaions_list)) {
        return 0;
    }

    /* copy data from i2c_operation struct */
    size = op->len + 4;
    memcpy(buf, &op->addr, size);

    /* remove from list */
    list_del(&op->list);
    kfree(op);

    return size;
}

static DEVICE_ATTR(datastream, 0444, datastream_show, NULL);

static int
i2c_mock_xfer_msg(struct i2c_adapter *adap, struct i2c_msg *msg, int stop)
{
    /* allocate and copy data transferred */
    const u32 struct_size = sizeof(struct i2c_operation) + msg->len;
    struct i2c_operation *new_operation = kmalloc(struct_size, GFP_KERNEL);
    new_operation->addr = msg->addr;
    new_operation->len = msg->len;
    memcpy(new_operation->data, msg->buf, msg->len);

    /* add to list */
    INIT_LIST_HEAD(&new_operation->list);
    list_add_tail(&(new_operation->list), &i2c_operaions_list);

    dev_info(&adap->dev, "msg to 0x%x\n", msg->addr);

	return 0;
}

static int
i2c_mock_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	int i;
	int ret;

	dev_info(&adap->dev, "received msgs: %d\n", num);

	for (i = 0; i < num; i++) {
		ret = i2c_mock_xfer_msg(adap, &msgs[i], (i == (num - 1)));
		dev_info(&adap->dev, "[%d/%d] ret: %d\n", i + 1, num,
			ret);
		if (ret < 0)
			return ret;
	}

	return num;
}

static u32 i2c_mock_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm i2c_mock_algo = {
	.master_xfer	= i2c_mock_xfer,
	.functionality	= i2c_mock_func,
};

static struct i2c_adapter i2c_mock_adapter = {
    .owner      = THIS_MODULE,
    .class      = I2C_CLASS_HWMON | I2C_CLASS_SPD,
    .algo       = &i2c_mock_algo,
    .name       = "mocked i2c adapter",
};

static int __init mock_i2c_init_driver(void)
{
    int ret = i2c_add_adapter(&i2c_mock_adapter);

    if (ret) {
        pr_err("i2c_mock: i2c_add_adapter failed\n");
        return ret;
    }

    ret = device_create_file(&i2c_mock_adapter.dev, &dev_attr_datastream);

    if (ret) {
        dev_err(&i2c_mock_adapter.dev, "failure creating sysfs\n");
        goto error_1;
    }

    return 0;

error_1:
    i2c_del_adapter(&i2c_mock_adapter);
    return ret;
}

static void __exit mock_i2c_exit_driver(void)
{
    device_remove_file(&i2c_mock_adapter.dev, &dev_attr_datastream);
    i2c_del_adapter(&i2c_mock_adapter);
}

MODULE_AUTHOR("Lukasz Gemborowski <lukasz.gemborowski@gmail.com>");
MODULE_DESCRIPTION("Mock I2C bus adapter");
MODULE_LICENSE("GPL");

module_init(mock_i2c_init_driver);
module_exit(mock_i2c_exit_driver);

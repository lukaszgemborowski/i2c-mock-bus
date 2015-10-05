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
#include <linux/completion.h>

struct i2c_operation {
    struct list_head list;

    u16 addr;
    u16 len;
    u8 data[0];
};

struct i2c_response {
    struct mutex lock;
    u16 len;
    u8 *data;
};

/* lovely globals! */
static struct i2c_response response = {__MUTEX_INITIALIZER(response.lock), 0, 0}; /* response for client driver */
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

static ssize_t
response_get(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    mutex_lock(&response.lock);

    /* free old data */
    if (response.len > 0 && response.data)
        kfree(response.data);

    response.len = count;
    response.data = kmalloc(count, GFP_KERNEL);

    if (!response.data) {
        mutex_unlock(&response.lock);
        return -ENOMEM;
    }

    memcpy(response.data, buf, count);

    mutex_unlock(&response.lock);

    return count;
}

static DEVICE_ATTR(datastream, 0444, datastream_show, NULL);
static DEVICE_ATTR(response, 0222, NULL, response_get);

static void
i2c_mock_add_msg_to_list(struct i2c_msg *msg)
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
}

static int
i2c_mock_copy_response(struct device* dev, struct i2c_msg *msg)
{
    if (msg->len != response.len) {
        dev_warn(dev, "response have different length (%iB) than caller expected (%iB). Aborting.\n", response.len, msg->len);
        return 0;
    }

    /* copy data from response buffer filled by "user" to response-i2c_msg structure. */
    memcpy(msg->buf, response.data, msg->len);

    /* release resources, we don't need them */
    response.len = 0;
    kfree(response.data);
    response.data = 0;

    return msg->len;
}

static int
i2c_mock_xfer_msg(struct i2c_adapter *adap, struct i2c_msg *msg)
{
    if (msg->flags == 0) {
        i2c_mock_add_msg_to_list(msg);
    }
    else if (msg->flags == I2C_M_RD) {
        /* client driver want to read something. Userspace need to provide this data */
        mutex_lock(&response.lock);

        if (response.len > 0 && response.data)
            /* copy the data if provided */
            i2c_mock_copy_response(&adap->dev, msg);
        else
            dev_warn(&adap->dev, "No response defined. Aborting\n");

        mutex_unlock(&response.lock);
    }
    else {
        dev_warn(&adap->dev, "unsupported i2c_msg::flags value: 0x%x\n", msg->flags);
    }

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
		ret = i2c_mock_xfer_msg(adap, &msgs[i]);
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
    int ret = 0;

    if ((ret = i2c_add_adapter(&i2c_mock_adapter))) {
        pr_err("i2c_mock: i2c_add_adapter failed\n");
        return ret;
    }

    if ((ret = device_create_file(&i2c_mock_adapter.dev, &dev_attr_datastream)) != 0) {
        dev_err(&i2c_mock_adapter.dev, "failure creating sysfs\n");
        goto error_1;
    }

    if((ret = device_create_file(&i2c_mock_adapter.dev, &dev_attr_response)) != 0) {
        dev_err(&i2c_mock_adapter.dev, "failure creating sysfs\n");
        goto error_2;
    }

    return 0;

error_1:
    i2c_del_adapter(&i2c_mock_adapter);

error_2:
    device_remove_file(&i2c_mock_adapter.dev, &dev_attr_datastream);
    return ret;
}

static void __exit mock_i2c_exit_driver(void)
{
    device_remove_file(&i2c_mock_adapter.dev, &dev_attr_datastream);
    device_remove_file(&i2c_mock_adapter.dev, &dev_attr_response);
    i2c_del_adapter(&i2c_mock_adapter);
}

MODULE_AUTHOR("Lukasz Gemborowski <lukasz.gemborowski@gmail.com>");
MODULE_DESCRIPTION("Mock I2C bus adapter");
MODULE_LICENSE("GPL");

module_init(mock_i2c_init_driver);
module_exit(mock_i2c_exit_driver);

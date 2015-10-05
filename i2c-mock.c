#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <linux/sysfs.h>

static ssize_t datastream_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "dummy");
}

static DEVICE_ATTR(datastream, 0444, datastream_show, NULL);

static int
i2c_mock_xfer_msg(struct i2c_adapter *adap, struct i2c_msg *msg, int stop)
{
    dev_info(&adap->dev, "sending msg to 0x%x\n", msg->addr);
	return -EIO;
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

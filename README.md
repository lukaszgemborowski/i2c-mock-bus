# i2c-mock-bus
Dummy i2c-adapter. Can be used when testing i2c device drivers.

# sample usage
Build i2c-mock and sample i2c client "dirver":

`make -C /lib/modules/$(uname -r)/build M=$(pwd) modules`

Load modules

`insmod i2c-mock.ko && insmod sample-i2c-client.ko`

Now, you need two terminals, in one you will need to instantiate sample-i2c-client at some i2c address:
```
cd /sys/bus/i2c/devices/i2c-4/
echo sample_i2c_client 0x50 > new_device
```

Two things:

1. i2c-4 - basically you need to find your adapter, probably one with highest number. If you cat "name" you should see something like "mocked i2c adapter"
2. your shell is now hanging because sample-i2c-client tried to transfer i2c message with I2C_M_RD flag. So basically it's waitng for input (one byte to be precise) from remote device. You need to feed mock with some data. That's why you need two shells.

In second shell execute something like:
```
cd /sys/bus/i2c/devices/i2c-4/
echo -n X > response
```

Now the first shell is unblocked. Data was transfered back to sample i2c device driver. You can see it in dmesg:

`sample_i2c_client 4-0050: received byte of value 0x5`

You can also review i2c_msgs sent from sample-i2c-client by cat-ing datastream file. It's in binary form. First two bytes are i2c address. Next two bytes are data length followed by raw buffer:

```
# xxd datastream
0000000: 5000 0400 af66 1122                      P....f."
# xxd datastream
0000000: 5000 0300 abcd ef                        P......
# xxd datastream
0000000: 5000 0100 ab                             P....
```

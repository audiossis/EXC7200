#!/bin/bash

cp exc7200.ko /lib/modules/6.6.74+rpt-rpi-v8/kernel/drivers/input/touchscreen/
depmod
modprobe exc7200

echo "exc7200" >> /etc/modules

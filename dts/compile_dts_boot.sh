#!/bin/bash

dtc -@ -I dts -O dtb -o exc7200.dtbo exc7200.dts

cp exc7200.dtbo /boot/overlays/

echo "exc7200.dtbo" >> /boot/config.txt
echo "dtoverlay=exc7200,interrupt=4,sizex=4096,sizey=4096,invx=0,invy=0,swapxy=0" >> /boot/firmware/config.txt

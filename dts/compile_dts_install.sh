#!/bin/bash

dtc -@ -I dts -O dtb -o exc7200.dtbo exc7200.dts

cp exc7200.dtbo /boot/overlays/

dtoverlay -r exc7200
dtoverlay -a | grep exc7200 
dtoverlay exc7200 interrupt=4 sizex=4095 sizey=4095 invx=0 invy=0 swapxy=0
dtoverlay -l | grep exc7200


#!/bin/bash

### NOTES ###
# dtsoverlay cannot unload overlays that were loaded at boot time and it will fail without error.
# Nor can it list overlays that were loaded at boot. So running dtoverlay -l or dtoverlay -a will produce no result.
#
# Attepmting to compile and load an overlay that was loaded at boot leads to an error when trying to load an overlay.
# This error *can* lead to one thinking that compilation/loading has failed due to an error in the overlay .dts file
# If compiling and loading a new overlay fails, then make sure the old overlay wasn't loaded at boot time
# remove or comment any references to the overlay in /boot/firmware/config.txt and reboot to continue testing.
#
#
# dtc -s /proc/device-tree 2>/dev/null > dts_loaded
# The command above will dump the device tree to a file.
# If the overlay is referenced in the file created, then it is already loaded.

echo "Deleting old .dtbo file:"
rm exc7200.dtbo

echo
echo "Compiling .dts file:"
dtc -@ -I dts -O dtb -o exc7200.dtbo exc7200.dts

echo
echo "Copying to /boot/overlays:"
cp exc7200.dtbo /boot/overlays/

echo
echo "Unloading overlay:"
dtoverlay -r exc7200

echo
echo "List active overlay:"
dtoverlay -a | grep exc7200

echo
echo "Loading overlay:" 
dtoverlay -v exc7200 interrupt=4 sizex=4095 sizey=4095 invx=0 invy=0 swapxy=0

echo
echo "listing overlay:"
dtoverlay -l | grep exc7200


TARGET = exc7200

KDIR ?= /lib/modules/`uname -r`/build
obj-m := exc7200.o

CC = gcc

CFLAGS += -Wall -Wextra -pedantic -Wshadow -std=c11
CFLAGS += -D_GNU_SOURCE
CFLAGS += -I .
CFLAGS += -I /usr
CFLAGS += -I /usr/include
CFLAGS += -I /usr/include/linux
CFLAGS += -I /usr/include/c++/12
CFLAGS += -I /usr/include/c++/12/tr1
CFLAGS += -I /usr/include/aarch64-linux-gnu/bits
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-common-rpi/include
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-common-rpi/include/generated
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-common-rpi/include/linux
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-common-rpi/arch/arm64/include
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-common-rpi/arch/arm64/include/generated
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-rpi-v8/arch/arm64/include
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-rpi-v8/arch/arm64/include/generated
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-rpi-v8/include
CFLAGS += -I /usr/src/linux-headers-6.6.74+rpt-rpi-v8/include/generated
CFLAGS += -I /lib/modules/6.6.74+rpt-rpi-v8/build/include/
CFLAGS += -I /lib/modules/6.6.74+rpt-rpi-v8/build/include/generated
CFLAGS += -I /usr/include/aarch64-linux-gnu/bits/
CFLAGS += -lm
LDFLAGS += -pthread

SOURCE = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SOURCE))

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules

#$(TARGET): $(OBJS)
#	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	@rm -rf *.o
	@rm -rf $(OBJS)
	@rm -rf $(TARGET)
	@rm -f .*.swp .*.cmd *.swp *.cmd *.o *.cmd *.mod *.mod.c .*.o* .*.ko* *~ *.ko Module.markers modules.order Module.symvers 
	@rm -rf .tmp_versions/

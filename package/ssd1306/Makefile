obj-m += ssd1306_mod.o
ssd1306_mod-objs += ssd1306.o
ccflags-y := -DDEBUG -g -std=gnu99 -Wno-declaration-after-statement
.PHONY: all clean

all:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(LINUX_DIR) M=$(PWD) clean
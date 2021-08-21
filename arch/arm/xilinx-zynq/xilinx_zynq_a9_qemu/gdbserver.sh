#!/bin/sh

qemu-system-arm -S -s -no-reboot -serial null -serial mon:stdio -net none -nographic -M xilinx-zynq-a9 -m 256M -kernel $1

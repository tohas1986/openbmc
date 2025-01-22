#!/bin/sh -e

reg=$(devmem 0x1e785030 || echo 1)
reg=$(((reg & 0x02) / 2 + 1))

echo "Flying on flash $reg"

if [ $reg = 2 ]; then
	src=/dev/$(cat /proc/mtd | grep spi2 | awk -F ":" '{print $1}')
	dst=/dev/$(cat /proc/mtd | grep spi1 | awk -F ":" '{print $1}')
	echo "CS lines are reverted, switching back"
	devmem 0x1e785034 32 1
else
	src=/dev/$(cat /proc/mtd | grep spi1 | awk -F ":" '{print $1}')
	dst=/dev/$(cat /proc/mtd | grep spi2 | awk -F ":" '{print $1}')
	echo "CS lines are normal"
fi

# Unlock flash
# Erase flash
# Copy flash
echo "Copying from $src to $dst"
dd if=$src of=/fullflash.bin bs=64k
flashcp -v /fullflash.bin $dst
rm -rf /fullflash.bin > /dev/null 2>&1
echo "Copy done"

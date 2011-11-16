#!/bin/bash -ex

scons
scons test

mkimage -A arm -O linux -T kernel -C none -a 0x80008000 -e 0x80008000 -n "Kiwi Loader" -d build/arm-omap3/source/loader build/arm-omap3/uImage.bin

mkdir -p bimgbuild/boot
cp build/arm-omap3/test/test.elf bimgbuild/

cat > bimgbuild/boot/loader.cfg << EOF
kboot "/test.elf" []
EOF

cd bimgbuild
tar -cvf ../boot.tar *
cd ..
rm -rf bimgbuild
mkimage -A arm -O linux -T ramdisk -C none -a 0x0 -n "Kiwi Boot Image" -d boot.tar build/arm-omap3/boot.bin
rm -f boot.tar

echo 'fatload mmc 0 80200000 uImage.bin' > boot.scr.tmp
echo 'fatload mmc 0 81600000 boot.bin' >> boot.scr.tmp
echo 'bootm 80200000 81600000' >> boot.scr.tmp
mkimage -A arm -O linux -T script -C none -a 0 -e 0 -n "Kiwi" -d boot.scr.tmp build/arm-omap3/boot.scr
rm -f boot.scr.tmp

if [ ! -f build/arm-omap3/beagle-nand.bin ]; then
	zcat test/beagle-nand.bin.gz > build/arm-omap3/beagle-nand.bin
fi
qemu-system-arm -M beagle -mtdblock build/arm-omap3/beagle-nand.bin -sd fat:build/arm-omap3 -serial stdio

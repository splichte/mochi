# booting code gets up to 16K.
cat boot/boot_sect.bin boot/switch_to_pm.bin > os.img

b=$(wc -c os.img | awk '{ print $1 }')

# pad to 4K + 512 bytes (4608) so bootloader starts on 0x2000, 
# after we load starting on sector 2 to 0x1000 in boot_sect.asm
dd if=/dev/zero bs=1 count=$((4608-$b)) >> os.img

cat boot/bootloader.bin >> os.img

b=$(wc -c os.img | awk '{ print $1 }')

# pad to 16K (so bootloader should not use more than about 12K)
dd if=/dev/zero bs=1 count=$((16384-$b)) >> os.img


# OS code starts at byte 16384
# and we guarantee it can have 8Mb.

cat kernel.bin >> os.img
rm kernel.bin
# pad to at least 8M. (4096 * 2048)
# if many blocks are needed, bs=1 is really slow. 
# hence we don't shoot for exactly 1M, and just 
# make sure we have enough. 
dd if=/dev/zero bs=4096 count=2048 >> os.img

cat drive.img >> os.img

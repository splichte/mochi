# booting code gets up to 8K.
cat boot/boot_sect.bin boot/switch_to_pm.bin boot/bootloader.bin > os_image

b=$(wc -c os_image | awk '{ print $1 }')

# pad to 8K
dd if=/dev/zero bs=1 count=$((8192-$b)) >> os_image


# OS code starts at byte 8192
# and we guarantee it can have 8Mb.

cat kernel.bin >> os_image
# pad to at least 8M. (4096 * 2048)
# if many blocks are needed, bs=1 is really slow. 
# hence we don't shoot for exactly 1M, and just 
# make sure we have enough. 
dd if=/dev/zero bs=4096 count=2048 >> os_image


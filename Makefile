C_SOURCES = $(wildcard kernel/*.c drivers/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h)

# Convert the *.c files to *.o
OBJ = ${C_SOURCES:.c=.o}

all: os_image

# NOTE: if e.g. switch_to_pm.asm changes, 'make run' will not refresh
# properly. need to run make clean.
run: all
	# https://wiki.qemu.org/Documentation/Networking#Network_Monitoring
	# todo: https://gist.github.com/extremecoders-re/e8fd8a67a515fee0c873dcafc81d811c
	# useful: memory_region*
	#	-d in_asm,op,trace:memory_region*,trace:guest_mem* 
	~/code/qemu/build/i386-softmmu/qemu-system-i386 \
		-drive format=raw,file=os_image,index=0 \
	-netdev user,id=mynet0,hostfwd=tcp::8080-:80 \
	-device e1000,netdev=mynet0 \
	-object filter-dump,id=f1,netdev=mynet0,file=netdump.dat

# dd call makes sure we have zero padding on the end so 
# we can always read the number of 512-byte blocks we need 
# in boot_sect.asm.
# without padding, we have roughly 16K in our final binary size.
os_image: boot/boot_sect.bin kernel.bin
	cat $^ > os_image
	dd if=/dev/zero bs=4096 count=48 >> os_image 2>/dev/null

# "-Ttext 0x1000" must match KERNEL_ENTRY in boot/boot_sect.asm
kernel.bin: kernel/kernel_entry.o ${OBJ}
	i386-elf-ld -Ttext 0x1000 --oformat binary $^ -o $@

# -mgeneral-regs-only lets you use __attribute__((interrupt))
%.o : %.c ${HEADERS}
	i386-elf-gcc -mgeneral-regs-only -ffreestanding -c $< -o $@

# boot_sect.bin
%.bin : %.asm
	nasm $< -f bin -o $@

# kernel_entry.o
%.o: %.asm
	nasm $< -f elf -o $@

clean: 
	rm -rf *.bin *.o os_image
	rm -rf kernel/*.o boot/*.bin drivers/*.o

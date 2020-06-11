C_SOURCES = $(wildcard kernel/*.c drivers/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h)

# Convert the *.c files to *.o
OBJ = ${C_SOURCES:.c=.o}

all: os_image

# TODO: if e.g. switch_to_pm.asm changes, 'make run' will not refresh
# properly. need to run make clean.
run: all
	# https://wiki.qemu.org/Documentation/Networking#Network_Monitoring
	# todo: https://gist.github.com/extremecoders-re/e8fd8a67a515fee0c873dcafc81d811c
	# useful: memory_region*
	#	-d in_asm,op,trace:memory_region*,trace:guest_mem* 
#	~/code/qemu/build/i386-softmmu/qemu-system-i386 -s 
	qemu-system-i386 \
		-drive format=raw,file=os_image,index=0 \
	-netdev user,id=mynet0,hostfwd=tcp::8080-:80 \
	-device e1000,netdev=mynet0 \
	-object filter-dump,id=f1,netdev=mynet0,file=netdump.dat

os_image: boot/boot_sect.bin boot/switch_to_pm.bin boot/bootloader.bin kernel.bin
	./prepare_image.sh

boot/boot_sect.bin: boot/boot_sect.asm
	nasm $< -f bin -o $@

boot/switch_to_pm.bin:	boot/switch_to_pm.asm
	nasm $< -f bin -o $@

# very important to not link in too many objects. 
# how does this run? 0x1000 should be offset...not raw addr
boot/bootloader.bin: boot/bootloader_entry.o boot/bootloader.o kernel/hardware.o drivers/screen.o drivers/disk.o
	i386-elf-ld -Ttext 0x1400 --oformat binary $^ -o $@

# "-Ttext 0x1000000" must match KERNEL_ENTRY in boot/bootloader.c
kernel.bin: kernel/kernel_entry.o kernel/timer_irq.o ${OBJ}
	i386-elf-ld -Ttext 0x1000000 --oformat binary $^ -o $@

# -mgeneral-regs-only lets you use __attribute__((interrupt))
%.o : %.c ${HEADERS}
	i386-elf-gcc -mgeneral-regs-only -ffreestanding -c $< -o $@

# boot_sect.bin
# kernel_entry.o, timer_handler.o
%.o: %.asm
	nasm $< -f elf -o $@

clean: 
	rm -rf *.bin *.o os_image
	rm -rf kernel/*.o boot/*.o boot/*.bin drivers/*.o

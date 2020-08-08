C_SOURCES = $(wildcard kernel/*.c net/*.c drivers/*.c)
HEADERS = $(wildcard include/*.h)

# Convert the *.c files to *.o
OBJ = ${C_SOURCES:.c=.o}
DEBUG_OBJ = ${C_SOURCES:.c=.debug.o}

all: os.img

debug: kernel.debug
	/usr/local/Cellar/binutils/2.34/bin/objdump -S -d kernel.debug > dis.txt


# TODO: if e.g. switch_to_pm.asm changes, 'make run' will not refresh
# properly. need to run make clean.
run: all
	# https://wiki.qemu.org/Documentation/Networking#Network_Monitoring
	# todo: https://gist.github.com/extremecoders-re/e8fd8a67a515fee0c873dcafc81d811c
	# useful: memory_region*
	#	-d in_asm,op,trace:memory_region*,trace:guest_mem* 
	#	# useful:
#	# -d in_asm,mmu,cpu_reset
#	~/code/qemu/i386-softmmu/qemu-system-i386 \
	#	-netdev user,id=mynet0,hostfwd=tcp::8080-:80 
#
	~/code/qemu/i386-softmmu/qemu-system-i386 \
		-drive format=raw,file=os.img,index=0 \
	-netdev user,id=u1 \
	-device e1000,netdev=u1 \
	-object filter-dump,id=f1,netdev=u1,file=netdump.dat

os.img: boot/boot_sect.bin boot/switch_to_pm.bin boot/bootloader.bin kernel.bin
	./script/prepare_image.sh

boot/boot_sect.bin: boot/boot_sect.asm
	nasm $< -f bin -o $@

boot/switch_to_pm.bin:	boot/switch_to_pm.asm
	nasm $< -f bin -o $@

# very important to not link in too many objects. 
boot/bootloader.bin: boot/bootloader_entry.o boot/bootloader.o kernel/hardware.o drivers/screen.o drivers/disk.o
	i386-elf-ld -Ttext 0x2000 --oformat binary $^ -o $@

# "-Ttext 0xc1000000" must match KERNEL_ENTRY in boot/bootloader.c
# (with an offset of 0xc0000000, to make a higher half kernel.)
# 
kernel.bin: kernel/kernel_entry.o kernel/timer_irq.o kernel/fork.o ${OBJ}
	i386-elf-ld -Ttext 0xc1000000 --oformat binary $^ -o $@

kernel.debug: kernel/kernel_entry.debug.o kernel/timer_irq.debug.o kernel/fork.debug.o ${DEBUG_OBJ}
	i386-elf-ld -Ttext 0xc1000000 $^ -o $@

# -mgeneral-regs-only lets you use __attribute__((interrupt))
%.o : %.c ${HEADERS}
	i386-elf-gcc -I./include -mgeneral-regs-only -ffreestanding -c $< -o $@

%.debug.o : %.c ${HEADERS}
	i386-elf-gcc -I./include -g -mgeneral-regs-only -ffreestanding -c $< -o $@

# boot_sect.bin
# kernel_entry.o, timer_handler.o
%.o: %.asm
	nasm $< -f elf -o $@

%.debug.o: %.asm
	nasm $< -f elf -o $@

clean: 
	rm -rf *.bin *.o os.img kernel.debug
	rm -rf kernel/*.o boot/*.o boot/*.bin drivers/*.o

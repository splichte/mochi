C_SOURCES = $(wildcard kernel/*.c drivers/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h)

# Convert the *.c files to *.o
OBJ = ${C_SOURCES:.c=.o}

all: os_image

run: all
	qemu-system-i386 -drive format=raw,file=os_image,index=0

# dd call makes sure we have zero padding on the end so 
# we can always read the number of 512-byte blocks we need 
# in boot_sect.asm.
# without padding, we have roughly 16K in our final binary size.
os_image: boot/boot_sect.bin kernel.bin
	cat $^ > os_image
	dd if=/dev/zero bs=4096 count=12 >> os_image 2>/dev/null

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

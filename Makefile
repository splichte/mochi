C_SOURCES = $(wildcard kernel/*.c drivers/*.c)
HEADERS = $(wildcard kernel/*.h drivers/*.h)

# Converet the *.c files to *.o
OBJ = ${C_SOURCES:.c=.o}

all: os_image

run: all
	qemu-system-i386 -drive format=raw,file=os_image,index=0

os_image: boot/boot_sect.bin kernel.bin
	cat $^ > os_image

kernel.bin: kernel/kernel_entry.o ${OBJ}
	i386-elf-ld -Ttext 0x1000 --oformat binary $^ -o $@

%.o : %.c ${HEADERS}
	i386-elf-gcc -ffreestanding -c $< -o $@

# boot_sect.bin
%.bin : %.asm
	nasm $< -f bin -o $@

# kernel_entry.o
%.o: %.asm
	nasm $< -f elf -o $@

clean: 
	rm -rf *.bin *.o os_image
	rm -rf kernel/*.o boot/*.bin drivers/*.o

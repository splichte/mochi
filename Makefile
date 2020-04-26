all: os_image

run: all
	qemu-system-i386 -drive format=raw,file=os_image,index=0

os_image: boot_sect.bin kernel.bin
	cat $^ > os_image

boot_sect.bin: boot_sect.asm
	nasm $< -f bin -o $@

kernel.bin: kernel_entry.o kernel.o
	i386-elf-ld -Ttext 0x1000 --oformat binary $^ -o $@

kernel.o: kernel.c
	i386-elf-gcc -ffreestanding -c $< -o $@

kernel_entry.o: kernel_entry.asm
	nasm $< -f elf -o $@

clean: 
	rm *.bin *.o os_image

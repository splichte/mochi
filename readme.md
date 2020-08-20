# Mochi: a from-scratch i386 operating system.

Mochi is a personal learning project. My goal was to understand how the operating system and the hardware (in this case, an Intel 80386 processor and peripherals) talk to each other. But as I researched, I hit some pain points and a general lack of references to primary sources. So now another goal is to document it enough that it might be useful to someone else. It only runs on qemu-system-i386 right now. I cross-compile it on my Mac with 32-bit GCC compilers and linkers (see the Makefile).

I've spent a few months of part-time work on it, but it is *extremely* in-progress. Most of the more hardware-interfacing bits are already pretty figured out (setting up interrupts and the page tables, writing some basic kernel memory allocators and string functions, writing the bootloaders, doing most of the direct device-level work, etc.)

Currently, I'm deep into the implementation of an ext2 filesystem and the network stack. Both have a lot of work done already, but they're not really usable yet. I also want to add a ton of tests, especially for the filesystem, but that will take time too. 

I'm going to try to update this README with as much useful information I've found for myself and others. 

## How To Build/Run

First, initialize an empty hard disk image (24Mb) with `./scripts/reset_disk.sh`. Then `make run` in the root directory. This builds the bootloader and kernel image, prepends it to the hard disk image, and runs QEmu with it. 

You need `qemu-system-i386`, `nasm`, `i386-elf-gcc`, and `i386-elf-ld`. To make the debug version, you need `objdump`. 
	
## Debugging Strategy

Using `qemu-system-i386` is a huge relief. It can be difficult to figure out how to get useful debugging information from such a large codebase, but once you do, it makes diagnosing low-level errors quick and reliable. In general, I use the following approach.

### 1. Building a local version of QEmu
The core processor emulation code of QEmu is extremely complicated. However, the device and network code is not. For instance, the E1000 emulation is located at `qemu/hw/net/e1000.c`, and the entire network stack is at `qemu/slirp/src` (NOTE: if you modify slirp, you need to manually delete the old qemu binary in between makes, since an intermediate static library is produced and Make doesn't seem to update the qemu binary properly).

When debugging the network code especially, it's very easy to just include stdio and put print statements everywhere to see what code paths are being hit, what is not happening that you expect to happen, etc.

Make sure to set the target list like `./configure --target-list=i386-softmmu` before making, or QEmu will try to make every single target architecture, which will take a long time.

### 2. Using QEmu debug flags `-d in_asm,cpu_reset` along with a GCC disassembly

When your machine crashes (it triggers a triple fault), QEmu saves the value of the instruction pointer just before the crash. By pairing this with a disassembly of your code that looks like this:

```
int disk_read_blk(uint32_t block_num, uint8_t *buf) {
c100004f:   55                      push   %ebp
c1000050:   89 e5                   mov    %esp,%ebp
c1000052:   83 ec 18                sub    $0x18,%esp
```

You can immediately figure out which instruction caused the crash. In mochi, `make debug` will produce this output for you, in a file `dis.txt`. 

In maybe a proper OS setup, you shouldn't *need* to do this -- the triple fault handler only triggers because there was some kind of error in the other exception handlers. But you might not have configured those yet, or there might be some kind of stack issue in your handlers. This strategy will work no matter how messed-up your code is.

Even if you're at an earlier stage of your OS development and are still in NASM-land, using the `-d in_asm` flag on QEmu is really helpful. Just by tracing the assembly instructions and jumps, you can quickly find out where your code made a bad jump. 

## Useful Resources

These were resources that helped me a ton, in some way or another, in roughly the chronological order that I encountered them.

## Interacting with Hardware

The most precise part comes at the start. You need to constantly worry about where things are located in physical memory and jump to raw addresses. Having a good debugging strategy proved critical for me (see above).

### [Nick Blundell - "Writing a Simple Operating System from Scratch"](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)

This was what started the whole journey, because I worked through this PDF on a whim and had fun, so I decided to keep going with this whole OS thing.

Specifically, I read [this PDF](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf) and did all the exercises in that. I used qemu-system-i386, not Bochs, and I used NASM and a GCC 32-bit cross-compiler running on my 64-bit Mac to produce ELF binaries. I had to fill in a couple of holes in the PDF here and there--I think the OS image you produce needs to be padded with zeros so the BIOS can load all the sectors of your code you need, and some of the screen-driver code in the last chapter is super garbled--but mostly everything went smoothly. 

That PDF is unfinished, but it's gradual and nicely written, and it
gets you to the point where you can boot into 32-bit protected mode, clear the screen, and print "Hello, world!". 
That's pretty great! Furthermore, the `in` and `out` x86 port instructions
you learned are how you're going to be talking to all the upcoming
devices you'll encounter, so you have a good foundation.


### [OSDev](https://wiki.osdev.org/PCI)

I really dislike the editorial style of OSDev, so I can't fully endorse it as a reference to anyone. Further, the quality of the wiki pages has high variance. But there are some wiki pages I found useful -- the PCI page especially, and I think I also got use out of the ATA pages (talking to the hard drive) and the Paging page (setting up virtual memory). 

In Mochi, I use PCI to initialize the e1000 -- the ethernet driver.  


### [ToaruOS](https://github.com/klange/toaruos)

ToaruOS basically already did what I wanted to do. Because of that, I tried to look at it as little as possible. I wanted to come up with my own solutions. But occasionally, when I was hitting a bug with some hardware initialization or endianness problem or other, it was really helpful to be able to cross-reference with an existing implementation.

 

### [IBM Personal System/2 Hardware Interface Technical Reference (May 1988)](http://classiccomputers.info/down/IBM_PS2/documents/PS2_Hardware_Interface_Technical_Reference_May88.pdf)
- Especially sections "Interrupt Controller", "Keyboard and Auxiliary Device Controller", "Video Subsystem", "Keyboards".
- Answers questions: where do those magic device numbers on OSDev or in Blundell's guide come from? How do I set up (PIC) hardware interrupts and talk to the mouse/keyboard/screen?
- The PS/2 is the nearest computer model that qemu-system-i386 emulates, and kind of a canonical "i386" machine. So it feels good to have a sense of what it looked like originally. 


Early on, after I'd gotten the booting to work and was starting to set up interrupts and talk to devices with the help of blog posts and OSDev, I began to wonder where all these magic numbers were coming from. For instance, the keyboard status byte is read from port 0x64 and the key byte is read from port 0x60. The video address for the screen is at 0xb8000. 
 
I was getting a little discouraged. Who set those numbers? Are they always valid, and if not, when are they not? There are tons of things like this, and it was harder to find answers than I expected. You very quickly wind up hitting web pages written in the late 90's/early 2000's -- or searching through documentation that is thousands of pages long and mainly written for electrical engineers.

This issue comes up a lot, when you first start out. For instance, setting up interrupts involves sending the correct 4 command bytes to the PIC
(programmable interrupt controller), which is an '80s Intel item that has documentation under the number 8259A. The PIC manual will tell you what those bytes are supposed to look like, but not how a programmer is supposed to actually send them (i.e. what port in the 80386's address space the interrupt controller's inputs are mapped to). The 8529A manual explains how, but because the 8259A and 80386 (the processor) are connected to each other externally and don't assume the other's existence, the 8259A manual only explains the format of the bytes and doesn't explain where a programmer of the 80386 is actually supposed to send them. 

The PS/2 Hardware Manual  is the only primary source I've found that actually says where PIC1 and PIC2 are mapped to in the CPU's address space (for at least a particular i386 computer type). Many useful resources like [OSDev](osdev.org) or [ToaruOS](https://github.com/klange/toaruos) do have these locations--which are`0x20` and `0xA0`, by the way -- but they don't say where they're getting them from.

So I spent some time digging, and finally found the PS/2 documentation, which helped me feel "grounded" and answered the particular questions I had, in a way that satisfied me. I also learned a lot about computing history that I didn't know. 

This is the real computer (with an i386 chip) that qemu-system-i386 sort-of emulates. As such, a lot of "magic" constants and how to talk to peripherals -- information rarely in any Intel manuals, since it isn't tied to the processor itself but rather to the motherboard and connection setup -- can be found here. 

### [cstack: Higher Half Kernel](https://medium.com/@connorstack/how-does-a-higher-half-kernel-work-107194e46a64)

Explains the how and why for the "higher half kernel", a concept I'd never heard of before trying to make an OS (like everything else). Once you start to set up the page tables for virtual memory, you want to consider where everything is going to actually live in the virtual address space. I don't actually have user mode yet, so I don't *quite* need this -- but I plan to! 

### [MIT Lab 6: Network Driver](https://pdos.csail.mit.edu/6.828/2018/labs/lab6/)

Probably my favorite resource for a self-learner on this list! This steps you through a working E1000 implementation for qemu-system-i386  and encourages you to read the Intel manual for the E1000, which it hosts itself. Because of this page, implementing the E1000 was one of the most fun parts of making the OS so far. I love it.

## Higher-Level Constructs: Network, Filesystem, Etc.

Once you reach this point, you're solidly in C-land, you can `malloc` and `memmove` stuff, and things move along really quickly.

### [The Second Extended File System: Internal Layout](https://www.nongnu.org/ext2-doc/ext2.html)

Basically what I followed in my implementation.

### Wikipedia and Wireshark (Network Protocols)

Wikipedia's pages for the network protocol headers -- Ethernet, IP, UDP, are seriously fantastic. Combine that with Wireshark to check that your packets are getting parsed correctly, and you barely need a book or the RFCs at all. 

### W. Richard Stevens - TCP/IP Illustrated, [Volume 1: The Protocols](https://www.amazon.com/TCP-Illustrated-Vol-Addison-Wesley-Professional/dp/0201633469/ref=sr_1_2?dchild=1&keywords=w+richard+stevens+tcp+ip&qid=1597941357&sr=8-2) and [Volume 2: The Implementation](https://www.amazon.com/TCP-IP-Illustrated-Implementation-Vol/dp/020163354X/ref=sr_1_3?dchild=1&keywords=w+richard+stevens+tcp+ip&qid=1597941330&sr=8-3)

Great books. I used the originals from the '90s, which are still available used on Amazon. Volume 2 seems forboding at first, but the sections on memory buffers and protocol buffers were really illuminating to me. It's really interesting to see how much higher-level language features like object methods 

### 

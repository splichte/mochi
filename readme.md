# Mochi: a baby OS.

*Note: Documentation in progress. Code in progress. Everything in progress!!*

Status: Keyboard interrupts working; all keys mapped correctly! 


## How does this work?

*Note: this is mostly for the author, but hopefully it will help someone else too.*

### Motive

I studied Computer Science in college, but I spent all my time trying to do trendy machine learning stuff. I had good intentions, but I wound up not learning a lot of fundamentals -- like operating systems.

### Nick Blundell's "Writing a Simple Operating System from Scratch"
My first step was working through [Nick Blundell's PDF](https://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf) and doing all the exercises in that. I used qemu-system-i386, not Bochs, and I used NASM and a GCC 32-bit cross-compiler running on my 64-bit Mac to produce ELF binaries. I had to fill in a couple of holes in the PDF here and there--I think the OS image you produce needs to be padded with zeros so the BIOS can load all the sectors of your code you need, and some of the screen-driver code in the last chapter is super garbled--but mostly everything went smoothly. 

That PDF is unfinished, but it's gradual and nicely written, and it
gets you to the point where you can boot into 32-bit protected mode, clear the screen, and print "Hello, world!". 
That's pretty great! Furthermore, the `in` and `out` x86 port instructions
you learned are how you're going to be talking to all the upcoming
devices you'll encounter, so you have a good foundation.

###Keyboard Input

Unfortunately, to get the keyboard working, you have to set up hardware
interrupts<sup>(1)</sup>. What I found is that once you know how the various pieces of '80s Intel hardware and processor data structures fit together, doing this kind of makes sense, but until then it can be quite frustrating.

Settting up interrupts involves sending the correct 4 command bytes to the PIC
(programmable interrupt controller), which is an '80s Intel item that has documentation under the number 8259A. The PIC manual will tell you what those bytes are supposed to look like, but not how a programmer is supposed to actually send them (i.e. what port in the 80386's address space the interrupt controller's inputs are mapped to). The 8529A manual explains how, but because the 8259A and 80386 (the processor) are connected to each other externally and don't assume the other's existence, the 8259A manual only explains the format of the bytes and doesn't explain where a programmer of the 80386 is actually supposed to send them. 

The PS/2 Hardware Manual (IBM, 1988) is the only primary source I've found that actually says where PIC1 and PIC2 are mapped to in the CPU's address space (for at least a particular i386 computer type). Many useful resources like [OSDev](osdev.org) or [ToaruOS](https://github.com/klange/toaruos) do have these locations--which are
`0x20` and `0xA0`, by the way -- but they don't say where they're getting them from, which I found really unsatisfying. Maybe people just know them. The manual for the PS/2, the computer that I think qemu-system-i386 is mostly emulating, *does* say this.
However, their command words differ ever-so-slightly from the command
words that ToaruOS sends. In general, I trust ToaruOS to better match my use case, since it was written to work on qemu-system-i386.

So, yeah. It all works, for now. I still don't really feel satisfied.

Some interesting system programming things I learned! The GCC compiler has a nice feature called `__attribute__((interrupt))` you can slap on functions, which handles the stack usage properly, so you can actually write the interrupt handlers in C and not assembly. I also had to use `__attribute__((packed))` on because Intel data structures are often 6 bytes and the C compiler really wants to align everything in 4-byte increments. There's an example of this in the `doc/examples` folder. It feels really cool to learn this stuff. I didn't know `__attribute__` existed at all before this. 



<sup>(1)</sup> You don't actually *have* to -- you can "poll" the devices by
  repeatedly checking them in an infinite loop -- but if the
  devices don't have any input ready, you're causing the CPU
  to keep looping, executing instructions without doing any
  useful work. And we need to learn how to manage interrupts
  anyway if we want the OS to be able to schedule processes
  effectively, since that involves attaching an OS function
  to a hardware timer that periodically interrupts the CPU.
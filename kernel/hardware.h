#define BOOTLOADER_OFFSET   0x2000
#define KERNEL_OFFSET       0xc0000000
/* note: we must use e.g. "unsigned short" instead of "uint16_t" 
 * so that the underlying "asm" command is happy with its operand constraints. */

unsigned char port_byte_in(unsigned short port);
void port_byte_out(unsigned short port, unsigned char data);
unsigned short port_word_in(unsigned short port);
void port_word_out(unsigned short port, unsigned short data);
void port_multiword_out(unsigned short port, unsigned char *data, unsigned long size);
void port_multiword_in(unsigned short port, unsigned char *data, unsigned long size);

void port_dword_out(unsigned short port, unsigned int data);
unsigned int port_dword_in(unsigned short port);


void sys_exit();

/* debugging */
#define HALT()  while (1) { }
#define PRINT_EIP()  do { \
    uint32_t eip; \
    print("%EIP: ");\
    asm volatile ("call _test\n\t" \
            "_test: pop %%eax\n\t"\
            "mov %%eax, %0;": "=a" (eip)); \
    print_word(eip); \
    HALT();\
    } while (0);

#define DUMP_STACK() do { \
    uint32_t test;\
    asm volatile ("mov %%esp, %0" : "=r" (test) :);\
    print("esp: "); print_word(test);\
    asm volatile ("mov %%ebp, %0" : "=r" (test) :);\
    print("ebp: "); print_word(test);\
    } while (0);

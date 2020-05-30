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



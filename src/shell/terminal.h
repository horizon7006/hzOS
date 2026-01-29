#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef void (*term_sink_t)(char c);

void terminal_init(void);
void terminal_clear(void);
void terminal_setcolor(uint8_t color);
void terminal_putc(char c);
void terminal_write(const char* str);
void terminal_writeln(const char* str);

void terminal_set_sink(term_sink_t sink);

#ifdef __cplusplus
}
#endif
#include "rtc.h"
#include "../core/io.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static int rtc_updating() {
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

static uint8_t read_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

void rtc_read(rtc_time_t* t) {
    while (rtc_updating());

    t->second = read_register(0x00);
    t->minute = read_register(0x02);
    t->hour   = read_register(0x04);
    t->day    = read_register(0x07);
    t->month  = read_register(0x08);
    t->year   = read_register(0x09);

    uint8_t regB = read_register(0x0B);

    if (!(regB & 0x04)) {
        t->second = (t->second & 0x0F) + ((t->second / 16) * 10);
        t->minute = (t->minute & 0x0F) + ((t->minute / 16) * 10);
        t->hour   = ( (t->hour & 0x0F) + (((t->hour & 0x70) / 16) * 10) ) | (t->hour & 0x80);
        t->day    = (t->day & 0x0F) + ((t->day / 16) * 10);
        t->month  = (t->month & 0x0F) + ((t->month / 16) * 10);
        t->year   = (t->year & 0x0F) + ((t->year / 16) * 10);
    }
    
    t->year += 2000; 
}

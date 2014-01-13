/*
 * Simulator of MK-54 programmable soviet calculator.
 * Based on sources of emu145 project: https://code.google.com/p/emu145/
 *
 * Copyright (C) 2013 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include "calc.h"
#include <stdio.h>

//
// MK-54 calculator consists of two PLM chips ИК1301 and ИК1303,
// and two serial FIFOs К145ИР2.
//
static plm_t ik1302, ik1303;
static fifo_t fifo1, fifo2;

//
// Initialize the calculator.
//
void calc_init()
{
    #include "ik1302.c"
    #include "ik1303.c"

    plm_init (&ik1302, ik1302_ucmd_rom, ik1302_cmd_rom, ik1302_prog_rom);
    plm_init (&ik1303, ik1303_ucmd_rom, ik1303_cmd_rom, ik1303_prog_rom);
    fifo_init (&fifo1);
    fifo_init (&fifo2);
}

//
// Extract stack and register values from the serial shift registers.
//
static value_t fetch_value (unsigned chip, unsigned address)
{
    unsigned char *data = 0;
    value_t result = { {0} };
    int i;

    switch (chip) {
    case 1: data = fifo1.data + address; break;
    case 2: data = fifo2.data + address; break;
    case 3: data = ik1302.M + address;   break;
    case 4: data = ik1303.M + address;   break;
    //case 5: data = ik1306.M + address;   break;
    }
    if (data) {
        for (i=0; i<6; i++, data-=6)
            result.byte[i] = data[0] | data[-3] << 4;
    }
    return result;
}

//
// Extract stack and register values from the serial shift registers.
//
static void fetch_data (int phase)
{
    typedef struct {
        unsigned char chip;
        unsigned char address;
    } location_t;

    static const location_t memory_map[15] = {
        {1, 41}, {1, 83}, {1, 125}, {1, 167}, {1, 209}, {1, 251},
        {2, 41}, {2, 83}, {2, 125}, {2, 167}, {2, 209}, {2, 251},
        {3, 41}, {4, 41}, {5, 41},
    };
    static const location_t stack_map[15] = {
        {1, 34}, {1, 76}, {1, 118}, {1, 160}, {1, 202}, {1, 244},
        {2, 34}, {2, 76}, {2, 118}, {2, 160}, {2, 202}, {2, 244},
        {3, 34}, {4, 34}, {5, 34},
    };
#if 1
    // For MK-54.
    static const unsigned char remap_memory[3][14] = {
        { 1, 2, 3, 4, 5, 13, 12, 6,  7,  8,  9,  10, 11, 0 },
        { 3, 4, 5, 0, 1, 13, 12, 8,  9,  10, 11, 6,  7,  2 },
        { 5, 0, 1, 2, 3, 13, 12, 10, 11, 6,  7,  8,  9,  4 },
    };
    static const unsigned char remap_stack[3][5] = {
        { 8,  9,  10, 11, 0 },
        { 10, 11, 6,  7,  2 },
        { 6,  7,  8,  9,  4 },
    };
#else
    // For MK-61
    static const unsigned char remap_memory[3][15] = {
        { 1,  2,  3,  4,  5,  14, 13, 12, 6, 7, 8,  9,  10, 11, 0 },
        { 10, 11, 6,  7,  2,  3,  4,  5,  0, 1, 14, 13, 12, 8,  9 },
        { 14, 13, 12, 10, 11, 6,  7,  8,  9, 4, 5,  0,  1,  2,  3 },
    };
    static const unsigned char remap_stack[3][5] = {
        { 8,  9,  10, 11, 0 },
        { 14, 13, 12, 8,  9 },
        { 5,  0,  1,  2,  3 },
    };
#endif
    location_t loc;
    int i;

    for (i=0; i<14; i++) {            // 15 for MK-61
        loc = memory_map[remap_memory[phase][i]];
        calc_reg[i] = fetch_value (loc.chip, loc.address - 8);
    }

    for (i=0; i<5; i++) {
        loc = stack_map[remap_stack[phase][i]];
        calc_stack[i] = fetch_value (loc.chip, loc.address);
    }
}

//
// Simulate one cycle of the calculator.
// Return 0 when stopped, or 1 when running a user program.
// Fill digit[] and dit[] arrays with the indicator contents.
//
int calc_step()
{
    int k, i, digit, dot;
    unsigned cycle;

    for (k=0; k<560; k++) {
        // Scan keypad.
        i = calc_keypad();
        ik1302.keyb_x = i >> 4;
        ik1302.keyb_y = i & 0xf;
        ik1303.keyb_x = calc_rgd();
        ik1303.keyb_y = 1;

        // Do computations.
        for (cycle=0; cycle<REG_NWORDS; cycle++) {
            calc_poll();
            ik1302.input = fifo2.output;
            plm_step (&ik1302, cycle);
            ik1303.input = ik1302.output;
            plm_step (&ik1303, cycle);
            fifo1.input = ik1303.output;
            fifo_step (&fifo1);
            fifo2.input = fifo1.output;
            fifo_step (&fifo2);
            ik1302.M[cycle] = fifo2.output;
        }
#if 0
        if (ik1302.dot == 11 && k%14 == 0) {
            printf ("             %-2u :", k/14);
            for (i=0; i<12; i++) {
                if (11-i < 3) {
                    // Exponent.
                    digit = ik1302.R [(11-i + 9) * 3];
                    dot = ik1302.show_dot [11-i + 10];
                } else {
                    // Mantissa.
                    digit = ik1302.R [(11-i - 3) * 3];
                    dot = ik1302.show_dot [11-i - 2];
                }
                putchar ("0123456789-LCRE " [digit]);
                if (dot)
                    putchar ('.');
            }
            printf ("' (%x %x) %08x\n", ik1302.R[39], ik1302.R[36], ik1302.command);
        }
#endif

        i = k % 14;
        if (i >= 12) {
            // Clear display.
            calc_display (-1, 0, 0);
        } else {
            if (i < 3) {
                // Exponent.
                digit = ik1302.R [(i + 9) * 3];
                dot = ik1302.show_dot [i + 10];
            } else {
                // Mantissa.
                digit = ik1302.R [(i - 3) * 3];
                dot = ik1302.show_dot [i - 2];
            }

            if (ik1302.dot == 11) {
                // Run mode: blink once per step with dots enabled.
                if (ik1302.command != 0x00117360)
                    digit = -1;
                calc_display (i, digit, 1);

            } else if (ik1302.enable_display) {
                // Manual mode.
                calc_display (i, digit, dot);
                ik1302.enable_display = 0;
            } else {
                // Clear display.
                calc_display (i, -1, -1);
            }
        }
    }
    fetch_data (fifo1.cycle / (2*REG_NWORDS));

    return (ik1302.dot == 11);
}

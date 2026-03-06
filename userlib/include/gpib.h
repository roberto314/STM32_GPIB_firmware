/*
 * gpib.h
 *
 *  Created on: Mar 5, 2026
 *      Author: rob
 */

#include "ch.h"
#include "hal.h"

#ifndef USERLIB_INCLUDE_GPIB_H_
#define USERLIB_INCLUDE_GPIB_H_

#define PROLOGIX_VERSION "AVR488 GPIB-USB Controller Version 6.00" // firmware version displayed by the ++ver command (for Prologix compatibility)

// GPIB Signal Lines
#define DAV  PAL_LINE(GPIOB, 4)
#define NRFD PAL_LINE(GPIOB, 10)
#define NDAC PAL_LINE(GPIOB, 2)
#define IFC  PAL_LINE(GPIOB, 1)
#define SRQ  PAL_LINE(GPIOA, 10)
#define ATN  PAL_LINE(GPIOA, 15)
#define REN  PAL_LINE(GPIOB, 0)
#define EOI  PAL_LINE(GPIOB, 3)
#define DC   PAL_LINE(GPIOA, 9)
#define TEx  PAL_LINE(GPIOC, 15)
#define PEx  PAL_LINE(GPIOB, 5)
#define SC   PAL_LINE(GPIOC, 14)

#define DIO1 PAL_LINE(GPIOA, 0)
#define DIO2 PAL_LINE(GPIOA, 1)
#define DIO3 PAL_LINE(GPIOA, 2)
#define DIO4 PAL_LINE(GPIOA, 3)
#define DIO5 PAL_LINE(GPIOA, 4)
#define DIO6 PAL_LINE(GPIOA, 5)
#define DIO7 PAL_LINE(GPIOA, 6)
#define DIO8 PAL_LINE(GPIOA, 7)

// GPIB command bytes
#define CMD_DCL 0x14
#define CMD_UNL 0x3f
#define CMD_UNT 0x5f
#define CMD_GET 0x08
#define CMD_SDC 0x04
#define CMD_LLO 0x11
#define CMD_GTL 0x01
#define CMD_SPE 0x18
#define CMD_SPD 0x19

// Defines for pin control using GPIB negative logic
#define ASSERT output_low
#define DEASSERT output_high
#define FLOAT output_float

THD_FUNCTION(GPIB_Thread, arg);

void set_eos_string(uint8_t code);
uint8_t srq_state(void);
void init_GPIB(void);

#endif /* USERLIB_INCLUDE_GPIB_H_ */

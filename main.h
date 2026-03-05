/*
 * main.h
 *
 *  Created on: Sep 23, 2020
 *      Author: rob
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <string.h>
#include "ch.h"
#include "hal.h"

#include "shell.h"
#include "chprintf.h"
#include "comm.h"

#define EXTBTN PAL_LINE(GPIOA, 0U)  // External button
#define LED    PAL_LINE(GPIOC, 13)  // LED on Blackpill
#define DEBUG1 PAL_LINE(GPIOB, 8)   // Debug 1
#define DEBUG2 PAL_LINE(GPIOB, 9)   // Debug 2

void flush_buffer(void);
typedef struct{
  uint8_t lastchar;
  uint8_t src;
  systime_t timestamp;
} lst_st;

#endif /* MAIN_H_ */

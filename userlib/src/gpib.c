/*
 * gpib.c
 *
 *  Created on: Mar 5, 2026
 *      Author: rob
 */

#include "ch.h"
#include "hal.h"
#include <string.h>
#include <stdlib.h>
#include "portab.h"
#include "shell.h"
#include "chprintf.h"
#include "comm.h"
#include "gpib.h"

THD_WORKING_AREA(waGPIB_Thread, 128);

THD_FUNCTION(GPIB_Thread, arg) {
  (void)arg;
  static uint8_t disable = 0;
  chRegSetThreadName("GPIB_Thread");
  while (true) {
    //check_data();
    chThdSleepMilliseconds(50);
  }
}

void output_low(uint32_t line){
  palSetLineMode(line, PAL_MODE_OUTPUT_PUSHPULL);
  palClearLine(line);
}

void output_high(uint32_t line){
  palSetLineMode(line, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLine(line);
}

void output_float(uint32_t line){
  palSetLineMode(line, PAL_STM32_MODE_INPUT);
  //palSetLine(line);
}

uint8_t input(uint32_t line){
  palSetLineMode(line, PAL_STM32_MODE_INPUT);
  return (palReadLine(line) == PAL_HIGH)?1:0;
}

void init_GPIB(void){
	
}
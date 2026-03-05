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

void init_GPIB(void){
	
}
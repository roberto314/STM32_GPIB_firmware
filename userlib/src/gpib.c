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

uint8_t partnerAddress = 0; // GPIB address of instrument we're communicating with 
uint8_t autoread = 1;   // whether to echo characters received from the USB port to the user
uint8_t localecho = 1;  // whether to automatically query an instrument (if there's a "?" in a command string)
uint8_t eoiUse;         // whether we are using EOI to signal end of message from instrument
uint8_t eot_enable;     // whether to append eot_char after EOI
uint8_t eot_char;       // end of transmission character added after EOI
uint8_t eos_code;       // end of transmission code to end a transmission to the instrument
uint8_t eos_string[3];  // characters to end a transmission to the instrument
uint16_t timeout;       // used for send/receive timeout monitoring - default timeout is 1000 milliseconds
uint8_t mode;

/* Function Prototypes  */
void output_low(uint32_t line);
void output_high(uint32_t line);
void output_float(uint32_t line);
uint8_t input(uint32_t line);
uint8_t srq_state(void);

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

/*
* Populate the GPIB end of string terminators
*/
void set_eos_string(uint8_t code){
    switch (code)    {
      case 0:
        eos_code = 0;
        eos_string[0] = '\r';
        eos_string[1] = '\n';
        eos_string[2] = 0x00;
        break;
      case 1:
        eos_code = 1;
        eos_string[0] = '\r';
        eos_string[1] = 0x00;
        break;
      case 2:
        eos_code = 2;
        eos_string[0] = '\n';
        eos_string[1] = 0x00;
        break;
      default:
        eos_code = 3;
        eos_string[0] = 0x00;
        break;
    }
}

/*
* Fetch (and invert - due to negative logic) the state of the SRQ bus line
*/
uint8_t srq_state(void){
    return !input(SRQ); // invert due to negative logic
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
/*
 * comm.c
 *
 *  Created on: 04.02.2018
 *      Author: Anwender
 */



#include "ch.h"
#include "hal.h"
#include "comm.h"

#include "chprintf.h"
//#include "chscanf.h"
#include "stdlib.h"
#include "string.h" /* for memset */
#include "shell.h"
#include "portab.h"
#include "main.h"
#include "gpib.h"

extern BaseSequentialStream *const dbg; //DEBUGPORT

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  char text[10];
  uint16_t val;

  chprintf(chp, "Enter Number (<256) \r\n");
//  ret = chscanf(chp, "%7s", &text);

  val = (uint16_t)strtol(text, NULL, 0);

  chprintf(chp, "You entered text: %s Val: %04x got: %02x\r\n",
                      text, val);
  chprintf(dbg, "OK\r\n");

}

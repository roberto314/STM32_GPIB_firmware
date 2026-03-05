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
extern mailbox_t mb_dbg;
extern uint8_t debuglevel;
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
void cmd_debug(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Enter Debug Level. It is now: 0x%02X\r\n", debuglevel);
    chprintf(chp, "One set bit is for one source.\r\n");
    chprintf(chp, "Input can be in HEX, DEC or OCT.\r\n");
    chprintf(chp, "If Base 2 is needed enter '2' as another parameter.\r\n");
    chprintf(chp, "Debug Sources are: \r\n");
    chprintf(chp, "     Bit0: Button.\r\n");
    chprintf(chp, "     Bit1: .\r\n");
    chprintf(chp, "     Bit2: .\r\n");
    chprintf(chp, "     Bit3: .\r\n");
    chprintf(chp, "     Bit4: .\r\n");
    chprintf(chp, "     Bit5: .\r\n");
    chprintf(chp, "     Bit6: .\r\n");
    chprintf(chp, "     Bit7: .\r\n");
    return;
  }
  if (argc > 1){
    val = (uint16_t)strtol(argv[0], NULL, 2);
    chprintf(chp, "Use Base 2!.\r\n");
  }
  else{
    val = (uint16_t)strtol(argv[0], NULL, 0);
  }
  chprintf(chp, "You entered Val: %0d \r\n", val);
  if (val < 256){
    debuglevel = (uint8_t)val;
    chprintf(chp, "Debuglevel changed to: %0d or 0x%02X \r\n", debuglevel, debuglevel);
  }
  else{
    chprintf(chp, "Value too big, Debuglevel NOT changed. Still: 0x%02X \r\n", debuglevel);
  }
}

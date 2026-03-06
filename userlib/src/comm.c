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
extern uint8_t debuglevel, partnerAddress, autoread, localecho, eoiUse, eot_enable, eos_code, eot_char, mode;
extern uint16_t timeout;

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

void cmd_gpib_cmd(BaseSequentialStream *chp, int argc, char *argv[]) {
  char *name = argv[argc-1];
  chprintf(chp, "GPIB Command: %s len: %d\r\n", name, argc);
  for (int i=0;i<argc;i++){
    chprintf(chp, "Argument %d: %s\r\n", i, argv[i]);
  }
}

void cmd_help(BaseSequentialStream *chp, int argc, char *argv[]) {
    chprintf(chp, "addr 1-30      Tell controller which instrument to address.\n\r");
    chprintf(chp, "auto 0|1       Enable (1) or disable (0) read after write.\n\r");
    chprintf(chp, "default        Emulates the Arduino AR488 command.\n\r");
    chprintf(chp, "debug N        Set level for debugging messages.\n\r");
    chprintf(chp, "echo 0|1       Enable (1) or disable (0) echoing of characters received from USB port.\n\r");
    chprintf(chp, "eoi 0|1        Enable (1) or disable (0) EOI with last byte.\n\r");
    chprintf(chp, "eos 0|1|2|3    EOS terminator — 0:CR+LF, 1:CR, 2:LF, 3:None.\n\r");
    chprintf(chp, "eot_char N     End of transmission character ASCII code.\n\r");
    chprintf(chp, "eot_enable N   End of transmission character enable (1) or disable (0).\n\r");
    chprintf(chp, "srq            Query status of SRQ line. 0: Not asserted, 1:Asserted.\n\r");
}

void cmd_addr(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Enter address. Current address: 0x%02X\r\n", partnerAddress);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 31){
    partnerAddress = (uint8_t)val;
    chprintf(chp, "Address changed to: %0d or 0x%02X \r\n", partnerAddress, partnerAddress);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", partnerAddress);
  }
}

void cmd_auto(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Enable Read after Write. Current setting: %d\r\n", autoread);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 2){
    autoread = (uint8_t)val;
    chprintf(chp, "Autoread changed to: %0d or 0x%02X \r\n", autoread, autoread);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", autoread);
  }
}

void cmd_localecho(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Enable Local Echo. Current setting: %d\r\n", localecho);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 2){
    localecho = (uint8_t)val;
    chprintf(chp, "Localecho changed to: %0d or 0x%02X \r\n", localecho, localecho);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", localecho);
  }
}

void cmd_default(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;

  localecho = 0;
  autoread = 0;
  eot_enable = 0;
  eot_char = 0;
  eoiUse = 0;
  eos_code = 0;
  set_eos_string(eos_code);
  timeout = 1200;
  debuglevel = 0;
}

void cmd_eoi(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Enable EOI. Current setting: %d\r\n", eoiUse);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 2){
    eoiUse = (uint8_t)val;
    chprintf(chp, "EOI changed to: %0d or 0x%02X \r\n", eoiUse, eoiUse);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", eoiUse);
  }
}

void cmd_eos(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Change EOS. Current setting: %d\r\n", eos_code);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 4){
    eos_code = (uint8_t)val;
    set_eos_string(eos_code);
    chprintf(chp, "EOI changed to: %0d or 0x%02X \r\n", eos_code, eos_code);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", eos_code);
  }
}

void cmd_eot(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Change EOT Character. Current setting: %d\r\n", eot_char);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 256){
    eot_char = (uint8_t)val;
    chprintf(chp, "EOT Char. changed to: %0d or 0x%02X \r\n", eot_char, eot_char);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", eot_char);
  }
}

void cmd_eot_en(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Enable EOT. Current setting: %d\r\n", eot_enable);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 2){
    eot_enable = (uint8_t)val;
    chprintf(chp, "EOT changed to: %0d or 0x%02X \r\n", eot_enable, eot_enable);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", eot_enable);
  }
}

void cmd_mode(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Change Mode. Current setting: %d\r\n", mode);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 2){
    mode = (uint8_t)val;
    chprintf(chp, "Mode changed to: %0d or 0x%02X \r\n", mode, mode);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", mode);
  }
}

void cmd_ver(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  chprintf(chp, "%s\r\n", PROLOGIX_VERSION);
}

void cmd_srq(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint8_t srq = srq_state();
  if (srq)
    chprintf(chp, "SRQ Asserted\r\n");
  else
    chprintf(chp, "SRQ NOT Asserted\r\n");
}

void cmd_trg(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  chprintf(chp, "TRG TODO\r\n");
}

void cmd_spoll(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  chprintf(chp, "SPOLL TODO\r\n");
}

void cmd_ifc(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  chprintf(chp, "IFC TODO\r\n");
}

void cmd_llo(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  chprintf(chp, "LLO TODO\r\n");
}

void cmd_loc(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  chprintf(chp, "LOC TODO\r\n");
}

void cmd_clr(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  chprintf(chp, "CLR TODO\r\n");
}

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
  //chprintf(chp, "You entered Val: %0d \r\n", val);
  if (val < 256){
    debuglevel = (uint8_t)val;
    chprintf(chp, "Debuglevel changed to: %0d or 0x%02X \r\n", debuglevel, debuglevel);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", debuglevel);
  }
}

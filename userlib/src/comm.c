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
extern uint8_t eos_char, listen_only, status_byte;
extern uint16_t timeout_value;

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

void cmd_gpib_cmd(BaseSequentialStream *chp, int argc, char *argv[]) {
  char *name = argv[argc-1]; // the last argument is the actual name
  if (debuglevel & 2){
    chprintf(chp, "GPIB Out: %s len: %d\r\n", name, argc);
    for (int i=0;i<argc;i++){
      chprintf(chp, "Argument %d: %s\r\n", i, argv[i]);
    }
  }
  do_gpib_command(name);
}

void cmd_addr(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Change target address. Now: %d\r\n", partnerAddress);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 31){
    partnerAddress = (uint8_t)val;
    chprintf(chp, "target address changed to: %0d or 0x%02X \r\n", partnerAddress, partnerAddress);
  }
  else{
    chprintf(chp, "Invalid instrument address, still: 0x%02X \r\n", partnerAddress);
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

void cmd_clr(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  do_clr();
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
    chprintf(chp, "     Bit1: GPIB High Level Commands.\r\n");
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
  timeout_value = 1200;
  debuglevel = 0;
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
    chprintf(chp, "EOS changed to: %0d or 0x%02X \r\n", eos_code, eos_code);
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

void cmd_help(BaseSequentialStream *chp, int argc, char *argv[]) {
    chprintf(chp, "Available commands (all are preceded by \"++\" and ARE case sensitive):\n\r");
    chprintf(chp, "addr 1-30      Tell controller which instrument to address.\n\r");
    chprintf(chp, "auto 0|1       Enable (1) or disable (0) read after write.\n\r");
    chprintf(chp, "clr            Issue device clear.\n\r");
    chprintf(chp, "debug N        Set level for debugging messages.\n\r");
    chprintf(chp, "default        Emulates the Arduino AR488 command.\n\r");
    chprintf(chp, "echo 0|1       Enable (1) or disable (0) echoing of characters received from USB port.\n\r");
    chprintf(chp, "eoi 0|1        Enable (1) or disable (0) EOI with last byte.\n\r");
    chprintf(chp, "eos 0|1|2|3    EOS terminator — 0:CR+LF, 1:CR, 2:LF, 3:None.\n\r");
    chprintf(chp, "eot_char N     End of transmission character ASCII code.\n\r");
    chprintf(chp, "eot_enable N   End of transmission character enable (1) or disable (0).\n\r");
    chprintf(chp, "ifc            Issue interface clear.\n\r");
    chprintf(chp, "llo            Disable front panel operation of the currently addressed instrument.\n\r");
    chprintf(chp, "loc            Enable front panel operation of the currently addressed instrument.\n\r");
    chprintf(chp, "lon  0|1       Change listen only mode (only if mode == Listener).\n\r");
    chprintf(chp, "mode 0|1       0: Listener, 1: Talker.\n\r");
    chprintf(chp, "read           Read from instrument until timeout.\n\r");
    chprintf(chp, "read eoi       Read from instrument until EOI detected or timeout.\n\r");
    chprintf(chp, "read_tmo_ms N  Timeout value, in milliseconds, for read and spoll commands.\n\r");
    chprintf(chp, "spoll [N]      Read status byte by serial polling the instrument.\n\r");
    chprintf(chp, "status [N]     Read or change status byte.\n\r");
    chprintf(chp, "srq            Query status of SRQ line. 0: Not asserted, 1:Asserted.\n\r");
    chprintf(chp, "trg [N]        Issue device trigger (GET) to the instrument.\n\r");
    chprintf(chp, "ver            Query AVR488 interface version.\n\r");
}

void cmd_ifc(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  do_ifc();
}

void cmd_llo(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  do_llo();
}

void cmd_loc(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  do_loc();
}

void cmd_get(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "GET from target address. Now: %d\r\n", partnerAddress);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 31){
    do_get((uint8_t)val);
    chprintf(chp, "GET from address: %0d or 0x%02X \r\n", val, val);
  }
  else{
    chprintf(chp, "Invalid instrument address, still: 0x%02X \r\n", partnerAddress);
  }
}

void cmd_lon(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Enable listen_only. Current setting: %d, mode: %d\r\n", listen_only, mode);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 2){
    if (mode == 0){
      listen_only = (uint8_t)val;
      chprintf(chp, "listen_only changed to: %0d or 0x%02X \r\n", listen_only, listen_only);
    }
    else{
      chprintf(chp, "listen_only NOT changed because we are in Controller Mode!\r\n");
    }
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", listen_only);
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
    do_change_mode();
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", mode);
  }
}

void cmd_read_tmo_ms(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint32_t val;
  if (argc < 1){
    chprintf(chp, "Enter timeout. Current timeout: %d ms\r\n", timeout_value);
    return;
  }
  val = (uint32_t)strtol(argv[0], NULL, 0);
  if (val < 65536){
    timeout_value = (uint16_t)val;
    chprintf(chp, "timeout changed to: %0d ms \r\n", timeout_value);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: %d ms \r\n", timeout_value);
  }
}

void cmd_read(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  if (argc == 1){
    if (argv[0][0] == 'e'){
      gpib_read(true); // read until EOI flagged
    }
  }
  else{
    gpib_read(false);  // read until EOS condition
  }
}

void cmd_spoll(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc > 0){
    val = (uint16_t)strtol(argv[0], NULL, 0);
    if (val < 31){
      serial_poll((uint8_t)val);
    }
  }
  else{
    serial_poll(partnerAddress);
  }
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

void cmd_status(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  if (argc < 1){
    chprintf(chp, "Change Status byte. Now: %d\r\n", status_byte);
    return;
  }
  val = (uint16_t)strtol(argv[0], NULL, 0);
  if (val < 256){
    status_byte = (uint8_t)val;
    chprintf(chp, "Status byte changed to: %0d or 0x%02X \r\n", status_byte, status_byte);
  }
  else{
    chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", status_byte);
  }
}

void cmd_trg(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  uint16_t val;
  uint8_t adr = partnerAddress;
  if (argc > 0){
    val = (uint16_t)strtol(argv[0], NULL, 0);
    if (val < 31){
      adr = (uint8_t)val;
    }
    else{
      chprintf(chp, "Value too big, NOT changed. Still: 0x%02X \r\n", partnerAddress);
      return;
    }  
  }
  do_trg(adr);
}

void cmd_ver(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  chprintf(chp, "%s\r\n", PROLOGIX_VERSION);
}

void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)* argv;
  (void)argc;
  char text[10];
  uint16_t val = 0;
  uint8_t section = 0;
  if (argc > 0){
    section = (uint8_t)strtol(argv[0], NULL, 0);
  }
  if (argc > 1){
    val = (uint16_t)strtol(argv[1], NULL, 0);
  }
  switch (section){
  case 1:
    chprintf(dbg, "Enter Pintest.\r\n");
    if (argc == 1){ // no value given
      palSetGroupMode(GPIOB, PAL_GROUP_MASK(8), 0, PAL_MODE_INPUT_PULLUP); // set PB0..7 as input
    }
    else{
      palSetGroupMode(GPIOB, PAL_GROUP_MASK(8), 0, PAL_MODE_OUTPUT_PUSHPULL); // set PB0..7 as output
      palWriteGroup(GPIOB, PAL_GROUP_MASK(8), 0, val); // write to PB0..7
    }
    break;
  default:
    chprintf(chp, "You must enter a section (1-255) \r\n");
    break;
  }
}


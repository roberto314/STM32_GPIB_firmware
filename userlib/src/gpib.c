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
#include "ctype.h"

extern BaseSequentialStream *const dbg; //DEBUGPORT
extern uint8_t debuglevel;
uint8_t partnerAddress = 2; // GPIB address of instrument we're communicating with 
uint8_t autoread = 1;   // whether to echo characters received from the USB port to the user
uint8_t localecho = 1;  // whether to automatically query an instrument (if there's a "?" in a command string)
uint8_t eoiUse = 1;     // whether we are using EOI to signal end of message from instrument
uint8_t eot_enable = 0; // whether to append eot_char after EOI
uint8_t eot_char = 13;  // end of transmission character added after EOI (default = CR)
uint8_t eos_code = 3;   // end of transmission code to end a transmission to the instrument
uint8_t eos_char;       // Default end of string character. Gets filled by set_eos_string()
uint8_t eos_string[3];  // characters to end a transmission to the instrument
uint16_t timeout_value = 1000; // used for send/receive timeout monitoring - default timeout is 1000 milliseconds
uint8_t mode = 1;
uint8_t myAddress;
uint8_t listen_only = 0;
uint8_t cmd_buf[10];
uint8_t writeError;
volatile uint8_t timeout = 0;

// Variables for device mode
uint8_t device_talk = false;
uint8_t device_listen = false;
uint8_t device_srq = false;
uint8_t status_byte = 0;


/* Function Prototypes  */
void output_low(uint32_t line);
void output_high(uint32_t line);
void output_float(uint32_t line);
void prep_gpib_pins(void);
uint8_t input(uint32_t line);
uint8_t srq_state(void);

static void gptcb(GPTDriver *drv){
  (void)drv;
  timeout = 1;
  //gptStopTimerI(&GPTD4);
}
/*
 * GPT4 configuration.
 */
static const GPTConfig gptcfg1 = {
  .frequency    = 10000U, //10kHz Timer Freq., 100us/tic, 
  .callback     = gptcb,
  .cr2          = 0U,
  .dier         = 0U
};

static THD_WORKING_AREA(waGPIB_Thread, 128);

static THD_FUNCTION(GPIB_Thread, arg) {
  (void)arg;
  static uint8_t disable = 0;
  chRegSetThreadName("GPIB_Thread");
  while (true) {
    //check_data();
    chThdSleepMilliseconds(50);
  }
}

void myputchar (uint8_t c){
  //streamPut(dbg, c);
  chprintf(dbg, "%c",c);
}

void _wdt_reset (void){

}

void _delay_us (uint16_t c){
  chThdSleepMicroseconds(c);
}

/*
 * Puts all the GPIB pins into their correct initial states.
 */
void prep_gpib_pins(void){
  output_low(TEx); // Disable Talk Enable for data and handshake lines
  output_low(PEx); // Enable Pullups for bus transceivers
  //output_high(PEx); // Disable Pullups for bus transceivers

  if (mode) {
    output_high(SC); // Allows transmit on ATN/REN/IFC
    output_low(DC);  // Transmit ATN and receive SRQ
  } 
  else {
    output_low(SC);  // 
    output_high(DC); // 
  }
  // tri-state data pins
  palSetGroupMode(GPIOB, PAL_GROUP_MASK(8), 0, PAL_MODE_INPUT_PULLUP); // set PB0..7 as input

  if (mode) {
    output_high(ATN);
    output_float(EOI);
    output_float(DAV);
    output_low(NRFD);
    output_low(NDAC);
    output_high(IFC);
    output_float(SRQ);
    output_low(REN);
  } 
  else {
    output_float(ATN);
    output_float(EOI);
    output_float(DAV);
    output_float(NRFD);
    output_float(NDAC);
    output_float(IFC);
    output_float(SRQ);
    output_float(REN);
  }
}

/*
 * Send a multibyte GPIB data string (command or data).
 */
uint8_t _gpib_write(uint8_t *bytes, uint16_t length, uint8_t attention, uint8_t useEOI){
    /*
    * Write a string of bytes to the bus
    * bytes: array containing characters to be written
    * length: number of bytes to write, 0 if not known.
    * attention: 1 if this is a gpib command, 0 for data
    */
  uint8_t a;   // Loop counter variable
  uint16_t i;  // Storage variable for the current character
  uint8_t temp1, temp2;
  /*
  * Returns true if there was an error
  */
  output_high(PEx);  // Disable Pullups for bus transceivers

  if(attention){     // If byte is a GPIB bus command
    output_low(ATN); // Assert the ATN line, informing all that this is a command byte
  }

  if(length==0){ // If the length was unknown
    length = strlen((char *)bytes); // Calculate the number of bytes to be sent
  }

  output_high(TEx); // Enable talking
  output_high(EOI);
  output_high(DAV);
  output_float(NRFD); // Let talkers control NRFD and NDAC
  output_float(NDAC);

  if (debuglevel & 2){
    temp1 = input(NDAC);
    temp2 = input(NRFD);
    chprintf(dbg, "NDAC:%x / NRFD:%x\n\r", temp1, temp2);
  }

  timeout = 0;
  gptStartContinuous(&GPTD4, (uint32_t)timeout_value/10);
  while((input(NDAC) || !(input(NRFD))) && (!timeout)) {
    _wdt_reset();
    if(timeout) {
      if (debuglevel & 2) {
        chprintf(dbg, "Timeout: Before writing %c %x ", bytes[0], bytes[0]);
      }
      device_talk = false;
      device_srq = false;
      prep_gpib_pins();
      return 1;
    }
  }
  gptStopTimer(&GPTD4);

  //--------------------------------------------------------------------------------------
  for (i=0; i<length; i++){ //Loop through each character, write to bus
    a = bytes[i]; // Character to send
    // Wait for listeners to be ready for data (NRFD should be high and NDAC low)
    // Wait for NDAC to go low, indicating previous bit is now done with
    timeout = 0;
    gptStartContinuous(&GPTD4, (uint32_t)timeout_value/10);
    while(input(NDAC) && !timeout){ // wait as long as timeout is low and (NRFD == 0 or NDAC == 1)
      if(timeout){
        if (debuglevel & 2)
          chprintf(dbg, "Timeout: Waiting for NRFD to go high and NDAC to go low while writing\n\r");
        device_talk = false;
        device_srq = false;
        prep_gpib_pins();
        return 1;
      }
    }

    gptStopTimer(&GPTD4);

    if (debuglevel & 2){
      if (attention)
        chprintf(dbg, "Writing command byte: 0x%02x\n\r", a);
      else
        chprintf(dbg, "Writing data byte: %c 0x%02x\n\r", isprint(a)?a:' ', a);
    }

    // Enable port B for output and put the byte on the data lines using negative logic
    palSetGroupMode(GPIOB, PAL_GROUP_MASK(8), 0, PAL_MODE_OUTPUT_PUSHPULL); // set PB0..7 as output
    palWriteGroup(GPIOB, PAL_GROUP_MASK(8), 0, ~a);

    // Wait for listeners to be ready for data (NRFD should be high)
    timeout = 0;
    gptStartContinuous(&GPTD4, (uint32_t)timeout_value/10);
    while(!(input(NRFD)) && !timeout) {
      _wdt_reset();
      if(timeout) {
        if (debuglevel == 1) {
          chprintf(dbg, "Timeout: Waiting for NRFD to go high while writing%c", eot_char);
        }
        device_talk = false;
        device_srq = false;
        prep_gpib_pins();
      return 1;
      }
    }
    gptStopTimer(&GPTD4);

    if((i==length-1) && (useEOI)) { // If last byte in string
    //if((i==length-1) && eoiUse && !attention){
      output_low(EOI); // If last byte in string assert EOI
        if (debuglevel & 2)
          chprintf(dbg, "Asserting EOI\n\r");
    }
    _delay_us(150); // delay to allow lines to settle
    output_low(DAV); // Inform listeners that the data is ready to be read
    _delay_us(150); // delay to allow lines to settle

    // Wait for NDAC to go high, all listeners have accepted the byte
    timeout = 0;
    gptStartContinuous(&GPTD4, (uint32_t)timeout_value/10);
    while(!(input(NDAC)) && (!timeout)){
      _wdt_reset();
      if(timeout) {
          if (debuglevel & 2)
              chprintf(dbg, "Timeout: Waiting for NDAC to go high while writing\n\r");
        device_talk = false;
        device_srq = false;
        prep_gpib_pins();
        return 1;
      }
    }
    gptStopTimer(&GPTD4);

    output_high(DAV); // Byte has been accepted by all, indicate the byte is no longer valid

  } // Finished outputting all bytes to listeners
  //--------------------------------------------------------------------------------------
  output_low(TEx); // Disable talking on data lines
  palSetGroupMode(GPIOB, PAL_GROUP_MASK(8), 0, PAL_MODE_INPUT_PULLUP); // set PB0..7 as input

  if(attention){     // If byte was a GPIB command byte release ATN line
    output_high(ATN);
  }
  output_float(DAV);
  output_float(EOI);
  output_high(NDAC);
  output_high(NRFD);
  output_low(PEx);  // Enable Pullups for bus transceivers
  return 0;
}

/*
 * Send a multibyte GPIB command (ATN line asserted).
 */
uint8_t gpib_cmd(uint8_t *bytes, uint16_t length){
  if (debuglevel & 2)
    chprintf(dbg, "GPIB command. len: %d \r\n", length);
  
  // Write a GPIB CMD byte to the bus
  return _gpib_write(bytes, length, 1, 0);
}

/*
 * Make ourselves the bus controller.
 */
uint8_t gpib_controller_assign(uint8_t address){
  myAddress = address;
  output_low(IFC); // Assert interface clear. Resets bus and makes it controller in charge.

  _wdt_reset();
  _delay_us(150);
  output_float(IFC); // Finishing clearing interface

  output_low(REN); // Put all connected devices into "remote" mode
  cmd_buf[0] = CMD_DCL;
  return gpib_cmd(cmd_buf, 1); // Send GPIB DCL command, clear all devices on bus
}

/*
 * Send a multibyte GPIB data string (ATN line not asserted).
 */
uint8_t gpib_write(uint8_t *bytes, uint16_t length, uint8_t useEOI){
  if (debuglevel & 2)
    chprintf(dbg, "gpib_write(): \"%s\"\n\r",bytes);
    // Write a GPIB data string to the bus
  return _gpib_write(bytes, length, 0, useEOI);
}

/*
 * Receive a single byte from the GPIB bus.
 */
uint8_t gpib_receive(uint8_t *byte){
  uint8_t a = 0; // Storage for received character
  uint8_t eoiStatus; // Returns 0x00 or 0x01 depending on status of EOI line or 0xff for error

  // Raise NRFD, telling the talker we are ready for the byte
  output_high(NRFD);

  // Assert NDAC informing the talker we have not accepted the byte yet
  output_low(NDAC);

  // Let the talker control the data valid line
  output_float(DAV);

  // Wait for DAV to go low (talker informing us the byte is ready)
  timeout = 0;
  gptStartContinuous(&GPTD4, (uint32_t)timeout_value*10);
  while (input(DAV) && (!timeout)) {
    _wdt_reset();
    if (timeout){
      if (debuglevel & 2)
        chprintf(dbg, "Timeout: Waiting for DAV to go low while reading\n\r");
      prep_gpib_pins();
      return 0xff;
    }
  }
  gptStopTimer(&GPTD4);

  // Assert NRFD, informing talker to not change the data lines
  output_low(NRFD);

  // Read port A, where the data lines are connected
  a = ~palReadGroup(GPIOB, PAL_GROUP_MASK(8), 0); // Flip all bits since GPIB uses negative logic.
  eoiStatus = input(EOI);
  _delay_us(150);

  if (debuglevel & 2)
    chprintf(dbg, "Read byte: %c 0x%02x\n\r", isprint(a)?a:' ', a);

  // Deassert NDAC, informing talker that we have accepted the byte
  output_float(NDAC);
  _delay_us(150);

  // Wait for DAV to go high (talker knows that we have read the byte)
  timeout = 0;
  gptStartContinuous(&GPTD4, (uint32_t)timeout_value*10);
  while(!(input(DAV)) && (!timeout)) {
    _wdt_reset();
    if(timeout){
      if (debuglevel & 2){
        chprintf(dbg, "Timeout: Waiting for DAV to go high while reading\n\r");
      }
      device_listen = false;
      prep_gpib_pins();
      return 0xff;
    }
  }
  gptStopTimer(&GPTD4);

  // Prep for next byte, we have not accepted anything
  output_low(NDAC);

  if (debuglevel & 2){
    chprintf(dbg, "EOI: %x\n\r", (eoiStatus>0)?0:1); // invert EOI (negative logic)
  }
  *byte = a;
  return eoiStatus;
}

//palSetLine(DEBUG1);
//palClearLine(DEBUG1);

/*
 * Receive a multiple bytes from the GPIB bus until either EOI or timeout. Returns non-zero if an error occurs.
 */
uint8_t gpib_read(uint8_t read_until_eoi){
  uint8_t readCharacter, eoiStatus;
  uint8_t errorFound = 0;
  uint8_t reading_done = false;
  //uint8_t readBuf[100];
  //uint8_t i = 0, j=0;
  //uint8_t *bufPnt;
  //bufPnt = &readBuf[0];
  
  if (debuglevel & 2){
    chprintf(dbg, "GPIB read. EOI: %d Address: %d\r\n", read_until_eoi, partnerAddress);
  }
  if (mode) {
    cmd_buf[0] = CMD_UNT;
    cmd_buf[1] = CMD_UNL;
    cmd_buf[2] = myAddress + 0x20;      // Set the controller into listener mode
    cmd_buf[3] = partnerAddress + 0x40; // Set target device into talker mode
    errorFound = gpib_cmd(cmd_buf, 3);
    if (errorFound)
      return 1;
  }

  if (read_until_eoi == 1){ // loop until we get an EOI indication
    if (debuglevel & 2) chprintf(dbg, "gpib_read eoi...\n\r");
    do {
      eoiStatus = gpib_receive(&readCharacter); // eoiStatus is line level of EOI
      _wdt_reset();
      if (!debuglevel){
        myputchar(readCharacter);
      }
      if(eoiStatus == 0xff){
        errorFound = 1;
        break;
      }
    } while (eoiStatus); // as long as EOI is high

    if (eot_enable){
      myputchar(eot_char);
    }
  }
  else {                   // loop until we get an EOS character
    if (debuglevel & 2) chprintf(dbg, "gpib_read eos...\n\r");
    do {
      eoiStatus = gpib_receive(&readCharacter); // eoiStatus is line level of EOI
      _wdt_reset();
      if (!debuglevel){
        myputchar(readCharacter);
      }
      if (eoiStatus==0xff){
        errorFound = 1;
        break;
      }
      if (eos_code != 3) { // means we are looking for a CR or LF or a combination of these
        if (readCharacter == eos_string[0]) // Check for EOM chars
            reading_done = true;
        if (readCharacter == eos_string[1])
              reading_done = true;
      }
    } while (reading_done == false);
  }

  if (mode){
    // Command all talkers and listeners to stop
    cmd_buf[0] = CMD_UNT;
    cmd_buf[1] = CMD_UNL;
    errorFound = gpib_cmd(cmd_buf, 2);
  }

  if (debuglevel & 2)
    chprintf(dbg, "gpib_read end (error=%d\n\r", errorFound);

  return errorFound;
}

/*
* Address the currently specified GPIB address (as set by the ++addr cmd) to listen
*/
uint8_t addressTarget(uint8_t address){
    cmd_buf[0] = CMD_UNT; // everyone stop talking
    cmd_buf[1] = CMD_UNL; // everyone stop listening
    cmd_buf[2] = address + 0x20; // tell addressed device to listen
    return gpib_cmd(cmd_buf, 3);
}

/*
* Fetch (and invert - due to negative logic) the state of the SRQ bus line
*/
uint8_t srq_state(void){
    return !input(SRQ); // invert due to negative logic
}

/*
* Fetch the instrument status byte using a serial poll command
*/
void serial_poll(uint8_t address){
  uint8_t error = 0;
  uint8_t status_byte;
  uint8_t buffer [10];

  cmd_buf[0] = CMD_SPE; // enable serial poll
  cmd_buf[1] = address + 0x40;
  error = gpib_cmd(cmd_buf, 2);
  if (gpib_receive(&status_byte) == 0xff)
    error = 1;
  cmd_buf[0] = CMD_SPD; // disable serial poll
  error = error || gpib_cmd(cmd_buf, 1);
  if (!error){
    chprintf(dbg, "Status: %02X\n\r", status_byte);
  }
  else
      chprintf(dbg, "ERROR in spoll\n\r");
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
        eos_char = '\n';
        break;
      case 1:
        eos_code = 1;
        eos_string[0] = '\r';
        eos_string[1] = 0x00;
        eos_char = '\r';
        break;
      case 2:
        eos_code = 2;
        eos_string[0] = '\n';
        eos_string[1] = 0x00;
        eos_char = '\n';
        break;
      default:
        eos_code = 3;
        eos_string[0] = 0x00;
        eos_char = 0;
        break;
    }
}

void do_ifc(void){
  output_low(IFC); // Assert interface clear.
  _delay_us(150);
  output_float(IFC); // Finishing clearing interface
  if (debuglevel & 2)
    chprintf(dbg, "OK\n\r");
}

void do_llo(void){
  writeError = addressTarget(partnerAddress); // always send UNT, UNL, address+0x20
  cmd_buf[0] = CMD_LLO;
  writeError = writeError || gpib_cmd(cmd_buf,1);
  if (writeError)
    chprintf(dbg, "ERROR in llo\n\r");
  else
    if (debuglevel & 2)
      chprintf(dbg, "OK\n\r");
}

void do_loc(void){
  writeError = addressTarget(partnerAddress); // always send UNT, UNL, address+0x20
  cmd_buf[0] = CMD_GTL;
  writeError = writeError || gpib_cmd(cmd_buf,1);
  if (writeError)
    chprintf(dbg, "ERROR in loc\n\r");
  else
    if (debuglevel & 2)
      chprintf(dbg, "OK\n\r");
}

void do_clr(void){
  writeError = addressTarget(partnerAddress); // always send UNT, UNL, address+0x20
  cmd_buf[0] = CMD_SDC;
  writeError = writeError || gpib_cmd(cmd_buf,1);
  if (writeError)
    chprintf(dbg, "ERROR in clr\n\r");
  else
    if (debuglevel & 2)
      chprintf(dbg, "OK\n\r");
}

void do_get(uint8_t address){
  writeError = addressTarget(partnerAddress); // always send UNT, UNL, address+0x20
  cmd_buf[0] = CMD_GET;
  writeError = writeError || gpib_cmd(cmd_buf,1);
  if (writeError)
    chprintf(dbg, "ERROR in get\n\r");
  else
    if (debuglevel & 2)
      chprintf(dbg, "OK\n\r");
}

void do_trg(uint8_t address){
  writeError = addressTarget(address); // always send UNT, UNL, address+0x20
  cmd_buf[0] = CMD_GET;
  writeError = writeError || gpib_cmd(cmd_buf, 1);
  if (writeError)
    chprintf(dbg, "ERROR in clr\n\r");
  else
    if (debuglevel & 2)
      chprintf(dbg, "OK\n\r");
}

void do_change_mode(void){
  if (mode) {
    gpib_controller_assign(0x00);
  }
  else{
    output_float(REN);
  }
}
/*
* Not an internal command, send to GPIB bus
* Command all talkers and listeners to stop
* and tell target to listen and send out command to the bus
*/
void do_gpib_command(char *cm){
  if (mode) {
    writeError = addressTarget(partnerAddress); // always send UNT, UNL, address+0x20
    // Set the controller into talker mode
    // cmd_buf[0] = myAddress + 0x40;
    // writeError = writeError || gpib_cmd(cmd_buf, 1);
  }

  if (mode || device_talk) {
    if (eos_code != 3){ // If we have an EOS code, need to append termination byte(s)
      writeError = writeError || gpib_write((uint8_t *)cm, 0, 0); // send to the instrument
      if (!writeError)
        writeError = gpib_write(eos_string, 0, eoiUse);
    }
    else {
      writeError = writeError || gpib_write((uint8_t *)cm, 0, 1); // send to the instrument
    }
  }
  
  // If we're in auto mode and command contains a question mark this is a query to the instrument, so automatically read the response
  if (autoread && mode) {
    if ((strchr(cm, '?') != NULL) && !(writeError))
      writeError = gpib_read(eoiUse);
    else {
      if (localecho){
        if (writeError)
          chprintf(dbg, "ERROR in query\n\r");

        else {
          if (debuglevel & 2)
            chprintf(dbg, "OK\n\r");
        }
      }
    }
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
  palSetLineMode(line, PAL_MODE_INPUT_PULLUP);
  //palSetLine(line);
}

uint8_t input(uint32_t line){
  uint8_t retval = 0;
  palSetLineMode(line, PAL_MODE_INPUT_PULLUP);
  if (palReadLine(line) == PAL_HIGH){
    retval = 1;
  }
  else {
    retval = 0;
  }
  //return (palReadLine(line) == PAL_HIGH)?0:1;  // returns the inverted state!
  return retval;
}

void init_GPIB(void){
  //palSetLine(DEBUG1);
  gptStart(&GPTD4, &gptcfg1);
  prep_gpib_pins();
  set_eos_string(eos_code);
  //palClearLine(DEBUG1);
  if (mode) {
    gpib_controller_assign(0x00);
  }
	//chThdCreateStatic(waGPIB_Thread, sizeof(waGPIB_Thread), NORMALPRIO, GPIB_Thread, NULL);
}
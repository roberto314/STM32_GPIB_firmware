*****************************************************************************
** ChibiOS/RT port for ARM-Cortex-M4 STM32F407.                            **
*****************************************************************************

** TARGET **

The demo runs on an STM32F411 or STM32F401 blackpill board.
(25MHz Crystal)
Serial Port over USB (also Debug Interface).

Build:
Build with eclipse or simply type "make" in the code folder.


-----------------   WORK in Progress

Functions:

Pinout:            Project Pins
PA0  - KEY Button | Button
PA1  - free       | REN
PA2  - TX2        | -
PA3  - RX2        | -
PA4  - CS_FLASH   | DEBUG
PA5  - SCK        | -
PA6  - MISO       | -
PA7  - MOSI       | -
PA8  - free       | NDAC
PA9  - TX1        | DC
PA10 - RX1        | SRQ
PA11 - USD DM     | USB
PA12 - USB DP     | USB
PA13 - SWDIO      | -
PA14 - SWCLK      | -
PA15 - free       | ATN
                  | 
PB0  - free       | DIO1 
PB1  - free       | DIO2 
PB2  - free       | DIO3 
PB3  - free       | DIO4 
PB4  - free       | DIO5 
PB5  - free       | DIO6 
PB6  - SCL1       | DIO7 
PB7  - SDA1       | DIO8 
PB8  - SCL1       | -
PB9  - SDA1       | -
PB10 - free       | NRFD
                  | 
PB12 - free       | EOI
PB13 - free       | DAV
PB14 - free       | PE  (not conn.)
PB15 - free       | IFC
                  | 
PC13 - LED        | LED
PC14 - free       | SC
PC15 - free       | TE

**************
**************
** NEEDED!! **
**************
**************

-------------------------------------------------------------------------------
add to os/various/shell/shell.c (line 59)
extern uint8_t localecho;

-------------------------------------------------------------------------------
Replace list_commands in os/various/shell/shell.c with this version:

#if (SHELL_CMD_HELP_ENABLED == TRUE)
static void list_commands(BaseSequentialStream *chp, const ShellCommand *scp) {

  while (scp->sc_name != NULL) {
    chprintf(chp, "%s ", scp->sc_name);
    scp++;
  }
}
#endif

-------------------------------------------------------------------------------
Replace cmdexec in os/various/shell/shell.c with this version:

static bool cmdexec(const ShellCommand *scp, BaseSequentialStream *chp,
                      char *name, int argc, char *argv[]) {

  while (scp->sc_name != NULL) {
    if (strcmp(scp->sc_name, name) == 0) {
      scp->sc_function(chp, argc, argv);
      return false;
    }
    else if (scp->sc_name[0] == '.') {  // execute function with dot as name
      argv[argc++] = name;              // add the actual name at the end
      scp->sc_function(chp, argc, argv);
      return false;
    }
    scp++;
  }
  return true;
}

-------------------------------------------------------------------------------
Replace IN THD_FUNCTION(shellThread, p) the following part 
(also in os/various/shell/shell.c): 

    if (cmd != NULL) {
    #if (SHELL_CMD_HELP_ENABLED == TRUE)
      if (strcmp(cmd, "help") == 0) {
        if (n > 0) {
          shellUsage(chp, "help");
          continue;
        }
        chprintf(chp, "Commands: help ");
        list_commands(chp, shell_local_commands);
        if (scp != NULL)
          list_commands(chp, scp);
        chprintf(chp, SHELL_NEWLINE_STR);
      }
    #else
      if (0){}
    #endif
      else if (cmdexec(shell_local_commands, chp, cmd, n, args) &&
          ((scp == NULL) || cmdexec(scp, chp, cmd, n, args))) {
        chprintf(chp, "%s", cmd);
        chprintf(chp, " ?" SHELL_NEWLINE_STR);
      }
    }
-------------------------------------------------------------------------------
Replace IN: bool shellGetLine(ShellConfig *scfg, char *line, unsigned size, ShellHistory *shp):

    if (p < line + size - 1) {
      if (localecho){
        streamPut(chp, c);
      }
      *p++ = (char)c;
    }


-------------------------------------------------------------------------------
in shecllconf.h add: 

#define SHELL_CMD_HELP_ENABLED    FALSE
-------------------------------------------------------------------------------
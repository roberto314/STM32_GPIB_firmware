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

Pinout:
PA0  - KEY Button
PA1  - free
PA2  - TX2 (listener 2)
PA3  - RX2 (listener 2)
PA4  - free
PA5  - free
PA6  - free
PA7  - free
PA8  - free
PA9  - TX1 (listener 1)
PA10 - RX1 (listener 1)
PA11 - USD DM (Ostrich)
PA12 - USB DP (Ostrich)
PA13 - SWDIO
PA14 - SWCLK
PA15 - free

PB0  - free
PB1  - free
PB2  - free
PB3  - free
PB4  - free
PB5  - free
PB6  - free
PB7  - free
PB8  - free
PB9  - free
PB10 - free

PB12 - free
PB13 - free
PB14 - free
PB15 - free

PC13 - LED
PC14 - Debug
PC15 - Debug


NEEDED!!
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
add to os/various/shell/shell.c (line 59)
extern uint8_t localecho;


-------------------------------------------------------------------------------
in shecllconf.h add: 

#define SHELL_CMD_HELP_ENABLED    FALSE
-------------------------------------------------------------------------------
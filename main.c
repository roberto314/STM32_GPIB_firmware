/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
#include "main.h"
#include "ch.h"
#include "hal.h"
#include <string.h>
#include <stdlib.h>
#include "portab.h"
#include "shell.h"
#include "chprintf.h"

#include "usbcfg.h"
#include "comm.h"
#include "gpib.h"

/*===========================================================================*/
/* Function Prototypes                                                       */
/*===========================================================================*/
void got_char(uint8_t c, uint8_t src);
uint32_t increase_rwcnt(uint32_t cnt);

/*===========================================================================*/
/* Global Variables                                                          */
/*===========================================================================*/
BaseSequentialStream *const shell = (BaseSequentialStream *)&SHELLPORT;
BaseSequentialStream *const dbg = (BaseSequentialStream *)&DEBUGPORT;
extern THD_WORKING_AREA(waGPIB_Thread, 128);
mailbox_t mb_dbg; // Mailbox for Debug Thread
msg_t msgbuf_dbg[8]; // Message Bufer for Debug Thread
uint8_t dbg_src;  // Debug Thread Vars.
uint16_t hi, lo;  // Debug Thread Vars.
uint8_t debuglevel = 0;
/*===========================================================================*/
/* Debug Thread                                                              */
/* we get a msg which is a 32bit value, 32..25 is the source, 24..17 is the  */
/* high byte (not used for now) and 16..0  the low word */
/*===========================================================================*/
static THD_WORKING_AREA(waDebugThread1, 128);
THD_FUNCTION(DebugThread1, arg) {
  (void)arg;
  msg_t msg;
  chMBObjectInit(&mb_dbg, msgbuf_dbg, 8);

  while (true) {
    chMBFetchTimeout(&mb_dbg, &msg, TIME_INFINITE);
    dbg_src = (uint8_t)((msg >> 24) & debuglevel); //one bit in debuglevel is for one source
    //hi = (uint16_t)(msg >> 16);
    lo = (uint16_t)(msg);
    if (debuglevel){  // debug only when debuglevel > 0
      switch (dbg_src){
        case 1:
          switch (lo){
            case 1:
              chprintf(dbg, "click long.\r\n");
            break;  
            case 2:
              chprintf(dbg, "click once.\r\n");
            break;  
            case 3:
              chprintf(dbg, "click double.\r\n");
            break;  
            //case 4:
            //  chprintf(dbg, "click immediately.\r\n");
            //break;  
            default:
              chprintf(dbg, "unknown message for %d!\r\n", dbg_src);
            break;  
          }
        break;
        default:
          //chprintf(dbg, "unknown message source: %d!\r\n", dbg_src);
        break;
      }
    }
    chThdSleepMilliseconds(50);
  }
}

/*===========================================================================*/
/* Button related code.                                                      */
/*===========================================================================*/

/* Function prototypes needed as the two callbacks call each other.
   there is no way to order the callback without triggering an error. */
static void button_cb(void *arg);
static void vt_cb(virtual_timer_t *vtp, void *p);
static uint8_t btn_second_edge = 0, btn_cnt = 0;

/* Virtual timer. */
static virtual_timer_t vt, vt2;

/* Callback of the virtual timer. */
static void vt_cb(virtual_timer_t *vtp, void *p) { // Timer cb after 50ms (single click)
  (void)vtp;
  (void)p;
  chSysLockFromISR();
  /* Enabling the event and associating the callback. */
  if (btn_second_edge) // The first edge is a negative one, the second a rising one
    palEnableLineEventI(EXTBTN, PAL_EVENT_MODE_RISING_EDGE); // Prepare for release of button
  else
    palEnableLineEventI(EXTBTN, PAL_EVENT_MODE_FALLING_EDGE);
  palSetLineCallbackI(EXTBTN, button_cb, NULL);
  chSysUnlockFromISR();
}

static void vt2_cb(virtual_timer_t *vtp, void *p) { // Timer cb after 200ms (double click)
  (void)vtp;
  (void)p;
  chSysLockFromISR();
  switch (btn_cnt){
  case 1:
    if (btn_second_edge){ // only for the first edge
      chMBPostI(&mb_dbg, (msg_t) (uint32_t)((1<<24) | 1));
    }
    break;
  case 2:
    chMBPostI(&mb_dbg, (msg_t) (uint32_t)((1<<24) | 2));
    break;
  default:
    chMBPostI(&mb_dbg, (msg_t) (uint32_t)((1<<24) | 3));
    break;
  }

  btn_cnt = 0;
  chSysUnlockFromISR();
}
/* Callback associated to the falling or rising edge of the button line. */
static void button_cb(void *arg) {
  (void)arg;
  //palToggleLine(LED);
  if (btn_second_edge){    // next edge
    btn_second_edge = 0;
  }
  else{                  // very first negative going edge or Button released
    btn_second_edge = 1;
    //chMBPostI(&mb_dbg, (msg_t) (uint32_t)((1<<24) | 4)); // crashes here!!
  }
  btn_cnt++;           // count edges
  chSysLockFromISR();
  /* Disabling the event on the line and setting a timer to
     re-enable it. */
  palDisableLineEventI(EXTBTN);
  /* Arming the VT timer to re-enable the event in 50ms. */
  chVTResetI(&vt);
  chVTDoSetI(&vt, TIME_MS2I(50), vt_cb, NULL);
  chVTResetI(&vt2);
  chVTDoSetI(&vt2, TIME_MS2I(200), vt2_cb, NULL);
  chSysUnlockFromISR();
}

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

char history_buffer[8*64];
char *completion_buffer[SHELL_MAX_COMPLETIONS];

static const ShellCommand commands[] = {
  {"test",cmd_test},
  {"++help",cmd_help},
  {"++addr",cmd_addr},
  {"++auto",cmd_auto},
  {"++echo",cmd_localecho},
  {"++debug",cmd_debug},
  {"++default",cmd_default},
  {"++eoi",cmd_eoi},
  {"++eos",cmd_eos},
  {"++eot",cmd_eot},
  {"++mode",cmd_mode},
  {"++eot_enable",cmd_eot_en},
  {"++srq",cmd_srq},
  {"++trg",cmd_trg},
  {"++spoll",cmd_spoll},
  {"++ifc",cmd_ifc},
  {"++llo",cmd_llo},
  {"++loc",cmd_loc},
  {"++clr",cmd_clr},
  {"++ver",cmd_ver},
  {".",cmd_gpib_cmd}, // last function gets called whenever all others are no match, needs '.' as name
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SHELLPORT,
  commands,
  history_buffer,
  sizeof(history_buffer),
  completion_buffer
};


/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    systime_t time;
    time = serusbcfg1.usbp->state == USB_ACTIVE ? 250 : 500;
    palClearLine(LED);
    chThdSleepMilliseconds(time);
    palSetLine(LED);
    chThdSleepMilliseconds(time);

    //chprintf(dbg, "LastWidth: %d \r\n", last_width1);
    //chprintf(dbg, "LastPeriod: %d \r\n", last_period1);
  }
}



/*
 * Application entry point.
 */
int main(void) {
  thread_t *shelltp = NULL;
  event_listener_t shell_el;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  palSetLineMode(LED, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(DEBUG1, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(DEBUG2, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(EXTBTN, PAL_MODE_INPUT_PULLUP); // Button
  /* Enabling the event and associating the callback. */
  palEnableLineEvent(EXTBTN, PAL_EVENT_MODE_FALLING_EDGE);
  palSetLineCallback(EXTBTN, button_cb, NULL);

  sduObjectInit(&SHELLPORT);
  sduStart(&SHELLPORT, &serusbcfg1);

  usbDisconnectBus(serusbcfg1.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg1.usbp, &usbcfg);
  usbConnectBus(serusbcfg1.usbp);

  //chprintf(dbg, "\r\nSerial Sniffer Programmer: %i.%i \r\nSystem started. (Shell)\r\n", VMAJOR, VMINOR);
  //palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(7));  // TX2
  //palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(7));  // RX2
  //palSetPadMode(GPIOA, 9, PAL_MODE_ALTERNATE(7));  // TX1
  //palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(7)); // RX1

  /*
   * Shell manager initialization.
   * Event zero is shell exit.
   */
  shellInit();
  chEvtRegister(&shell_terminated, &shell_el, 0);
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
  init_GPIB();
  chThdCreateStatic(waGPIB_Thread, sizeof(waGPIB_Thread), NORMALPRIO, GPIB_Thread, NULL);
  chThdCreateStatic(waDebugThread1, sizeof(waDebugThread1), NORMALPRIO, DebugThread1, NULL);
  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
#if USB_SHELL == 1
    if (SHELLPORT.config->usbp->state == USB_ACTIVE) {
      /* Starting shells.*/
      if (shelltp == NULL) {
        shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                       "shell1", NORMALPRIO + 1,
                                       shellThread, (void *)&shell_cfg1);
      }
    //chThdWait(shelltp);               /* Waiting termination.             */
    chEvtWaitAny(EVENT_MASK(0));
    if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);
      shelltp = NULL;
    }
#else
    if (!shelltp)
      shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                    "shell", NORMALPRIO + 1,
                                    shellThread, (void *)&shell_cfg1);
    else if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
      shelltp = NULL;           /* Triggers spawning of a new shell.        */
    }
#endif
    /* Waiting for an exit event then freeing terminated shells.*/
    }
    else{
      chThdSleepMilliseconds(1000);
    }
  }
}

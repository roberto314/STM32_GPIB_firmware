/*
 * comm.h
 *
 *  Created on: 04.02.2018
 *      Author: Anwender
 */

#ifndef USERLIB_INCLUDE_COMM_H_
#define USERLIB_INCLUDE_COMM_H_

#include "main.h"
//#include "myStringfunctions.h"
#define cli_println(a); chprintf((BaseSequentialStream *)&DEBUGPORT, a"\r\n");
#define OK(); do{cli_println(" ... OK"); chThdSleepMilliseconds(20);}while(0)

void cmd_gpib_cmd(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_help(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_addr(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_auto(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_localecho(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_default(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_eoi(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_eos(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_eot(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_eot_en(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_mode(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_srq(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_trg(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_spoll(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_ifc(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_llo(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_loc(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_clr(BaseSequentialStream *chp, int argc, char *argv[]);

void cmd_ver(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]);
void cmd_debug(BaseSequentialStream *chp, int argc, char *argv[]);

#endif /* USERLIB_INCLUDE_COMM_H_ */

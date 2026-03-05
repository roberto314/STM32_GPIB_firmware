/*
 * gpib.h
 *
 *  Created on: Mar 5, 2026
 *      Author: rob
 */

#include "ch.h"
#include "hal.h"

#ifndef USERLIB_INCLUDE_GPIB_H_
#define USERLIB_INCLUDE_GPIB_H_

THD_FUNCTION(GPIB_Thread, arg);

void init_GPIB(void);

#endif /* USERLIB_INCLUDE_GPIB_H_ */

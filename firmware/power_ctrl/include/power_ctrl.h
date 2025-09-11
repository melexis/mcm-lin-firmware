/**
 * @file
 * @brief The power control definitions.
 * @internal
 *
 * @copyright (C) 2025 Melexis N.V.
 *
 * Melexis N.V. is supplying this code for use with Melexis N.V. processor based microcontrollers only.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY,
 * INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.  MELEXIS N.V. SHALL NOT IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * @endinternal
 *
 * @ingroup application
 *
 * @details This file contains the definitions of the power control module.
 */

#ifndef POWER_CTRL_H_
    #define POWER_CTRL_H_

#include <stdint.h>
#include <stdbool.h>

/** initialize the slave power control module */
void powerctrl_init(void);

/** enable power to the slave module */
void powerctrl_slaveEnable(void);

/** disable power to the slave module */
void powerctrl_slaveDisable(void);

/** check if the power to the slave module is enabled
 *
 * @retval  true  power is enabled.
 * @retval  false  power is disabled.
 */
bool powerctrl_slaveEnabled(void);

/** */
int32_t powerctrl_getOutputCurrent(void);

int32_t powerctrl_getSupplyVoltage(void);
int32_t powerctrl_getBusVoltage(void);

#endif  /* POWER_CTRL_H_ */

/*
 * adua7118Driver.h
 *
 *  Created on: Aug 30, 2026
 *      Author: 310004
 */

#ifndef SDS_ADUA7118_ADUA7118DRIVER_H_
#define SDS_ADUA7118_ADUA7118DRIVER_H_

#include "stm32f7xx_hal.h"

#define ADAU7118_I2C_ADDR   (0x4B << 1)

void ADAU7118_Initxxx(I2C_HandleTypeDef *hi2c);


#endif /* SDS_ADUA7118_ADUA7118DRIVER_H_ */

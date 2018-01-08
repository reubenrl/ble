/***************************************************************************//**
 * @file init_board.c
 *******************************************************************************
 * # License
 * <b>Copyright 2017 Silicon Labs, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#include "hal-config.h"
#else
#include "bspconfig.h"
#endif

#include "board_features.h"

#include "em_cmu.h"

#include "em_usart.h"
#include "mx25flash_spi.h"

#include "bsp.h"

void initBoard(void)
{
  // Enable GPIO clock source
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Put the SPI flash into Deep Power Down mode for those radio boards where it is available
  MX25_init();
  MX25_DP();
  // We must disable SPI communication
  USART_Reset(MX25_USART);
}
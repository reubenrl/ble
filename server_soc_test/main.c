/***********************************************************************************************//**
 * \file   main.c
 * \brief  Silicon Labs Empty Example Project
 *
 * This example demonstrates the bare minimum needed for a Blue Gecko C application
 * that allows Over-the-Air Device Firmware Upgrading (OTA DFU). The application
 * starts advertising after boot and restarts advertising after a connection is closed.
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

/* Board headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "boards.h"
#include "ble-configuration.h"
#include "board_features.h"

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include "aat.h"

/* Libraries containing default Gecko configuration values */
#include "em_emu.h"
#include "em_cmu.h"

/* Device initialization header */
#include "hal-config.h"

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif


#include <stdio.h>
#include "retargetserial.h"
#include "sleep.h"

/***********************************************************************************************//**
 * @addtogroup Application
 * @{
 **************************************************************************************************/

/***********************************************************************************************//**
 * @addtogroup app
 * @{
 **************************************************************************************************/

#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 4
#endif
uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];

// Gecko configuration parameters (see gecko_configuration.h)
static const gecko_configuration_t config = {
  .config_flags = 0,
  .sleep.flags = SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
  .bluetooth.sleep_clock_accuracy = 100, // ppm
  .gattdb = &bg_gattdb_data,
  .ota.flags = 0,
  .ota.device_name_len = 3,
  .ota.device_name_ptr = "OTA",
#if (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
  .pa.config_enable = 1, // Enable high power PA
  .pa.input = GECKO_RADIO_PA_INPUT_VBAT, // Configure PA input to VBAT
#endif // (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
};

// Flag for indicating DFU Reset must be performed
uint8_t boot_to_dfu = 0;

void spp_client_main(void);	/* added */
void spp_server_main(void);	/* added */


/**
 * @brief  Main function
 */
void main(void)
{
  // Initialize device
  initMcu();
  // Initialize board
  initBoard();
  // Initialize application
  initApp();

  RETARGET_SerialInit();	/* added */

  printf("\r\nHello.....\r\n");
  // Initialize stack
  gecko_init(&config);

  /* select SPP client or server role based on PB0/PB1 status.
    * default (PBx released) -> server
    *   PBx pressed          -> client
    * */
   GPIO_PinModeSet(BSP_BUTTON0_PORT,BSP_BUTTON0_PIN, gpioModeInput, 1);	/* added */
   GPIO_PinModeSet(BSP_BUTTON1_PORT,BSP_BUTTON1_PIN, gpioModeInput, 1);	/* added */

   /* keeping either PB0 or PB1 pressed during reboot selects 'client mode' */
   if((GPIO_PinInGet(BSP_BUTTON0_PORT,BSP_BUTTON0_PIN) == 0) || (GPIO_PinInGet(BSP_BUTTON1_PORT,BSP_BUTTON1_PIN) == 0))	/* added */
   {	/* added */
 	  printf("* SPP client mode *\r\n");
 	  spp_client_main();
   }
   else
   {
 	  printf("* SPP server mode *\r\n");
 	  spp_server_main();
   }


}

/** @} (end addtogroup app) */
/** @} (end addtogroup Application) */

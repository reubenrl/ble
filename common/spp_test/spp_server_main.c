/***********************************************************************************************//**
 * \file   spp_server_main.c
 * \brief  SPP server example
 *
 *
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

/* Board headers */
#include "boards.h"
#include "ble-configuration.h"
#include "board_features.h"

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

/* Libraries containing default Gecko configuration values */

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

/***************************************************************************************************
  Local Macros and Definitions
 **************************************************************************************************/

#define SPP_TX_TIMER  2

#define STATE_ADVERTISING 1
#define STATE_CONNECTED   2
#define STATE_SPP_MODE    3


/***************************************************************************************************
 Local Variables
 **************************************************************************************************/
static uint8 _conn_handle = 0xFF;
static int _main_state;


static void reset_variables()
{
	_conn_handle = 0xFF;
	_main_state = STATE_ADVERTISING;
}


/* this is called periodically when SPP data mode is active */
static void send_spp_data()
{
	uint8 len = 0;
	uint8 data[20];
	uint16 result;

	int c;

	// read up to 20 characters from local buffer
	while(len < 20)
	{
		  c = RETARGET_ReadChar();

		  if(c < 0)
		  {
			  break;
		  }
		  else
		  {
			  data[len++] = (uint8)c;
		  }
	}

	if(len > 0)
	{
		// stack may return "out-of-memory" error if the local buffer is full -> in that case, just keep trying until the command succeeds
		do
		{
			result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, len, data)->result;
		}
		while(result == bg_err_out_of_memory);
	}
}

/**
 * @brief  SPP server mode main loop
 */
void spp_server_main(void)
{

	char printbuf[128];

  while (1) {
    /* Event pointer for handling events */
    struct gecko_cmd_packet* evt;
    
    /* Check for stack event. */
    evt = gecko_wait_event();

    /* Handle events */
    switch (BGLIB_MSG_ID(evt->header)) {

      /* This boot event is generated when the system boots up after reset.
       * Here the system is set to start advertising immediately after boot procedure. */
    case gecko_evt_system_boot_id:

    	reset_variables();

    	gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
    	break;


    	/* Connection opened event */
    case gecko_evt_le_connection_opened_id:

    	_conn_handle = evt->data.evt_le_connection_opened.connection;
    	printf("connected");
    	_main_state = STATE_CONNECTED;

    	/* request connection parameter update.
    	 * conn.interval min 20ms, max 40ms, slave latency 4 intervals,
    	 * supervision timeout 2 seconds
    	 * (These should be compliant with Apple Bluetooth Accessory Design Guidelines) */
    	gecko_cmd_le_connection_set_parameters(_conn_handle, 16, 32, 4, 200);
    	break;

    case gecko_evt_le_connection_parameters_id:
    	printf("Conn.parameters: interval %u units, txsize %u\r\n",
    	evt->data.evt_le_connection_parameters.interval,
		evt->data.evt_le_connection_parameters.txsize);
    	break;

    case gecko_evt_le_connection_closed_id:
    	printf("DISCONNECTED!");

    	reset_variables();
    	SLEEP_SleepBlockEnd(sleepEM2); // enable sleeping

    	// stop TX timer
    	gecko_cmd_hardware_set_soft_timer(0, SPP_TX_TIMER, 0);

    	gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
    	break;

    case gecko_evt_gatt_server_characteristic_status_id:
    {
    	struct gecko_msg_gatt_server_characteristic_status_evt_t *pStatus;
    	pStatus = &(evt->data.evt_gatt_server_characteristic_status);

    	if(pStatus->characteristic == gattdb_gatt_spp_data)
    	{
    		if(pStatus->status_flags == gatt_server_client_config)
    		{
    			// Characteristic client configuration (CCC) for spp_data has been changed
    			if(pStatus->client_config_flags == gatt_notification)
    			{
    				printf("SPP mode ON\r\n");
    				// start TX timer
    				gecko_cmd_hardware_set_soft_timer(328, SPP_TX_TIMER, 0);
    				SLEEP_SleepBlockBegin(sleepEM2); // disable sleeping
    			}
    			else
    			{
    				printf("SPP mode OFF\r\n");
    				// stop TX timer
    				gecko_cmd_hardware_set_soft_timer(0, SPP_TX_TIMER, 0);
    				SLEEP_SleepBlockEnd(sleepEM2); // enable sleeping
    			}

    		}
    	}
    }
    break;

    case gecko_evt_gatt_server_attribute_value_id:
    {
    	 // data received from SPP client -> print to UART
    	 // NOTE: this works only with text (no binary) because printf() expects null-terminated strings as input
    	 memcpy(printbuf, evt->data.evt_gatt_server_attribute_value.value.data, evt->data.evt_gatt_server_attribute_value.value.len);
    	 printbuf[evt->data.evt_gatt_server_attribute_value.value.len] = 0;
    	 printf(printbuf);
    }
    break;

    	/* Software Timer event */
    case gecko_evt_hardware_soft_timer_id:

    	switch (evt->data.evt_hardware_soft_timer.handle) {

    	case SPP_TX_TIMER:
    		// send data from local TX buffer
    		send_spp_data();
    		break;


    	default:
    		break;
    	}
    	break;

    	break;

      default:
        break;
    }
  }
}


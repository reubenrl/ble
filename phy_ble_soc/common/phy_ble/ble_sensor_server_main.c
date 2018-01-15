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

#include "infrastructure.h"

/* Libraries containing default Gecko configuration values */

/* Device initialization header */
#include "hal-config.h"

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif

#include "bsp.h"

#include <stdio.h>
#include "retargetserial.h"
#include "sleep.h"


#include "i2cspm.h"
#include "si7013.h"
#include "tempsens.h"

/***************************************************************************************************
  Local Macros and Definitions
 **************************************************************************************************/

#define TEMPERATURE_TIMER	2


#define STATE_ADVERTISING 1
#define STATE_CONNECTED   2
#define STATE_SPP_MODE    3


/***************************************************************************************************
 Local Variables
 **************************************************************************************************/
static uint8 _conn_handle = 0xFF;
static int _main_state;

static volatile uint32 delay_sec_timer = 2;

static void reset_variables()
{
	_conn_handle = 0xFF;
	_main_state = STATE_ADVERTISING;
}


/* this is called periodically when SPP data mode is active */
/*
static void send_spp_data()
{
	uint8 len = 0;
	uint8 data[20];
	uint16 result;

	int c;

	// read up to 20 characters from local buffer
	while(len < 20)	{
		  c = RETARGET_ReadChar();

		  if(c < 0){
			  break;
		  }else{
			  data[len++] = (uint8)c;
		  }
	}

	if(len > 0){
		// stack may return "out-of-memory" error if the local buffer is full -> in that case, just keep trying until the command succeeds
		do{
			result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, len, data)->result;
		}while(result == bg_err_out_of_memory);
	}
}
*/

void temperatureMeasure()
{
  uint8_t htmTempBuffer[5]; 	/* Stores the temperature data in the Health Thermometer (HTM) format. */
  uint8_t flags = 0x00;   		/* HTM flags set as 0 for Celsius, no time stamp and no temperature type. */
  int32_t tempData;     		/* Stores the Temperature data read from the RHT sensor. */
  uint32_t rhData = 0;    		/* Dummy needed for storing Relative Humidity data. */
  uint32_t temperature;   		/* Stores the temperature data read from the sensor in the correct format */
  uint8_t *p = htmTempBuffer; 	/* Pointer to HTM temperature buffer needed for converting values to bitstream. */

  /* Convert flags to bitstream and append them in the HTM temperature data buffer (htmTempBuffer) */
  UINT8_TO_BITSTREAM(p, flags);

  /* Sensor relative humidity and temperature measurement returns 0 on success, nonzero otherwise */
  if (Si7013_MeasureRHAndTemp(I2C0, SI7021_ADDR, &rhData, &tempData) == 0) {
	  printf("\r\nrhData%d%%: temperature:%d\r\n", rhData/1000U, tempData/1000U);


    /* Convert sensor data to correct temperature format */
    //temperature = FLT_TO_UINT32(tempData, -3);
    /* Convert temperature to bitstream and place it in the HTM temperature data buffer (htmTempBuffer) */
    UINT32_TO_BITSTREAM(p, tempData);


    /* Send indication of the temperature in htmTempBuffer to all "listening" clients.
     * This enables the Health Thermometer in the Blue Gecko app to display the temperature.
     *  0xFF as connection ID will send indications to all connections. */
    gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_phy_ble_data, 5, htmTempBuffer);
  }
}


/**
 * @brief  SPP server mode main loop
 */
void ble_sensor_server_main(void)
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
    	printf("connected\r\n");
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
    	printf("DISCONNECTED!\r\n");

    	reset_variables();
    	SLEEP_SleepBlockEnd(sleepEM2); // enable sleeping

    	// stop TX timer
    	//gecko_cmd_hardware_set_soft_timer(0, SPP_TX_TIMER, 0);

    	gecko_cmd_hardware_set_soft_timer(0, TEMPERATURE_TIMER, 0);


    	gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

    	break;


    case gecko_evt_gatt_server_characteristic_status_id:
    {
    	struct gecko_msg_gatt_server_characteristic_status_evt_t *pStatus;
    	pStatus = &(evt->data.evt_gatt_server_characteristic_status);

    	if(pStatus->characteristic == gattdb_phy_ble_data){
    		if(pStatus->status_flags == gatt_server_client_config){
    			// Characteristic client configuration (CCC) for spp_data has been changed
    			if(pStatus->client_config_flags == gatt_notification){
    				printf("phy_ble mode ON\r\n");

    				// test
    				// start TX timer:  32768(ticks) = 1 sec => 328 = 10ms
    				gecko_cmd_hardware_set_soft_timer(32768UL*5UL, TEMPERATURE_TIMER, 0); // 5 seconds

					//gecko_cmd_hardware_set_soft_timer(32768, DELAY_SEC_TIMER, 0);	// led 1 sec

    				SLEEP_SleepBlockBegin(sleepEM2); // disable sleeping
    			}else{
    				printf("phy_ble mode OFF\r\n");
    				// test
    				 //gecko_cmd_hardware_set_soft_timer(0, DELAY_SEC_TIMER, 0);

    				// stop TX timer
    				gecko_cmd_hardware_set_soft_timer(0, TEMPERATURE_TIMER, 0);
    				SLEEP_SleepBlockEnd(sleepEM2); // enable sleeping
    			}
    		}
    	}


    	 /* Check that the characteristic in question is temperature - its ID is defined
    	         * in gatt.xml as "temp_measurement". Also check that status_flags = 1, meaning that
    	         * the characteristic client configuration was changed (notifications or indications
    	         * enabled or disabled). */
//    	        if ((pStatus->characteristic == gattdb_temp_measurement)
//    	            && (pStatus->status_flags == 0x01)) {
//    	          if (pStatus->client_config_flags == 0x02) {
//    	            /* Indications have been turned ON - start the repeating timer. The 1st parameter '32768'
//    	             * tells the timer to run for 1 second (32.768 kHz oscillator), the 2nd parameter is
//    	             * the timer handle and the 3rd parameter '0' tells the timer to repeat continuously until
//    	             * stopped manually.*/
//
//    	        	  printf("Health Thermometer mode ON\r\n");
//    	           // gecko_cmd_hardware_set_soft_timer(32768, 0, 0);
//    	            /*
//					 * by me
//					 */
//					gecko_cmd_hardware_set_soft_timer(32768UL*10UL, TEMPERATURE_TIMER, 0); // 10 seconds
//
//					SLEEP_SleepBlockBegin(sleepEM2); // disable sleeping
//
//    	          } else if (pStatus->client_config_flags == 0x00) {
//    	        	  printf("Health Thermometer mode OFF\r\n");
//    	            /* Indications have been turned OFF - stop the timer. */
//    	            //gecko_cmd_hardware_set_soft_timer(0, 0, 0);
//    	            gecko_cmd_hardware_set_soft_timer(0, TEMPERATURE_TIMER, 0);
//
//    	            SLEEP_SleepBlockEnd(sleepEM2); // enable sleeping
//    	          }
//    	        }

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
 /*   	case SPP_TX_TIMER:
    		// send data from local TX buffer
    		send_spp_data();
    		break;

    	case DELAY_SEC_TIMER:
    		if(delay_sec_timer) delay_sec_timer--;
    		break;
*/
    	case TEMPERATURE_TIMER:
    		temperatureMeasure();
    		break;

    	default:
    		break;
    	}// end switch

    	break;

    	break;

      default:
        break;
    }// end switch

    if(!delay_sec_timer){
    	BSP_LedToggle(1);
    	delay_sec_timer = 2;
    }

  }// end while
}


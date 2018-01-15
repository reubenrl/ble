/***********************************************************************************************//**
 * \file   spp_client_main.c
 * \brief  SPP client example
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

//#include "i2cspm.h"
//#include "si7013.h"
//#include "tempsens.h"
/***************************************************************************************************
  Local Macros and Definitions
 **************************************************************************************************/

#define DISCONNECTED	0
#define SCANNING		1
#define FIND_SERVICE	2
#define FIND_CHAR		3
#define ENABLE_NOTIF 	4
#define DATA_MODE		5

// const uint8 serviceUUID[2] = {0x09, 0x18}; // HTM service UUID : 0x1809
// const uint8 charUUID[2] = {0x1C, 0x2A}; // temp.meas UUID : 0x2A1C

// SPP service UUID: 4880c12c-fdcb-4077-8920-a450d7f9b907
//const uint8 serviceUUID[16] = {0x07, 0xb9, 0xf9, 0xd7, 0x50, 0xa4, 0x20, 0x89, 0x77, 0x40, 0xcb, 0xfd, 0x2c, 0xc1, 0x80, 0x48};
// SPP data UUID: fec26ec4-6d71-4442-9f81-55bc21d658d6
//const uint8 charUUID[16] = {0xd6, 0x58, 0xd6, 0x21, 0xbc, 0x55, 0x81, 0x9f, 0x42, 0x44, 0x71, 0x6d, 0xc4, 0x6e, 0xc2, 0xfe};

const uint8 serviceUUID[2] = {0x09, 0x18}; // HTM service UUID : 0x1809
const uint8 charUUID[2] = {0x1C, 0x2A}; // temp.meas UUID : 0x2A1C


#define RESTART_TIMER 1
#define SPP_TX_TIMER  2

/***************************************************************************************************
 Local Variables
 **************************************************************************************************/
static uint8 _conn_handle = 0xFF;
static int _main_state;
static uint32 _service_handle;
static uint16 _char_handle;


static void reset_variables()
{
	_conn_handle = 0xFF;
	_main_state = DISCONNECTED;
	_service_handle = 0;
	_char_handle = 0;
}


static int process_scan_response(struct gecko_msg_le_gap_scan_response_evt_t *pResp)
{
	// decoding advertising packets is done here. The list of AD types can be found
	// at: https://www.bluetooth.com/specifications/assigned-numbers/Generic-Access-Profile

    int i = 0;
    int ad_match_found = 0;
	int ad_len;
    int ad_type;

    char name[32];

    while (i < (pResp->data.len - 1))
    {

        ad_len  = pResp->data.data[i];
        ad_type = pResp->data.data[i+1];

        printf("\r\nad_type:%d\r\n", ad_type);

        if (ad_type == 0x08 || ad_type == 0x09 )
        {
            // type 0x08 = Shortened Local Name
            // type 0x09 = Complete Local Name
            memcpy(name, &(pResp->data.data[i+2]), ad_len-1);
            name[ad_len-1] = 0;
            printf(name);

        }
        /*
        // 4880c12c-fdcb-4077-8920-a450d7f9b907
        if (ad_type == 0x06 || ad_type == 0x07)
        {
        	// type 0x06 = Incomplete List of 128-bit Service Class UUIDs
        	// type 0x07 = Complete List of 128-bit Service Class UUIDs
        	if(memcmp(serviceUUID, &(pResp->data.data[i+2]),16) == 0)
        	{
        		printf("Found SPP device\r\n");
        		ad_match_found = 1;
        	}
        }
*/


        if (ad_type == 0x02 || ad_type == 0x03)
        {
            // type 0x02 = Incomplete List of 16-bit Service Class UUIDs
            // type 0x03 = Complete List of 16-bit Service Class UUIDs

            // note: this check assumes that the service we are looking for is first
            // in the list. To be fixed so that there is no such limitation...
            if(memcmp(serviceUUID, &(pResp->data.data[i+2]),2) == 0)
            {
            	printf("Found Health_Thermometer\r\n");
            	ad_match_found = 1;
            }
        }

        //jump to next AD record
        i = i + ad_len + 1;
    }

    return(ad_match_found);
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
			result = gecko_cmd_gatt_write_characteristic_value_without_response(_conn_handle, _char_handle, len, data)->result;
		}
		while(result == bg_err_out_of_memory);
		if(result != 0)
		{
			printf("WTF: %x\r\n", result);
		}
	}

}
*/
/**
 * @brief  SPP client mode main loop
 */
void spp_client_main(void)
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

    	// start discovery
    	gecko_cmd_le_gap_discover(le_gap_discover_generic);
    	break;

    case gecko_evt_le_gap_scan_response_id:

    	// process scan responses: this function returns 1 if we found the service we are looking for
    	if(process_scan_response(&(evt->data.evt_le_gap_scan_response)) > 0)
    	{
    		struct gecko_msg_le_gap_open_rsp_t *pResp;

    		// match found -> stop discovery and try to connect
    		gecko_cmd_le_gap_end_procedure();

    		pResp = gecko_cmd_le_gap_open(evt->data.evt_le_gap_scan_response.address, evt->data.evt_le_gap_scan_response.address_type);

    		// make copy of connection handle for later use (for example, to cancel the connection attempt)
    		_conn_handle = pResp->connection;

    	}
    	break;

    	/* Connection opened event */
    case gecko_evt_le_connection_opened_id:

    	printf("connected\r\n");

    	//	 start service discovery (we are only interested in one UUID)
    	gecko_cmd_gatt_discover_primary_services_by_uuid(_conn_handle, 2, serviceUUID);
    	_main_state = FIND_SERVICE;

    	break;

    case gecko_evt_le_connection_closed_id:
    	printf("DISCONNECTED!\r\n");

    	reset_variables();
    	// stop TX timer:
    	gecko_cmd_hardware_set_soft_timer(0, SPP_TX_TIMER, 0);

    	SLEEP_SleepBlockEnd(sleepEM2); // enable sleeping after disconnect

    	// create one-shot soft timer that will restart discovery after 1 second delay
    	gecko_cmd_hardware_set_soft_timer(32768*3, RESTART_TIMER, true);
    	break;

    case gecko_evt_le_connection_parameters_id:
       	printf("Conn.parameters: interval %u units, txsize %u\r\n",
       	evt->data.evt_le_connection_parameters.interval,
   		evt->data.evt_le_connection_parameters.txsize);
       	break;

    case gecko_evt_gatt_service_id:

    	if(evt->data.evt_gatt_service.uuid.len == 2)
    	{
    		if(memcmp(serviceUUID, evt->data.evt_gatt_service.uuid.data,2) == 0)
    		{
    			printf("service discovered\r\n");
    			_service_handle = evt->data.evt_gatt_service.service;
    		}
    	}
    	break;

    case gecko_evt_gatt_procedure_completed_id:

    	switch(_main_state)
    	{
    	case FIND_SERVICE:

    		if (_service_handle > 0)
    		{
    			// Service found, next search for characteristics
    			gecko_cmd_gatt_discover_characteristics(_conn_handle, _service_handle);
    			_main_state = FIND_CHAR;
    		}
    		else
    		{
    			// no service found -> disconnect
    			//printf("SPP service not found?\r\n");
    			printf("Health_Thermometer - service not found?\r\n");

    			gecko_cmd_endpoint_close(_conn_handle);
    		}

    		break;

    	case FIND_CHAR:
    		if (_char_handle > 0)
    		{
    			// Char found, turn on indications
    			gecko_cmd_gatt_set_characteristic_notification(_conn_handle, _char_handle, gatt_notification);
    			//gecko_cmd_gatt_set_characteristic_notification(_conn_handle, _char_handle, gatt_indication);
    			_main_state = ENABLE_NOTIF;
    		}
    		else
    		{
    			// no characteristic found? -> disconnect
    			//printf("SPP char not found?\r\n");
    			printf("Health_Thermometer char not found?\r\n");
    			gecko_cmd_endpoint_close(_conn_handle);
    		}
    		break;

    	case ENABLE_NOTIF:
    		_main_state = DATA_MODE;
    		//printf("SPP mode ON\r\n");
    		printf("Health_Thermometer mode ON\r\n");
    		// start soft timer that is used to offload local TX buffer
    		gecko_cmd_hardware_set_soft_timer(32768, SPP_TX_TIMER, 0);
    		SLEEP_SleepBlockBegin(sleepEM2); // disable sleeping when SPP mode active
    		break;

    	default:
    		break;
    	}
    	break;

    	case gecko_evt_gatt_characteristic_id:

    		if(evt->data.evt_gatt_characteristic.uuid.len == 2)
    		{
    			if(memcmp(charUUID, evt->data.evt_gatt_characteristic.uuid.data,2) == 0)
    			{
    				printf("char discovered\r\n");
    				_char_handle = evt->data.evt_gatt_characteristic.characteristic;
    			}
    		}

    		break;

    	case gecko_evt_gatt_characteristic_value_id:

    		if(evt->data.evt_gatt_characteristic_value.characteristic == _char_handle)
    		{
    			// data received from SPP server -> print to UART
    			// NOTE: this works only with text (no binary) because printf() expects null-terminated strings as input
    			memcpy(printbuf, evt->data.evt_gatt_characteristic_value.value.data, evt->data.evt_gatt_characteristic_value.value.len);
    			printbuf[evt->data.evt_gatt_characteristic_value.value.len] = 0;
    			printf(printbuf);
    		}
    		break;

    		/* Software Timer event */
    	case gecko_evt_hardware_soft_timer_id:

    		switch (evt->data.evt_hardware_soft_timer.handle) {

    		case SPP_TX_TIMER:
    			// send data from local TX buffer
    			//send_spp_data();
    			break;

    		case RESTART_TIMER:
    			gecko_cmd_le_gap_discover(le_gap_discover_generic);
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


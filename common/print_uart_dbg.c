/*
 * print_uart_dbg.c
 *
 *  Created on: Jan 7, 2018
 *      Author: phytech
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "print_uart_dbg.h"

// Define receive/transmit operation queues
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_RX_BUFS, rxBufferQueue);
DEFINE_BUF_QUEUE(EMDRV_UARTDRV_MAX_CONCURRENT_TX_BUFS, txBufferQueue);

volatile static bool print_uartInitialized = false;
volatile static bool CRLF_Enable = true;


UARTDRV_HandleData_t uart0_handleData;
UARTDRV_Handle_t uart0_handle = &uart0_handleData;

/*

volatile static bool callTx = false;
volatile uint32_t txCount;

void callback(UARTDRV_Handle_t handle,
              Ecode_t transferStatus,
              uint8_t *data,
              UARTDRV_Count_t transferCount)
{
  (void)handle;
  (void)transferStatus;
  (void)data;
  //(void)transferCount;
  txCount = transferCount;
  callTx = true;

}
*/



void print_uar0tInit(void)
{

	if(print_uartInitialized == true) return;

	print_uartInitialized = true;

	// Initialize driver handle
	 UARTDRV_InitUart_t initData = PRINT_UART_INIT;

	if( UARTDRV_InitUart(uart0_handle, &initData) != ECODE_EMDRV_UARTDRV_OK ){
		while(1);
	}

}


void printDbg(const char* format, ...)
{
	char  msg[100];
	print_uar0tInit();

    va_list    args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg) , format, args); // do check return value
    va_end(args);
    if(CRLF_Enable){
    	if(strcat(msg, "\r\n") == NULL){
    		while(1);
    	}
    }
    // Transmit data using a non-blocking transmit function
   // if( UARTDRV_Transmit(uart0_handle, (uint8_t *)msg, (UARTDRV_Count_t)strlen(msg), callback) != ECODE_EMDRV_UARTDRV_OK){
    if( UARTDRV_TransmitB(uart0_handle, (uint8_t *)msg, (UARTDRV_Count_t)strlen(msg)) != ECODE_EMDRV_UARTDRV_OK){
    	while(1);
    }

}

void printDbgCRLF(void)
{
	CRLF_Enable = false;
	printDbg("\r\n");
	CRLF_Enable = true;
}

void printDbgBuffer( uint8_t *pBuff , uint32_t buffLen)
{
	print_uar0tInit();

	CRLF_Enable = false;

	if( buffLen ){
		//if( UARTDRV_Transmit(uart0_handle, pBuff, buffLen, callback) != ECODE_EMDRV_UARTDRV_OK ){
		if( UARTDRV_TransmitB(uart0_handle, pBuff, buffLen) != ECODE_EMDRV_UARTDRV_OK){
			while(1);
		}
	}

	CRLF_Enable = true;
}

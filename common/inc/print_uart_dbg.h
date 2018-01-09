/*
 * print_uart_dbg.h
 *
 *  Created on: Jan 7, 2018
 *      Author: phytech
 */

#ifndef PRINT_UART_DBG_H
#define PRINT_UART_DBG_H

#include "uartdrv.h"

#define PRINT_UART_INIT {                             		\
		  USART0,                                           \
		  115200,                                           \
		  _USART_ROUTELOC0_TXLOC_DEFAULT,             		\
		  _USART_ROUTELOC0_RXLOC_DEFAULT,            		\
		  usartStopbits1,                                   \
		  usartNoParity,                                    \
		  usartOVS16,                                       \
		  false,                                            \
		  uartdrvFlowControlNone,               			\
		  gpioPortA,                                        \
		  2,                                                \
		  gpioPortA,                                        \
		  3,                                                \
		  (UARTDRV_Buffer_FifoQueue_t *)&rxBufferQueue,     \
		  (UARTDRV_Buffer_FifoQueue_t *)&txBufferQueue,     \
		  _USART_ROUTELOC1_CTSLOC_LOC28,           			\
		  _USART_ROUTELOC1_RTSLOC_LOC28          			\
		};

void printDbg(const char* format, ...);
void printDbgCRLF(void);
void printDbgBuffer( uint8_t *pBuff , uint32_t buffLen);


#endif /* PRINT_UART_DBG_H */

/*
    FreeRTOS V8.2.0rc1 - Copyright (C) 2014 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?".  Have you defined configASSERT()?  *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    ***************************************************************************
     *                                                                       *
     *   Investing in training allows your team to be as productive as       *
     *   possible as early as possible, lowering your overall development    *
     *   cost, and enabling you to bring a more robust product to market     *
     *   earlier than would otherwise be possible.  Richard Barry is both    *
     *   the architect and key author of FreeRTOS, and so also the world's   *
     *   leading authority on what is the world's most popular real time     *
     *   kernel for deeply embedded MCU designs.  Obtaining your training    *
     *   from Richard ensures your team will gain directly from his in-depth *
     *   product knowledge and years of usage experience.  Contact Real Time *
     *   Engineers Ltd to enquire about the FreeRTOS Masterclass, presented  *
     *   by Richard Barry:  http://www.FreeRTOS.org/contact
     *                                                                       *
    ***************************************************************************

    ***************************************************************************
     *                                                                       *
     *    You are receiving this top quality software for free.  Please play *
     *    fair and reciprocate by reporting any suspected issues and         *
     *    participating in the community forum:                              *
     *    http://www.FreeRTOS.org/support                                    *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * Very simple task that responds with a single WEB page to http requests.
 *
 * The WEB page displays task and system status.  A semaphore is used to
 * wake the task when there is processing to perform as determined by the
 * interrupts generated by the Ethernet interface.
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Tern includes. */
#include "utils\system_common.h"
#include "i2chip_hw.h"
#include "socket.h"

/* FreeRTOS.org includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* The standard http port on which we are going to listen. */
#define httpPORT 80

#define httpTX_WAIT 2

/* Network address configuration. */
const unsigned char ucMacAddress[] =			{ 12, 128, 12, 34, 56, 78 };
const unsigned char ucGatewayAddress[] =		{ 192, 168, 2, 1 };
const unsigned char ucIPAddress[] =				{ 172, 25, 218, 210 };
const unsigned char ucSubnetMask[] =			{ 255, 255, 255, 0 };

/* The number of sockets this task is going to handle. */
#define httpSOCKET_NUM                       3
unsigned char ucConnection[ httpSOCKET_NUM ];

/* The maximum data buffer size we can handle. */
#define httpSOCKET_BUFFER_SIZE	2048

/* Standard HTTP response. */
#define httpOUTPUT_OK	"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"

/* Hard coded HTML components.  Other data is generated dynamically. */
#define HTML_OUTPUT_BEGIN "\
<HTML><head><meta http-equiv=\"Refresh\" content=\"1\;url=index.htm\"></head>\
<BODY bgcolor=\"#CCCCFF\"><font face=\"arial\"><H2>FreeRTOS.org<sup>tm</sup> + Tern E-Engine<sup>tm</sup></H2>\
<a href=\"http:\/\/www.FreeRTOS.org\">FreeRTOS.org Homepage</a><P>\
<HR>Task status table:\r\n\
<p><font face=\"courier\"><pre>Task          State  Priority  Stack	#<br>\
************************************************<br>"

#define HTML_OUTPUT_END   "\
</font></BODY></HTML>"

/*-----------------------------------------------------------*/

/*
 * Initialise the data structures used to hold the socket status.
 */
static void prvHTTPInit( void );

/*
 * Setup the Ethernet interface with the network addressing information.
 */
static void prvNetifInit( void );

/*
 * Generate the dynamic components of the served WEB page and transmit the
 * entire page through the socket.
 */
static void prvTransmitHTTP( unsigned char socket );
/*-----------------------------------------------------------*/

/* This variable is simply incremented by the idle task hook so the number of
iterations the idle task has performed can be displayed as part of the served
page. */
unsigned long ulIdleLoops = 0UL;

/* Data buffer shared by sockets. */
unsigned char ucSocketBuffer[ httpSOCKET_BUFFER_SIZE ];

/* The semaphore used by the Ethernet ISR to signal that the task should wake
and process whatever caused the interrupt. */
SemaphoreHandle_t xTCPSemaphore = NULL;

/*-----------------------------------------------------------*/
void vHTTPTask( void * pvParameters )
{
short i, sLen;
unsigned char ucState;

	( void ) pvParameters;

    /* Create the semaphore used to communicate between this task and the
    WIZnet ISR. */
    vSemaphoreCreateBinary( xTCPSemaphore );

	/* Make sure everything is setup before we start. */
	prvNetifInit();
	prvHTTPInit();

	for( ;; )
	{
		/* Wait until the ISR tells us there is something to do. */
    	xSemaphoreTake( xTCPSemaphore, portMAX_DELAY );

		/* Check each socket. */
		for( i = 0; i < httpSOCKET_NUM; i++ )
		{
			ucState = select( i, SEL_CONTROL );

			switch (ucState)
			{
				case SOCK_ESTABLISHED :  /* new connection established. */

					if( ( sLen = select( i, SEL_RECV ) ) > 0 )
					{
						if( sLen > httpSOCKET_BUFFER_SIZE )
						{
							sLen = httpSOCKET_BUFFER_SIZE;
						}

						disable();

						sLen = recv( i, ucSocketBuffer, sLen );

						if( ucConnection[ i ] == 1 )
						{
							/* This is our first time processing a HTTP
							 request on this connection. */
							prvTransmitHTTP( i );
							ucConnection[i] = 0;
						}
						enable();
					}
					break;

				case SOCK_CLOSE_WAIT :

					close(i);
					break;

				case SOCK_CLOSED :

					ucConnection[i] = 1;
					socket( i, SOCK_STREAM, 80, 0x00 );
					NBlisten( i ); /* reinitialize socket. */
					break;
			}
		}
	}
}
/*-----------------------------------------------------------*/

static void prvHTTPInit( void )
{
unsigned char ucIndex;

	/* There are 4 total sockets available; we will claim 3 for HTTP. */
	for(ucIndex = 0; ucIndex < httpSOCKET_NUM; ucIndex++)
	{
		socket( ucIndex, SOCK_STREAM, httpPORT, 0x00 );
		NBlisten( ucIndex );
		ucConnection[ ucIndex ] = 1;
	}
}
/*-----------------------------------------------------------*/

static void prvNetifInit( void )
{
	i2chip_init();
	initW3100A();

	setMACAddr( ( unsigned char * ) ucMacAddress );
	setgateway( ( unsigned char * ) ucGatewayAddress );
	setsubmask( ( unsigned char * ) ucSubnetMask );
	setIP( ( unsigned char * ) ucIPAddress );

	/* See definition of 'sysinit' in socket.c
	 - 8 KB transmit buffer, and 8 KB receive buffer available.  These buffers
	   are shared by all 4 channels.
	 - (0x55, 0x55) configures the send and receive buffers at
		httpSOCKET_BUFFER_SIZE bytes for each of the 4 channels. */
	sysinit( 0x55, 0x55 );
}
/*-----------------------------------------------------------*/

static void prvTransmitHTTP(unsigned char socket)
{
extern short usCheckStatus;

	/* Send the http and html headers. */
	send( socket, ( unsigned char * ) httpOUTPUT_OK, strlen( httpOUTPUT_OK ) );
	send( socket, ( unsigned char * ) HTML_OUTPUT_BEGIN, strlen( HTML_OUTPUT_BEGIN ) );

	/* Generate then send the table showing the status of each task. */
	vTaskList( ( char * ) ucSocketBuffer );
 	send( socket, ( unsigned char * ) ucSocketBuffer, strlen( ucSocketBuffer ) );

	/* Send the number of times the idle task has looped. */
    sprintf( ucSocketBuffer, "</pre></font><p><br>The idle task has looped 0x%08lx times<br>", ulIdleLoops );
	send( socket, ( unsigned char * ) ucSocketBuffer, strlen( ucSocketBuffer ) );

	/* Send the tick count. */
    sprintf( ucSocketBuffer, "The tick count is 0x%08lx<br>", xTaskGetTickCount() );
	send( socket, ( unsigned char * ) ucSocketBuffer, strlen( ucSocketBuffer ) );

	/* Show a message indicating whether or not the check task has discovered
	an error in any of the standard demo tasks. */
    if( usCheckStatus == 0 )
    {
	    sprintf( ucSocketBuffer, "No errors detected." );
		send( socket, ( unsigned char * ) ucSocketBuffer, strlen( ucSocketBuffer ) );
    }
    else
    {
	    sprintf( ucSocketBuffer, "<font color=\"red\">An error has been detected in at least one task %x.</font><p>", usCheckStatus );
		send( socket, ( unsigned char * ) ucSocketBuffer, strlen( ucSocketBuffer ) );
    }

	/* Finish the page off. */
	send( socket, (unsigned char*)HTML_OUTPUT_END, strlen(HTML_OUTPUT_END));

	/* Must make sure the data is gone before closing the socket. */
	while( !tx_empty( socket ) )
    {
    	vTaskDelay( httpTX_WAIT );
    }
	close(socket);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	ulIdleLoops++;
}





















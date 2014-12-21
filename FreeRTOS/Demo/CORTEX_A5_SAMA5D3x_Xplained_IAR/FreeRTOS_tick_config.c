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

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "Task.h"

/* Library includes. */
#include "board.h"

/*
 * The FreeRTOS tick handler.  This function must be installed as the handler
 * for the timer used to generate the tick interrupt.  Note that the interrupt
 * generated by the PIT is shared by other system peripherals, so if the PIT is
 * used for Tick generation then FreeRTOS_Tick_Handler() can only be installed
 * directly as the PIT handler if no other system interrupts need to be
 * serviced.  If system interrupts other than the PIT need to be serviced then
 * install System_Handler() as the PIT interrupt handler in place of
 * FreeRTOS_Tick_Handler() and add additional interrupt processing into the
 * implementation of System_Handler().
 */
extern void FreeRTOS_Tick_Handler( void );
static void System_Handler( void );

/*-----------------------------------------------------------*/

static void System_Handler( void )
{
	/* See the comments above the function prototype in this file. */
	FreeRTOS_Tick_Handler();
}
/*-----------------------------------------------------------*/

/*
 * The application must provide a function that configures a peripheral to
 * create the FreeRTOS tick interrupt, then define configSETUP_TICK_INTERRUPT()
 * in FreeRTOSConfig.h to call the function.  This file contains a function
 * that is suitable for use on the Atmel SAMA5.
 */
void vConfigureTickInterrupt( void )
{
	/* NOTE:  The PIT interrupt is cleared by the configCLEAR_TICK_INTERRUPT()
	macro in FreeRTOSConfig.h. */

	/* Enable the PIT clock. */
	PMC->PMC_PCER0 = 1 << ID_PIT;

	/* Initialize the PIT to the desired frequency - specified in uS. */
	PIT_Init( 1000000UL / configTICK_RATE_HZ, BOARD_MCK / 1000000 );

	/* Configure interrupt on PIT.  Note this is on the system interrupt, which
	is shared with other system peripherals, so System_Handler() must be
	installed in place of FreeRTOS_Tick_Handler() if other system handlers are
	required.  The tick must be given the lowest priority (0 in the SAMA5 AIC) */
	IRQ_ConfigureIT( ID_PIT, AIC_SMR_SRCTYPE_EXT_POSITIVE_EDGE, FreeRTOS_Tick_Handler );
	/* See commend directly above IRQ_ConfigureIT( ID_PIT, 0, System_Handler ); */
	IRQ_EnableIT( ID_PIT );
	PIT_EnableIT();

	/* Enable the pit. */
	PIT_Enable();

	/* Prevent compiler warnings in the case where System_Handler() is not used
	as the handler.  See the comments above the System_Handler() function
	prototype at the top of this file. */
	( void ) System_Handler;
}
/*-----------------------------------------------------------*/


/**
 * \file
 *
 * \brief TC16 related functionality implementation.
 *
 * Copyright (C) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include <tc16.h>
#include <utils.h>

/**
 * \brief Initialize TIMER_0 interface
 */
int8_t TIMER_0_init()
{

	/* Enable TC1 */
	PRR0 &= ~(1 << PRTIM1);

	// TCCR1A = (0 << COM1A1) | (0 << COM1A0) /* Normal port operation, OCA disconnected */
	//		 | (0 << COM1B1) | (0 << COM1B0) /* Normal port operation, OCB disconnected */
	//		 | (0 << WGM11) | (0 << WGM10); /* TC16 Mode 0 Normal */

	TCCR1B = (0 << WGM13) | (0 << WGM12)                /* TC16 Mode 0 Normal */
	         | 0 << ICNC1                               /* Input Capture Noise Canceler: disabled */
	         | 0 << ICES1                               /* Input Capture Edge Select: disabled */
	         | (0 << CS12) | (0 << CS11) | (1 << CS10); /* No prescaling */

	// ICR1 = 0; /* Input capture value, used as top counter value in some modes: 0 */

	OCR1A = 16000; /* Output compare A: 16000 */

	OCR1B = 16000; /* Output compare B: 16000 */

	TIMSK1 = 0 << OCIE1B   /* Output Compare B Match Interrupt Enable: disabled */
	         | 1 << OCIE1A /* Output Compare A Match Interrupt Enable: enabled */
	         | 0 << ICIE1  /* Input Capture Interrupt Enable: disabled */
	         | 0 << TOIE1; /* Overflow Interrupt Enable: disabled */

	return 0;
}

/**
 * \brief Initialize TIMER_1 interface
 */
int8_t TIMER_1_init()
{

	/* Enable TC3 */
	PRR1 &= ~(1 << PRTIM3);

	// TCCR3A = (0 << COM3A1) | (0 << COM3A0) /* Normal port operation, OCA disconnected */
	//		 | (0 << COM3B1) | (0 << COM3B0) /* Normal port operation, OCB disconnected */
	//		 | (0 << WGM31) | (0 << WGM30); /* TC16 Mode 0 Normal */

	TCCR3B = (0 << WGM33) | (0 << WGM32)                /* TC16 Mode 0 Normal */
	         | 0 << ICNC3                               /* Input Capture Noise Canceler: disabled */
	         | 0 << ICES3                               /* Input Capture Edge Select: disabled */
	         | (0 << CS32) | (0 << CS31) | (1 << CS30); /* No prescaling */

	// ICR3 = 0; /* Input capture value, used as top counter value in some modes: 0 */

	OCR3A = 16000; /* Output compare A: 16000 */

	// OCR3B = 0; /* Output compare B: 0 */

	TIMSK3 = 0 << OCIE3B   /* Output Compare B Match Interrupt Enable: disabled */
	         | 1 << OCIE3A /* Output Compare A Match Interrupt Enable: enabled */
	         | 0 << ICIE3  /* Input Capture Interrupt Enable: disabled */
	         | 0 << TOIE3; /* Overflow Interrupt Enable: disabled */

	return 0;
}

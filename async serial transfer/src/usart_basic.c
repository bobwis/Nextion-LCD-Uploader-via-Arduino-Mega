/**
 * \file
 *
 * \brief USART basic driver.
 *
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
 *
 */
#include <compiler.h>
#include <clock_config.h>
#include <usart_basic.h>
#include <atomic.h>

/* Static Variables holding the ringbuffer used in IRQ mode */
static uint8_t          USART_0_rxbuf[USART_0_RX_BUFFER_SIZE];
static volatile uint8_t USART_0_rx_head;
static volatile uint8_t USART_0_rx_tail;
static volatile uint8_t USART_0_rx_elements;
static uint8_t          USART_0_txbuf[USART_0_TX_BUFFER_SIZE];
static volatile uint8_t USART_0_tx_head;
static volatile uint8_t USART_0_tx_tail;
static volatile uint8_t USART_0_tx_elements;

/* Interrupt service routine for RX complete */
ISR(USART0_RX_vect)
{
	uint8_t data;
	uint8_t tmphead;

	/* Read the received data */
	data = UDR0;
	/* Calculate buffer index */
	tmphead = (USART_0_rx_head + 1) & USART_0_RX_BUFFER_MASK;
	/* Store new index */
	USART_0_rx_head = tmphead;

	if (tmphead == USART_0_rx_tail) {
		/* ERROR! Receive buffer overflow */
	}
	/* Store received data in buffer */
	USART_0_rxbuf[tmphead] = data;
	USART_0_rx_elements++;
}

/* Interrupt service routine for Data Register Empty */
ISR(USART0_UDRE_vect)
{
	uint8_t tmptail;

	/* Check if all data is transmitted */
	if (USART_0_tx_elements != 0) {
		/* Calculate buffer index */
		tmptail = (USART_0_tx_tail + 1) & USART_0_TX_BUFFER_MASK;
		/* Store new index */
		USART_0_tx_tail = tmptail;
		/* Start transmission */
		UDR0 = USART_0_txbuf[tmptail];
		USART_0_tx_elements--;
	}

	if (USART_0_tx_elements == 0) {
		/* Disable UDRE interrupt */
		UCSR0B &= ~(1 << UDRIE0);
	}
}

bool USART_0_is_tx_ready()
{
	return (USART_0_tx_elements != USART_0_TX_BUFFER_SIZE);
}

bool USART_0_is_rx_ready()
{
	return (USART_0_rx_elements != 0);
}

bool USART_0_is_tx_busy()
{
	return (!(UCSR0A & (1 << TXC0)));
}

uint8_t USART_0_read(void)
{
	uint8_t tmptail;

	/* Wait for incoming data */
	while (USART_0_rx_elements == 0)
		;
	/* Calculate buffer index */
	tmptail = (USART_0_rx_tail + 1) & USART_0_RX_BUFFER_MASK;
	/* Store new index */
	USART_0_rx_tail = tmptail;
	ENTER_CRITICAL(R);
	USART_0_rx_elements--;
	EXIT_CRITICAL(R);

	/* Return data */
	return USART_0_rxbuf[tmptail];
}

void USART_0_write(const uint8_t data)
{
	uint8_t tmphead;

	/* Calculate buffer index */
	tmphead = (USART_0_tx_head + 1) & USART_0_TX_BUFFER_MASK;
	/* Wait for free space in buffer */
	while (USART_0_tx_elements == USART_0_TX_BUFFER_SIZE)
		;
	/* Store data in buffer */
	USART_0_txbuf[tmphead] = data;
	/* Store new index */
	USART_0_tx_head = tmphead;
	ENTER_CRITICAL(W);
	USART_0_tx_elements++;
	EXIT_CRITICAL(W);
	/* Enable UDRE interrupt */
	UCSR0B |= (1 << UDRIE0);
}

int8_t USART_0_init()
{

	// Module is in UART mode

	/* Enable USART0 */
	PRR0 &= ~(1 << PRUSART0);

#define BAUD 9600

#include <utils/setbaud.h>

	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

	UCSR0A = USE_2X << U2X0 /*  */
	         | 0 << MPCM0;  /* Multi-processor Communication Mode: disabled */

	UCSR0B = 1 << RXCIE0    /* RX Complete Interrupt Enable: enabled */
	         | 0 << UDRIE0  /* USART Data Register Empty Interupt Enable: disabled */
	         | 1 << RXEN0   /* Receiver Enable: enabled */
	         | 1 << TXEN0   /* Transmitter Enable: enabled */
	         | 0 << UCSZ02; /*  */

	// UCSR0C = (0 << UMSEL01) | (0 << UMSEL00) /*  */
	//		 | (0 << UPM01) | (0 << UPM00) /* Disabled */
	//		 | 0 << USBS0 /* USART Stop Bit Select: disabled */
	//		 | (1 << UCSZ01) | (1 << UCSZ00); /* 8-bit */

	uint8_t x;

	/* Initialize ringbuffers */
	x = 0;

	USART_0_rx_tail     = x;
	USART_0_rx_head     = x;
	USART_0_rx_elements = x;
	USART_0_tx_tail     = x;
	USART_0_tx_head     = x;
	USART_0_tx_elements = x;

	return 0;
}

void USART_0_enable()
{
	UCSR0B |= ((1 << TXEN0) | (1 << RXEN0));
}

void USART_0_enable_rx()
{
	UCSR0B |= (1 << RXEN0);
}

void USART_0_enable_tx()
{
	UCSR0B |= (1 << TXEN0);
}

void USART_0_disable()
{
	UCSR0B &= ~((1 << TXEN0) | (1 << RXEN0));
}

/* Static Variables holding the ringbuffer used in IRQ mode */
static uint8_t          USART_1_rxbuf[USART_1_RX_BUFFER_SIZE];
static volatile uint8_t USART_1_rx_head;
static volatile uint8_t USART_1_rx_tail;
static volatile uint8_t USART_1_rx_elements;
static uint8_t          USART_1_txbuf[USART_1_TX_BUFFER_SIZE];
static volatile uint8_t USART_1_tx_head;
static volatile uint8_t USART_1_tx_tail;
static volatile uint8_t USART_1_tx_elements;

/* Interrupt service routine for RX complete */
ISR(USART1_RX_vect)
{
	uint8_t data;
	uint8_t tmphead;

	/* Read the received data */
	data = UDR1;
	/* Calculate buffer index */
	tmphead = (USART_1_rx_head + 1) & USART_1_RX_BUFFER_MASK;
	/* Store new index */
	USART_1_rx_head = tmphead;

	if (tmphead == USART_1_rx_tail) {
		/* ERROR! Receive buffer overflow */
	}
	/* Store received data in buffer */
	USART_1_rxbuf[tmphead] = data;
	USART_1_rx_elements++;
}

/* Interrupt service routine for Data Register Empty */
ISR(USART1_UDRE_vect)
{
	uint8_t tmptail;

	/* Check if all data is transmitted */
	if (USART_1_tx_elements != 0) {
		/* Calculate buffer index */
		tmptail = (USART_1_tx_tail + 1) & USART_1_TX_BUFFER_MASK;
		/* Store new index */
		USART_1_tx_tail = tmptail;
		/* Start transmission */
		UDR1 = USART_1_txbuf[tmptail];
		USART_1_tx_elements--;
	}

	if (USART_1_tx_elements == 0) {
		/* Disable UDRE interrupt */
		UCSR1B &= ~(1 << UDRIE1);
	}
}

bool USART_1_is_tx_ready()
{
	return (USART_1_tx_elements != USART_1_TX_BUFFER_SIZE);
}

bool USART_1_is_rx_ready()
{
	return (USART_1_rx_elements != 0);
}

bool USART_1_is_tx_busy()
{
	return (!(UCSR1A & (1 << TXC1)));
}

uint8_t USART_1_read(void)
{
	uint8_t tmptail;

	/* Wait for incoming data */
	while (USART_1_rx_elements == 0)
		;
	/* Calculate buffer index */
	tmptail = (USART_1_rx_tail + 1) & USART_1_RX_BUFFER_MASK;
	/* Store new index */
	USART_1_rx_tail = tmptail;
	ENTER_CRITICAL(R);
	USART_1_rx_elements--;
	EXIT_CRITICAL(R);

	/* Return data */
	return USART_1_rxbuf[tmptail];
}

void USART_1_write(const uint8_t data)
{
	uint8_t tmphead;

	/* Calculate buffer index */
	tmphead = (USART_1_tx_head + 1) & USART_1_TX_BUFFER_MASK;
	/* Wait for free space in buffer */
	while (USART_1_tx_elements == USART_1_TX_BUFFER_SIZE)
		;
	/* Store data in buffer */
	USART_1_txbuf[tmphead] = data;
	/* Store new index */
	USART_1_tx_head = tmphead;
	ENTER_CRITICAL(W);
	USART_1_tx_elements++;
	EXIT_CRITICAL(W);
	/* Enable UDRE interrupt */
	UCSR1B |= (1 << UDRIE1);
}

int8_t USART_1_init()
{

	// Module is in UART mode

	/* Enable USART1 */
	PRR1 &= ~(1 << PRUSART1);

#define BAUD 9600

#include <utils/setbaud.h>

	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;

	UCSR1A = USE_2X << U2X1 /*  */
	         | 0 << MPCM1;  /* Multi-processor Communication Mode: disabled */

	UCSR1B = 1 << RXCIE1    /* RX Complete Interrupt Enable: enabled */
	         | 0 << UDRIE1  /* USART Data Register Empty Interupt Enable: disabled */
	         | 1 << RXEN1   /* Receiver Enable: enabled */
	         | 1 << TXEN1   /* Transmitter Enable: enabled */
	         | 0 << UCSZ12; /*  */

	// UCSR1C = (0 << UMSEL11) | (0 << UMSEL10) /*  */
	//		 | (0 << UPM11) | (0 << UPM10) /* Disabled */
	//		 | 0 << USBS1 /* USART Stop Bit Select: disabled */
	//		 | (1 << UCSZ11) | (1 << UCSZ10); /* 8-bit */

	uint8_t x;

	/* Initialize ringbuffers */
	x = 0;

	USART_1_rx_tail     = x;
	USART_1_rx_head     = x;
	USART_1_rx_elements = x;
	USART_1_tx_tail     = x;
	USART_1_tx_head     = x;
	USART_1_tx_elements = x;

	return 0;
}

void USART_1_enable()
{
	UCSR1B |= ((1 << TXEN1) | (1 << RXEN1));
}

void USART_1_enable_rx()
{
	UCSR1B |= (1 << RXEN1);
}

void USART_1_enable_tx()
{
	UCSR1B |= (1 << TXEN1);
}

void USART_1_disable()
{
	UCSR1B &= ~((1 << TXEN1) | (1 << RXEN1));
}

/* Static Variables holding the ringbuffer used in IRQ mode */
static uint8_t          USART_2_rxbuf[USART_2_RX_BUFFER_SIZE];
static volatile uint8_t USART_2_rx_head;
static volatile uint8_t USART_2_rx_tail;
static volatile uint8_t USART_2_rx_elements;
static uint8_t          USART_2_txbuf[USART_2_TX_BUFFER_SIZE];
static volatile uint8_t USART_2_tx_head;
static volatile uint8_t USART_2_tx_tail;
static volatile uint8_t USART_2_tx_elements;

/* Interrupt service routine for RX complete */
ISR(USART2_RX_vect)
{
	uint8_t data;
	uint8_t tmphead;

	/* Read the received data */
	data = UDR2;
	/* Calculate buffer index */
	tmphead = (USART_2_rx_head + 1) & USART_2_RX_BUFFER_MASK;
	/* Store new index */
	USART_2_rx_head = tmphead;

	if (tmphead == USART_2_rx_tail) {
		/* ERROR! Receive buffer overflow */
	}
	/* Store received data in buffer */
	USART_2_rxbuf[tmphead] = data;
	USART_2_rx_elements++;
}

/* Interrupt service routine for Data Register Empty */
ISR(USART2_UDRE_vect)
{
	uint8_t tmptail;

	/* Check if all data is transmitted */
	if (USART_2_tx_elements != 0) {
		/* Calculate buffer index */
		tmptail = (USART_2_tx_tail + 1) & USART_2_TX_BUFFER_MASK;
		/* Store new index */
		USART_2_tx_tail = tmptail;
		/* Start transmission */
		UDR2 = USART_2_txbuf[tmptail];
		USART_2_tx_elements--;
	}

	if (USART_2_tx_elements == 0) {
		/* Disable UDRE interrupt */
		UCSR2B &= ~(1 << UDRIE2);
	}
}

bool USART_2_is_tx_ready()
{
	return (USART_2_tx_elements != USART_2_TX_BUFFER_SIZE);
}

bool USART_2_is_rx_ready()
{
	return (USART_2_rx_elements != 0);
}

bool USART_2_is_tx_busy()
{
	return (!(UCSR2A & (1 << TXC2)));
}

uint8_t USART_2_read(void)
{
	uint8_t tmptail;

	/* Wait for incoming data */
	while (USART_2_rx_elements == 0)
		;
	/* Calculate buffer index */
	tmptail = (USART_2_rx_tail + 1) & USART_2_RX_BUFFER_MASK;
	/* Store new index */
	USART_2_rx_tail = tmptail;
	ENTER_CRITICAL(R);
	USART_2_rx_elements--;
	EXIT_CRITICAL(R);

	/* Return data */
	return USART_2_rxbuf[tmptail];
}

void USART_2_write(const uint8_t data)
{
	uint8_t tmphead;

	/* Calculate buffer index */
	tmphead = (USART_2_tx_head + 1) & USART_2_TX_BUFFER_MASK;
	/* Wait for free space in buffer */
	while (USART_2_tx_elements == USART_2_TX_BUFFER_SIZE)
		;
	/* Store data in buffer */
	USART_2_txbuf[tmphead] = data;
	/* Store new index */
	USART_2_tx_head = tmphead;
	ENTER_CRITICAL(W);
	USART_2_tx_elements++;
	EXIT_CRITICAL(W);
	/* Enable UDRE interrupt */
	UCSR2B |= (1 << UDRIE2);
}

int8_t USART_2_init()
{

	// Module is in UART mode

	/* Enable USART2 */
	PRR1 &= ~(1 << PRUSART2);

#define BAUD 9600

#include <utils/setbaud.h>

	UBRR2H = UBRRH_VALUE;
	UBRR2L = UBRRL_VALUE;

	UCSR2A = USE_2X << U2X2 /*  */
	         | 0 << MPCM2;  /* Multi-processor Communication Mode: disabled */

	UCSR2B = 1 << RXCIE2    /* RX Complete Interrupt Enable: enabled */
	         | 0 << UDRIE2  /* USART Data Register Empty Interupt Enable: disabled */
	         | 1 << RXEN2   /* Receiver Enable: enabled */
	         | 1 << TXEN2   /* Transmitter Enable: enabled */
	         | 0 << UCSZ22; /*  */

	// UCSR2C = (0 << UMSEL21) | (0 << UMSEL20) /*  */
	//		 | (0 << UPM21) | (0 << UPM20) /* Disabled */
	//		 | 0 << USBS2 /* USART Stop Bit Select: disabled */
	//		 | (1 << UCSZ21) | (1 << UCSZ20); /* 8-bit */

	uint8_t x;

	/* Initialize ringbuffers */
	x = 0;

	USART_2_rx_tail     = x;
	USART_2_rx_head     = x;
	USART_2_rx_elements = x;
	USART_2_tx_tail     = x;
	USART_2_tx_head     = x;
	USART_2_tx_elements = x;

	return 0;
}

void USART_2_enable()
{
	UCSR2B |= ((1 << TXEN2) | (1 << RXEN2));
}

void USART_2_enable_rx()
{
	UCSR2B |= (1 << RXEN2);
}

void USART_2_enable_tx()
{
	UCSR2B |= (1 << TXEN2);
}

void USART_2_disable()
{
	UCSR2B &= ~((1 << TXEN2) | (1 << RXEN2));
}

#include <stdio.h>

#if defined(__GNUC__)

int USART_3_printCHAR(char character, FILE *stream)
{
	USART_3_write(character);
	return 0;
}

FILE USART_3_stream = FDEV_SETUP_STREAM(USART_3_printCHAR, NULL, _FDEV_SETUP_WRITE);

#elif defined(__ICCAVR__)

int putchar(int outChar)
{
	USART_0_write(outChar);
	return outChar;
}
#endif

/* Static Variables holding the ringbuffer used in IRQ mode */
static uint8_t          USART_3_rxbuf[USART_3_RX_BUFFER_SIZE];
static volatile uint8_t USART_3_rx_head;
static volatile uint8_t USART_3_rx_tail;
static volatile uint8_t USART_3_rx_elements;
static uint8_t          USART_3_txbuf[USART_3_TX_BUFFER_SIZE];
static volatile uint8_t USART_3_tx_head;
static volatile uint8_t USART_3_tx_tail;
static volatile uint8_t USART_3_tx_elements;

/* Interrupt service routine for RX complete */
ISR(USART3_RX_vect)
{
	uint8_t data;
	uint8_t tmphead;

	/* Read the received data */
	data = UDR3;
	/* Calculate buffer index */
	tmphead = (USART_3_rx_head + 1) & USART_3_RX_BUFFER_MASK;
	/* Store new index */
	USART_3_rx_head = tmphead;

	if (tmphead == USART_3_rx_tail) {
		/* ERROR! Receive buffer overflow */
	}
	/* Store received data in buffer */
	USART_3_rxbuf[tmphead] = data;
	USART_3_rx_elements++;
}

/* Interrupt service routine for Data Register Empty */
ISR(USART3_UDRE_vect)
{
	uint8_t tmptail;

	/* Check if all data is transmitted */
	if (USART_3_tx_elements != 0) {
		/* Calculate buffer index */
		tmptail = (USART_3_tx_tail + 1) & USART_3_TX_BUFFER_MASK;
		/* Store new index */
		USART_3_tx_tail = tmptail;
		/* Start transmission */
		UDR3 = USART_3_txbuf[tmptail];
		USART_3_tx_elements--;
	}

	if (USART_3_tx_elements == 0) {
		/* Disable UDRE interrupt */
		UCSR3B &= ~(1 << UDRIE3);
	}
}

bool USART_3_is_tx_ready()
{
	return (USART_3_tx_elements != USART_3_TX_BUFFER_SIZE);
}

bool USART_3_is_rx_ready()
{
	return (USART_3_rx_elements != 0);
}

bool USART_3_is_tx_busy()
{
	return (!(UCSR3A & (1 << TXC3)));
}

uint8_t USART_3_read(void)
{
	uint8_t tmptail;

	/* Wait for incoming data */
	while (USART_3_rx_elements == 0)
		;
	/* Calculate buffer index */
	tmptail = (USART_3_rx_tail + 1) & USART_3_RX_BUFFER_MASK;
	/* Store new index */
	USART_3_rx_tail = tmptail;
	ENTER_CRITICAL(R);
	USART_3_rx_elements--;
	EXIT_CRITICAL(R);

	/* Return data */
	return USART_3_rxbuf[tmptail];
}

void USART_3_write(const uint8_t data)
{
	uint8_t tmphead;

	/* Calculate buffer index */
	tmphead = (USART_3_tx_head + 1) & USART_3_TX_BUFFER_MASK;
	/* Wait for free space in buffer */
	while (USART_3_tx_elements == USART_3_TX_BUFFER_SIZE)
		;
	/* Store data in buffer */
	USART_3_txbuf[tmphead] = data;
	/* Store new index */
	USART_3_tx_head = tmphead;
	ENTER_CRITICAL(W);
	USART_3_tx_elements++;
	EXIT_CRITICAL(W);
	/* Enable UDRE interrupt */
	UCSR3B |= (1 << UDRIE3);
}

int8_t USART_3_init()
{

	// Module is in UART mode

	/* Enable USART3 */
	PRR1 &= ~(1 << PRUSART3);

#define BAUD 9600

#include <utils/setbaud.h>

	UBRR3H = UBRRH_VALUE;
	UBRR3L = UBRRL_VALUE;

	UCSR3A = USE_2X << U2X3 /*  */
	         | 0 << MPCM3;  /* Multi-processor Communication Mode: disabled */

	UCSR3B = 1 << RXCIE3    /* RX Complete Interrupt Enable: enabled */
	         | 0 << UDRIE3  /* USART Data Register Empty Interupt Enable: disabled */
	         | 1 << RXEN3   /* Receiver Enable: enabled */
	         | 1 << TXEN3   /* Transmitter Enable: enabled */
	         | 0 << UCSZ32; /*  */

	// UCSR3C = (0 << UMSEL31) | (0 << UMSEL30) /*  */
	//		 | (0 << UPM31) | (0 << UPM30) /* Disabled */
	//		 | 0 << USBS3 /* USART Stop Bit Select: disabled */
	//		 | (1 << UCSZ31) | (1 << UCSZ30); /* 8-bit */

	uint8_t x;

	/* Initialize ringbuffers */
	x = 0;

	USART_3_rx_tail     = x;
	USART_3_rx_head     = x;
	USART_3_rx_elements = x;
	USART_3_tx_tail     = x;
	USART_3_tx_head     = x;
	USART_3_tx_elements = x;

#if defined(__GNUC__)
	stdout = &USART_3_stream;
#endif

	return 0;
}

void USART_3_enable()
{
	UCSR3B |= ((1 << TXEN3) | (1 << RXEN3));
}

void USART_3_enable_rx()
{
	UCSR3B |= (1 << RXEN3);
}

void USART_3_enable_tx()
{
	UCSR3B |= (1 << TXEN3);
}

void USART_3_disable()
{
	UCSR3B &= ~((1 << TXEN3) | (1 << RXEN3));
}

// Arduino Mega pass-thru loader for Nextion LCD
// V0.1	8th Sept 2017 JRW
// Now under revision control, committed 8/9/2017

// Second change

#include <atmel_start.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <atomic.h>

#define LCDPORT USART_2

extern volatile uint64_t msectimer0;

static char lcdsig[80];			// holds the returned LCD signature string

const uint8_t string[12] PROGMEM = {"hello world!"};

// baud rates corresponding to the clock settings below
const uint32_t bauds[7]={
	2400,115200,4800,57600,9600,38400,19200
};

// baud rate clock settings from 2400 to 115200
// column for clock multiplier used 1 or 2
#define BMULT 0		// column
const uint16_t btable[7][2]={
	{416,832},	// 2400
	{8,16},		// 115.2k
	{207,416},	// 4800
	{16,34},	// 57.6k
	{103,207},	// 9600
	{25,51},	// 38400
	{51,103}	// 19200
};

#if 0
void delay_ms(uint16_t count) {
	while(count--) {
		_delay_ms(1);
	}
}
#else
void delay_ms(uint16_t count)
{
	volatile uint64_t k,j;
	while(1) {
		cli();
		j = msectimer0;
		sei();
		k = j + (uint64_t)count;
		while(1) {
			cli();
			j = msectimer0;
			sei();
			if (j >= k)
			{
				return;
			}
		}
	}
}
#endif

// set the baud on the fly for usart 0; this makes sure uart is idle
void set0baud(int baudindex) {

	//Check UART tx Data has been completed
	while (!(UCSR0A & (1 <<UDRE0)))			// 	while (!(UCSR0A & (1 <<TXC0)))
	;
	ENTER_CRITICAL(W);
	while (!(UCSR0A & (1 <<UDRE0)))		// make sure no sneaky isr got in
	;

	if (baudindex == 1)		// 115200


	// deactivate USART
	UCSR0B = 0 << RXCIE0    /* RX Complete Interrupt Enable: enabled */
	| 0 << UDRIE0  /* USART Data Register Empty Interupt Enable: disabled */
	| 0 << RXEN0   /* Receiver Enable: enabled */
	| 0 << TXEN0   /* Transmitter Enable: enabled */
	| 0 << UCSZ02; /*  */

	if (baudindex == 1)		// 115200
	{
		UCSR0A |= (1 << U2X0);
		//Reconfigure baud rate
		UBRR0H = (btable[baudindex][1] >> 8);
		UBRR0L = (btable[baudindex][1] & 0xff);
	}
	else
	{
		UCSR0A &= ~(1 << U2X0);
		//Reconfigure baud rate
		UBRR0H = (btable[baudindex][BMULT] >> 8);
		UBRR0L = (btable[baudindex][BMULT] & 0xff);
	}

	// activate USART
	UCSR0B = 1 << RXCIE0    /* RX Complete Interrupt Enable: enabled */
	| 0 << UDRIE0  /* USART Data Register Empty Interupt Enable: disabled */
	| 1 << RXEN0   /* Receiver Enable: enabled */
	| 1 << TXEN0   /* Transmitter Enable: enabled */
	| 0 << UCSZ02; /*  */

	EXIT_CRITICAL(W);
}

// set the baud on the fly for usart 2; this makes sure uart is idle
void set2baud(int baudindex) {

	//Check UART tx Data has been completed
	while (!(UCSR2A & (1 <<UDRE2)))			// 	while (!(UCSR2A & (1 <<TXC2)))
	;
	ENTER_CRITICAL(W);
	while (!(UCSR2A & (1 <<UDRE2)))		// make sure no sneaky isr got in
	;
	// deactivate USART
	UCSR2B = 0 << RXCIE2    /* RX Complete Interrupt Enable: enabled */
	| 0 << UDRIE2  /* USART Data Register Empty Interupt Enable: disabled */
	| 0 << RXEN2   /* Receiver Enable: enabled */
	| 0 << TXEN2   /* Transmitter Enable: enabled */
	| 0 << UCSZ22; /*  */
	
	//Reconfigure baud rate
	UBRR2H = (btable[baudindex][BMULT] >> 8);
	UBRR2L = (btable[baudindex][BMULT] & 0xff);

	// activate USART
	UCSR2B = 1 << RXCIE2    /* RX Complete Interrupt Enable: enabled */
	| 0 << UDRIE2  /* USART Data Register Empty Interupt Enable: disabled */
	| 1 << RXEN2   /* Receiver Enable: enabled */
	| 1 << TXEN2   /* Transmitter Enable: enabled */
	| 0 << UCSZ22; /*  */

	EXIT_CRITICAL(W);
}

// Find the LCD and return the current baud rate or -1 if not found
int findlcd(void)
{
	const char discovermsg[]="\x00\xff\xff\xff""connect\xff\xff\xff";	// discovery message
	const char foundmsg[]="comok ";		// first part of expected LCD response
	char response[128];	// response buffer

	int	i, j, rindex, bindex;
	int inindex = 0;
	int wtim = 0;

	// clear arrays
	memset(lcdsig, 0, sizeof lcdsig);
	memset(response, 0, sizeof response);

	for(bindex=0; bindex<sizeof(btable); bindex++)		// try every baud
	{
		inindex = 0;
		set2baud(bindex);			// set the LCD baud rate
		delay_ms(2);			// allow baud gen to settle

		for(j=0; j<sizeof(discovermsg)-1; j++)		// send discovery command to LCD
		{
			USART_2_write(discovermsg[j]);	// connect
			while(USART_2_is_rx_ready())
			{
				if(inindex < sizeof(response))
				{
					response[inindex++] = USART_2_read();
				}
			}
		}

		for (wtim = 0; (wtim < 250); wtim++)		// hang around a bit and try to collect complete response
		{
			while(USART_2_is_rx_ready())
			{
				if(inindex < sizeof(response))
				{
					response[inindex++] = USART_2_read();
				}
			}
			delay_ms(1);			// allow one char time at 9600 baud
		}

		if (inindex)		// we *have* received something
		{
			for(rindex=0; rindex<inindex; rindex++)		// the length of the rx'd string
			{
				if (strncmp(&response[rindex],foundmsg,sizeof(foundmsg-1)) == 0)		// look for the start
				{
					for(i=rindex; i<inindex; i++)		// found start, now look for terminator bytes
					{
						if ((response[i] == 0xff) && (response[i+1] == 0xff) && (response[i+2] == 0xff))	// found response terminator
						{
							if(i+2-rindex > sizeof(lcdsig)-1)		// will fit in the buffer
							{
								printf("LCD response too long\n\r");
								return(-1);
							}
							else
							{
								int k;
								j = 0;
								k = i + 3 - rindex;
								while (j < k)
								{
									lcdsig[j++] = response[rindex++];		// copy response string into global
								}
								lcdsig[j] = '\0';		// add our null terminator
								return(bindex);
							}
						}
					}
				}

			}
		}
	}
	return(-1);
}


// see if Nextion editor connects
int getconnect(char buf[], int bsize)
{
	int inindex = 0, mindex = 0;
	int wtim, i;
	char ch;
	const char discovermsg[]="connect\xff\xff\xff";		// expected discovery message

	for(i=0; i<bsize; buf[i++]='\0');
	for (wtim = 0; (wtim < 7000); wtim++)		// hang around waiting for some input
	{
		while(USART_0_is_rx_ready())
		{
			if(inindex < bsize)
			{
				ch = USART_0_read();
				buf[inindex++] = ch;
				if (discovermsg[mindex] == ch)
				{
					mindex++;
					if (mindex == sizeof(discovermsg)-1)	// all matched
					{
						return(0);
					}
				}
				else
				{
					mindex = 0;		// reset the search
					inindex = 0;
				}
			}
			else
			{
				// input buffer full
				inindex = 0;
				for(i=0; i<bsize; buf[i++]='\0');
			}
		}
		delay_ms(1);
	}
	return(-1);
}

// wait for connect from Nextion Editor and respond
int conntoed()
{
	int i;
	char inbuf[128];
	const char nulresp[]={0x1a,0xff,0xff,0xff};


	i = getconnect(inbuf,sizeof(inbuf));
	if (i < 0)
	{
		return(-1);
	}
	// Pc has connected, now send LCD signature response
	for(i=0; i<4; i++)		// send error response - might not be needed
	{
		USART_0_write(nulresp[i]);
	}
	i = 0;
	while(lcdsig[i])
	{
		USART_0_write(lcdsig[i++]);		// send the saved LCD response to the Editor
	}
	return(0);
}


// wait for and parse the baud rate from the 'upload' command
// return the new baud rate or -1 if not found
long getupcmd(void)
{
	int mindex = 0;
	int wtim, termcnt = 0, commacnt = 0;
	volatile long newbaud = 0;
	char ch;
	bool validcmd = false;

	const char uploadmsg[]="whmi-wri ";		// expected upload command

	for (wtim = 0; (wtim < 5000); wtim++)		// hang around waiting for some input
	{
		while(USART_0_is_rx_ready())
		{
			ch = USART_0_read();
			USART_2_write(ch);	// copy to the LCD
			if (!(validcmd)) {
				if (uploadmsg[mindex] == ch)			// compare this char with upload cmd string
				{
					//						USART_3_write(ch);
					mindex++;
					if (mindex == sizeof(uploadmsg)-1)	// all matched
					{
						validcmd = true;
						commacnt = 0;
						termcnt = 0;
						newbaud = 0;
					}
				}
				else
				{
					//						inindex = 0;	// no need to keep that input
					mindex = 0;		// reset the search
				}
			}
			else
			{
				// valid upload command seen - we need to get the params and find the end
				if (ch == 0xff)
				{
					termcnt++;
					if (termcnt == 3)
					{
						return(newbaud);
					}
				}
				if (ch == ',')		// comma between parameters
				{
					commacnt++;
				}
				if (commacnt == 1)
				{
					if ((ch >= '0') && (ch <= '9'))
					{
						newbaud *= 10;
						newbaud = newbaud + ch - '0';
					}
				}

			}
		}
		while(USART_2_is_rx_ready())
		{
			ch = USART_2_read();
			USART_0_write(ch);	// copy to the PC
		}
		delay_ms(1);
	}
	return((newbaud > 0) ? newbaud : -1L);
}

// wait for upload command from Nextion Editor and send it to the LCD
// then change the baud rates and perform the transfer
int doupload()
{
	register char ch;
	unsigned long baudrate;
	int bindex;
	volatile long bcount;
	register uint8_t active, started;
	uint64_t now;

	baudrate = getupcmd();
	if ((long)baudrate < 0)
	{
		return(-1);
	}
	// Pc has sent upload command
	printf("Starting Upload @ %ld\n\r",baudrate);

	// set the specified baudrate
	bindex = 0;
	while(bindex < 7)
	{
		if (bauds[bindex] == baudrate)
		{
			break;
		}
		bindex++;
	}

	set0baud(bindex);			// set the PC baud rate
	set2baud(bindex);			// set the LCD baud rate

	active = 0;
	started = 0;
	bcount = 0;
	now = msectimer0;
	for(;;)
	{
		if (USART_0_is_rx_ready())
		{
			active = 1;
			started = 1;
			ch = USART_0_read();
			USART_2_write(ch);	// copy to the LCD
			bcount++;
		}
#if 0
		while (USART_0_is_rx_ready())	// slurp
		{
			ch = USART_0_read();
			USART_2_write(ch);	// copy to the LCD
		}
#endif
		if(USART_2_is_rx_ready())
		{
			ch = USART_2_read();
			USART_0_write(ch);	// copy to the PC
		}
		if (active++ == 0)		// once every 256 loops with no uart0 rx
		{
			if (msectimer0 > now+5000L)
			{
				break;
			}
			now = msectimer0;
		}
	}
	bindex = bcount;
	return((started) ? 0 : -1);
}


int main(void)
{
	unsigned int ledcnt = 0;
	volatile int i;
	int initialbaud;

	char hellomsg[]="Hello\r\n";

	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

	/* Replace with your application code */
	sei();

	for(i=0; i<sizeof(hellomsg)-1; i++)
	{
		USART_3_write(hellomsg[i]);
	}

	while (1)
	{
		ledcnt = 1;
		i = -1;
		while (i < 1)
		{
			printf("Finding LCD\n\r");
			i = findlcd();
		}
		initialbaud = i;
		//		printf("Found LCD at bindex %d, %s\n\r",i,lcdsig);
		printf("Found LCD\n\r");
		i = -1;
		while( i < 0)
		{
			printf("Waiting for Nextion Editor to connect\n\r");
			i = conntoed();
			if (i >= 0)
			{
				printf("Nextion Editor connected\n\r");
			}
		}

		printf("Waiting for upload cmd\n\r");
		if (doupload() < 0)			// did not receive the upload command
		{
			continue;
		}

		set0baud(initialbaud);			// reset the PC baud rate
		set2baud(initialbaud);			// reset the LCD baud rate
		
		printf("Upload timed out, finished?\n\r");

		ledcnt = 0x4000;
		if(ledcnt)
		{
			ledcnt--;
			Led_set_level(1);
		}
		else
		{
			Led_set_level(0);
		}
	}
}
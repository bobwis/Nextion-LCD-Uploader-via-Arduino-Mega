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

#define LCDPORT USART_2

const uint8_t string[12] PROGMEM = {"hello world!"};

// baud rate from 2400 to 115200
const uint16_t btable[7][2]={
	{416,832},	// 2400
	{8,16},		// 115.2k
	{207,416},	// 4800
	{16,34},	// 57.6k
	{103,207},	// 9600
	{25,51},	// 38400
	{51,103}	// 19200
};

void dly(unsigned int value)
{
	#if 1
	volatile unsigned int i,j;

	for(j=0; j<value; j++)
	for(i=0;i<3;i++);	// dly
	#else
	_delay_us ((double)(value));
	#endif
}


static char lcdsig[128];

// Find the LCD and return the current baud rate  or -1 if not found
int findlcd()
{
	const char discovermsg[]="\x00\xff\xff\xff""connect\xff\xff\xff";	// discovery message

	const char foundmsg[]="comok ";		// first part of expected LCD response
	char response[128];	// response buffer
	char data;

	int	i, j, rindex, bindex;
	int inindex = 0;
	int wtim = 0;

	for(i=0; i<sizeof(lcdsig); i++)
	{
		lcdsig[i] = 0;			// clear the rx bufffer
	}
	for(i=0; i<sizeof(response); i++)
	{
		response[i] = 0;			// clear the response bufffer
	}

	for(bindex=0; bindex<sizeof(btable); bindex++)		// try every baud
	{
		inindex = 0;

		UBRR2H = (btable[bindex][0] >> 8);
		UBRR2L = (btable[bindex][0] & 0xff);
		dly(12000);

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

		for (wtim = 0; (wtim < 32000); wtim++)		// hang around a bit and try to collect complete response
		{
			while(USART_2_is_rx_ready())
			{
				if(inindex < sizeof(response))
				{
					response[inindex++] = USART_2_read();
				}
			}
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
							for(j=0; j<(i+2-rindex); j++)
							{
								lcdsig[j] = response[rindex++];		// copy response string into global
							}
							return(bindex);
						}
					}
				}

			}
		}
	}
	return(-1);
}

int main(void)
{
	uint8_t data;
	unsigned int i,bps, ledcnt = 0;
	volatile unsigned int j;

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
		i = findlcd();
		USART_3_write(i+'0');
		ledcnt = lcdsig[0];
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
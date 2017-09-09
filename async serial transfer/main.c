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

extern volatile uint64_t msectimer0;

static char lcdsig[80];			// holds the returned LCD signature string

const uint8_t string[12] PROGMEM = {"hello world!"};

// baud rates corresponding to the clock settings below
const uint64_t bauds[7]={
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
		UBRR2H = (btable[bindex][BMULT] >> 8);
		UBRR2L = (btable[bindex][BMULT] & 0xff);
		delay_ms(10);			// allow baud gen to settle

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
			delay_ms(1);			// allow one char time at 2400 baud, 5ms is 64 chars at 115200
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
								//		the expression evals wrong??			while (j < (i+3-rindex))
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
	// test
	lcdsig[0]=128;
	return(-1);
}


// see if Nextion editor connects
int getconnect(char buf[], int bsize)
{
	int inindex = 0, mindex = 0;
	int wtim, i;
	volatile char ch;
	//	volatile char buf[128];
	const char discovermsg[]="connect\xff\xff\xff";		// expected discovery message

	for(i=0; i<bsize; buf[i++]='\0');
	for (wtim = 0; (wtim < 5000); wtim++)		// hang around waiting for some input
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
					if (mindex == sizeof(discovermsg))	// all matched
					{
						ch = i & 0xff;
						return(1);
					}
				}
				else
				{
					mindex = 0;		// reset the search
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
	volatile int i;
	char ch;
	char inbuf[128];
	const char nulresp[]={0x1a,0xff,0xff,0xff};

	i = getconnect(inbuf,sizeof(inbuf));
	if (i < 0)
	{
		return(-1);
	}
	// Pc has connected, now send LCD signature response


	for(i=0; i<4; i++)
	{
		USART_0_write(nulresp[i]);
		USART_3_write(nulresp[i]);
	}

	i = 0;
	while(lcdsig[i])
	{
		USART_0_write(lcdsig[i]);
		//		USART_3_write(lcdsig[i++]);
	}
	return(0);
}


// wait for and parse the baud rate from the upload command
int getupcmd(char* buf, int bsize)
{
	int inindex = 0, mindex = 0;
	int wtim, i, termcnt;
	char ch;
	bool validcmd = false;

	const char uploadmsg[]="whmi-wri ";		// expected upload command

	for(i=0; i<bsize; buf[i++]='\0');
	termcnt = 0;
	for (wtim = 0; (wtim < 5000); wtim++)		// hang around waiting for some input
	{
		while(USART_0_is_rx_ready())
		{
			if(inindex < bsize)		// check not overflowed our buffer
			{
				ch = USART_0_read();
				buf[inindex++] = ch;
				if (!(validcmd)) {
					if (uploadmsg[mindex] == ch)			// compare this char
					{
						USART_3_write(ch);
						mindex++;
						if (mindex == sizeof(uploadmsg))	// all matched
						{
							validcmd = true;
						}
					}
					else
					{
						inindex = 0;	// no need to keep that input
						mindex = 0;		// reset the search
					}
				}
				else
				{
					// valid upload command - we need to get the params and find the end
					if (ch == 0xff)
					{
						termcnt++;
						if (termcnt == 3)
						{
							return(0);
						}
					}
				}
			}
			else
			{
				// input buffer full
				inindex = 0;
				validcmd = false;
				termcnt = 0;
				for(i=0; i<bsize; buf[i++]='\0');
			}
		}
		delay_ms(1);
	}
	return(-1);
}

// wait for upload command from Nextion Editor and send it to the LCD
// then change the baud rates
int doupload()
{
	int i;
	char ch;
	char inbuf[128];
	unsigned long filesize, baudrate;
	char cmd[16];

	i = getupcmd(inbuf,sizeof(inbuf));
	if (i < 0)
	{
		return(-1);
	}
	// Pc has sent upload command
	sscanf(inbuf,"%s,%ld,%ld",cmd,&filesize,&baudrate);

	i = (int)baudrate;
	i = i + (int)filesize;
	return(0);
}


int main(void)
{
	uint8_t data;
	unsigned int bps, ledcnt = 0;
	volatile int i;
	volatile unsigned int j, k;

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
//		printf("Found LCD at bindex %d, %s\n\r",i,lcdsig);
		printf("Found LCD\n\r");
		i = -1;
		while( i < 0)
		{
			printf("Waiting for Nextion Editor to connect\n\r");
			i = conntoed();
		}


		doupload();


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
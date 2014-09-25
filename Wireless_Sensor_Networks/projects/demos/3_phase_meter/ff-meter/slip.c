#include <avr/interrupt.h>
#include <stdio.h>
#include "slip.h"

#define C_HeaderBytes 3

//uint8_t slip_tx (uint8_t *buf, int size, uint8_t *EndOfBuff, uint32_t MaxSizeBuff, uint8_t *buf_out)
uint8_t slip_tx (uint8_t *buf, int size, uint8_t *EndOfBuff, uint32_t MaxSizeBuff, uint8_t *buf_out, uint8_t pkt_iden, uint8_t meter_iden, uint8_t slip_pkt_seq)
{
	uint8_t i;
	uint8_t checksum;
	int8_t c;
	uint8_t index = 0;
	uint8_t header[C_HeaderBytes];

	if(size > 128)
		return;

	checksum = 0;

	//Send Start byte
	c = STARTx;
	*(buf_out+ index++)= c;	//putchar(c);

	c = size+C_HeaderBytes; //Since we are sending out pkt_iden and meter_iden
//	c = size; //Since we are sending out pkt_iden and meter_iden
	*(buf_out+ index++)=c; //putchar(c);;

	header[0]=pkt_iden;
	header[1]=meter_iden;
	header[2]=slip_pkt_seq;

	for (i=0; i<C_HeaderBytes; i++)
	{
		if(header[i] == END || header[i] ==ESC)
		{
			c = ESC;
			*(buf_out+ index++)=c; //putchar(c);;
		}
		c = header[i];
		*(buf_out+ index++)=c; //putchar(c);;
		checksum += header[i];
	}
	for (i=0; i<size; i++)
	{
		if(*buf == END || *buf ==ESC)
		{
			c = ESC;
			*(buf_out+ index++)=c; //putchar(c);;
		}
		c = *buf;
		*(buf_out+ index++)=c; //putchar(c);;
		checksum += *buf;

		buf++;
		if(pkt_iden == 'F')//For fast data
			if(buf > EndOfBuff)
				buf = buf-MaxSizeBuff;
	}

	checksum &= 0x7f;

	//Send end byte
	c = checksum;
	*(buf_out+ index++)=c; //putchar(c);;
	c = END;
	*(buf_out+ index++)=c; //putchar(c);;

	return index;
}
/*
void slip_tx (uint8_t *buf, int size, uint8_t *EndOfBuff, uint32_t MaxSizeBuff)
{
	uint8_t i;
	uint8_t checksum;
	int8_t c;

	if(size > 128)
		return;

	checksum = 0;

	//Send Start byte
	c = STARTx;
	putchar(c);

	c = size;
	putchar(c);

	for (i=0; i<size; i++)
	{
		if(*buf == END || *buf ==ESC)
		{
			c = ESC;
			putchar(c);
		}
		c = *buf;
		putchar(c);
		checksum += *buf;

		buf++;
		if(buf > EndOfBuff)
			buf = buf-MaxSizeBuff;
	}

	checksum &= 0x7f;

	//Send end byte
	c = checksum;
	putchar(c);
	c = END;
	putchar(c);
}*/

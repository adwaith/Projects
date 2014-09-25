/******************************************************************************
*  Nano-RK, a real-time operating system for sensor networks.
*  Copyright (C) 2007, Real-Time and Multimedia Lab, Carnegie Mellon University
*  All rights reserved.
*
*  This is the Open Source Version of Nano-RK included as part of a Dual
*  Licensing Model. If you are unsure which license to use please refer to:
*  http://www.nanork.org/nano-RK/wiki/Licensing
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, version 2.0 of the License.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*******************************************************************************/
//OOK modulation
//Configure the number of nodes. For each node, send through serial the OFF freq, ON freq and the Data byte value
//Send 200 as the first byte in the serial packet to keep the LED ON for 30 seconds
#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>


// This is the address in the msg array that this node will use
#define VLC_ADDR	4//Start with 0

// This is the total number of nodes supported
#define TOTAL_NODES	5


#define MSG_SIZE	6 //Freq_ON Freq_OFF Data RptJitter Jitter_HighByte Jitter_LowByte
#define MSG_BUF_SIZE	(MSG_SIZE*TOTAL_NODES)		
#define SYMBOL_LEN	50
//uint8_t msg_buf[MSG_BUF_SIZE];
uint8_t msg_buf[64];

nrk_sig_t uart_rx_signal;
nrk_sig_mask_t sm;

NRK_STK Stack1[NRK_APP_STACKSIZE];
nrk_task_type TaskOne;
void Task1(void);

NRK_STK Stack2[NRK_APP_STACKSIZE];
nrk_task_type TaskTwo;
void Task2 (void);
/*
NRK_STK Stack3[NRK_APP_STACKSIZE];
nrk_task_type TaskThree;
void Task3 (void);*/

uint16_t compare_vt;
uint8_t f_toggle=0;
uint8_t freq_f=0;
uint8_t led_active;

uint8_t jtemp;
uint16_t jitter_us,jitterH,jitterL,jitter_ustemp;
uint8_t jr,jitterRpt;

void nrk_create_taskset();

int main ()
{
  uint8_t t;
  
  nrk_setup_ports();
  nrk_setup_uart(UART_BAUDRATE_115K2);
  
  DDRF = 0xff;
  DDRB = 0xff;	
  nrk_kprintf( PSTR("Starting up...\r\n") );

  Task2();

  nrk_init();

 
  nrk_time_set(0,0);
  nrk_create_taskset ();
  nrk_start();
  return 0;
}

void my_timer_callback()
{
//	TCNT3 = 0;//Ideally this should be happening automatically, but the count match gets missed sometimes. This may be due to the expected auto-clear of TCNT and a write to the Timer compare - both happening within the timeer ISR
	//nrk_led_toggle(ORANGE_LED);
	//PORTF ^= 0xff;
	//nrk_gpio_toggle(NRK_DEBUG_0);
//	nrk_led_set(ORANGE_LED);	
	if(f_toggle==0)//ON
	{
		PORTF |= 0xff;
		PORTB |= 0xff;
		f_toggle=1;
		switch(freq_f)
		{
			case 1://1k
			compare_vt = 8000;
			break;

			case 2://2k
			compare_vt = 4000;
			break;
	
			case 3://3k
			compare_vt = 2667;
			break;

			case 4://4k
			compare_vt = 2000;
			break;
			
			case 5://5k
			compare_vt = 1600;
			break;
	
			case 6://6k
			compare_vt = 1333;
			break;

			case 7://7k
			compare_vt = 1143;
			break;

			case 8://8k
			compare_vt = 1000;
			break;
		
			case 9://9k
			compare_vt = 889;
			break;

			case 10://10k
			compare_vt = 800;
			break;

			default://10k
			compare_vt = 800;
			break;
		}
	}
	else
	{
		PORTF &= ~0xff;
		PORTB &= ~0xff;
		f_toggle=0;
		switch(freq_f)
		{
			case 1://1k
			compare_vt = 8000;
			break;

			case 2://2k
			compare_vt = 4000;
			break;
	
			case 3://3k
			compare_vt = 2667;
			break;

			case 4://4k
			compare_vt = 2000;
			break;
			
			case 5://5k
			compare_vt = 1600;
			break;
	
			case 6://6k
			compare_vt = 1333;
			break;

			case 7://7k
			compare_vt = 1143;
			break;

			case 8://8k
			compare_vt = 1000;
			break;
		
			case 9://9k
			compare_vt = 889;
			break;

			case 10://10k
			compare_vt = 800;
			break;

			default://10k
			compare_vt = 800;
			break;
		}
	}


//		if(compare_vt<TCNT3) printf( "TCNT3=%d cvt=%d\r\n",TCNT3,compare_vt );
		//Load new compare value to the Timer Compare registers
	OCR3AH = (compare_vt >> 8) & 0xFF;
	OCR3AL = (compare_vt & 0xFF);
//	nrk_led_clr(ORANGE_LED);
}

void Task1()
{
	

  printf( "VLC address is %d\r\n",VLC_ADDR);
  printf( "Task1 PID=%d\r\n",nrk_get_pid());

		
	while(1)
		nrk_wait_until_next_period();
}

uint8_t spin_wait_ms(uint16_t t)
{
	volatile uint16_t x,y;

	// Counter seems to be going twice as fast?
	//y=t*2;
	// set the interrupt to trigger at a high value (i.e. don't let it fire)

	if(t>2) t=t-2;
	TCCR2A = 0; // Start OS timer
	TCCR2B = 0;  // Start OS timer
	TCNT2=0;
	GTCCR |= BM(PSRASY);              // reset prescaler
	GTCCR |= BM(PSRSYNC);
	//TCCR2A = 0; // Start OS timer
	//TCCR2A = BM(WGM21);
	TCCR2B = BM(CS21) | BM(CS20);  // Start OS timer
	//_nrk_os_timer_start();
	// spin on timer value
	while(1){ 
	x=(volatile)TCNT2;
//	printf( "Loop: T2=%d\r\n",TCNT2);
	if(x>t) break;
	 } 
	TCCR2A = 0; 
	TCCR2B = 0;  
	TCNT2=0;
	nrk_spin_wait_us(2000);
	//printf( "%d\r\n",TCNT2);
}

void Task2()
{
	uint16_t cnt,compare_v;
	uint8_t bcnt=0,c;
	uint8_t bit,i,k;
	int8_t val;
	uint8_t tx_val;
	uint8_t Nbits = 8;//10;//8 = 2 preamble bits + 6 data bits
	uint8_t index,ready;
	uint8_t f_off,f_on,f_default;
	uint16_t loop1,loop2,loop3,loop4;
//	uint8_t temp;
//	uint16_t jitter_us,jitterH,jitterL,jitter_ustemp;
//	uint8_t jr,jitterRpt;
	f_default = 10;

	// Timer 0 Setup as Asynchronous timer running from 32Khz Clock
	ASSR = BM(AS2);
	OCR2A = 255; 
	//OCR2B = 2;
	TIFR2 =   BM(OCF2A) | BM(TOV2); //| BM(OCF2B2) ;       // Clear interrupt flag
	TCCR2A = BM(WGM21);

	TCCR2B = BM(CS21) | BM(CS20); //|      // reset counter on interrupt, set divider to 128
	GTCCR |= BM(PSRASY);              // reset prescaler
	// Clear interrupt flag
	TIFR2 =   BM(OCF2A) | BM(TOV2);
	// reset counter on interrupt, set divider to 128
	//TCCR0A = BM(WGM01) | BM(CS01) | BM(CS00);
	// reset prescaler
	//GTCCR |= TSM;              
	GTCCR |= BM(PSRASY);              // reset prescaler
	GTCCR |= BM(PSRSYNC);

	_nrk_os_timer_stop();
	_nrk_os_timer_reset();

    printf( "My node's address is %d\r\n",VLC_ADDR);
	printf( "Task2 PID=%d\r\n",nrk_get_pid());
  	cnt=0;

	if(VLC_ADDR>=TOTAL_NODES) {
	nrk_kprintf( PSTR("VLC address greated than total nodes\r\n"));
	nrk_led_set(RED_LED);	
	while(1);
	}

	nrk_int_enable(); 
	val=nrk_timer_int_configure(NRK_APP_TIMER_0, 1, 800, &my_timer_callback );// this will give a 400HZ timer int
	if(val==NRK_OK) nrk_kprintf( PSTR("Callback timer setup\r\n"));
	else nrk_kprintf( PSTR("Error setting up timer callback\r\n"));

	freq_f = f_default;
	nrk_timer_int_stop(NRK_APP_TIMER_0);
	nrk_timer_int_reset(NRK_APP_TIMER_0);
	TCNT3 = 0;
	
	nrk_timer_int_start(NRK_APP_TIMER_0);

	while(1)
	{
		freq_f = f_default;	
		// stop interrupt and turn off light
	
		nrk_timer_int_stop(NRK_APP_TIMER_0);
		PORTF = 0xff;
		PORTB = 0xff;	

		index=0;
		ready=0;
	//	f_toggle=0;

		// Polling UART receive to avoid timing overhead of using signals (up to 1ms)
		do{
		c = getchar();
		}while(c!='#');
		
//Waits to get #, this is to avoid the loop below getting stuck in case junk is received from serial port initially
		
		do {
			c=getchar();
			if(index==MSG_BUF_SIZE && c=='z' ) 
				ready=1;//changed '\r' tp 'z'
			else 
				msg_buf[index]=c;	
			if(index<MSG_BUF_SIZE) 
				index++;
			else  
			{ 
				index=0; nrk_led_set(RED_LED); 
			}
			/*if(c=='z' && ready==0) 
			{ 
				index=0; nrk_led_set(RED_LED);  
			}*/
		} while(!ready);
		nrk_led_clr(RED_LED);

		// Grab low frequency value
		f_on 		= msg_buf[VLC_ADDR*MSG_SIZE];
		f_off		= msg_buf[VLC_ADDR*MSG_SIZE+1];
		tx_val		= msg_buf[VLC_ADDR*MSG_SIZE+2];
		jtemp 	 	= msg_buf[VLC_ADDR*MSG_SIZE+3];//max value we will give is 50000us, for more than that, repeat the jitter
		jitterRpt 	= (uint16_t)jtemp;
		jtemp 	 	= msg_buf[VLC_ADDR*MSG_SIZE+4];
		jitterH 	= (uint16_t)jtemp;
		jtemp 	 	= msg_buf[VLC_ADDR*MSG_SIZE+5];
		jitterL 	= (uint16_t)jtemp;

		jitter_ustemp 	= (jitterH & 0xff)<<8; 
		jitter_us   = (jitter_ustemp & 0xff00) + (jitterL & 0xff)+1;//Add 1 to avoid the case of sending 0 to spin_wait()

//	printf("Data %d,%d,%d,%d,%d,%d;",msg_buf[VLC_ADDR*MSG_SIZE],msg_buf[VLC_ADDR*MSG_SIZE+1],msg_buf[VLC_ADDR*MSG_SIZE+2],msg_buf[VLC_ADDR*MSG_SIZE+3],msg_buf[VLC_ADDR*MSG_SIZE+4],msg_buf[VLC_ADDR*MSG_SIZE+5]);
//	printf("Jh %d Jl %d  jus %d\r\n",jitterH,jitterL,jitter_us);
//	printf( "* f_on: %d f_off: %d value: %d jitter %d Rpt %d\r\n",f_on,f_off,tx_val,jitter_us,jitterRpt );
/*		while(1)
		{
		nrk_led_toggle(RED_LED);	
		for(jr=0;jr<jitterRpt;jr++)
			nrk_spin_wait_us(50000);//Jitter
		nrk_spin_wait_us(jitter_us);
		}
*/	 	if(msg_buf[0]!=200)	
		{
			for(jr=0;jr<jitterRpt;jr++)
				nrk_spin_wait_us(50000);//Jitter
			nrk_spin_wait_us(jitter_us);
		}
			
		tx_val = tx_val & 0x3f;//6 bits
//		f_on = 3;f_off = 8;
		
		// Lets pad out the first bit
		freq_f = f_off;
		// setup the timer for the first cycle and make sure its cleared to 0
		nrk_timer_int_reset(NRK_APP_TIMER_0);
		TCNT3 = 0;
		//OCR3AH = 0;
		//OCR3AL = 100;
		// Zero the timer...
		// Start the timer...
		nrk_timer_int_start(NRK_APP_TIMER_0);
 //cree
		led_active=1;		

		// It might be safer to call wait_until_next_period() except that it won't be phase-aligned between boards
		// nrk_wait_until_next_period();
		if(msg_buf[0]==200)	//To keep the light ON constant for 30 seconds, just send decimal 200 in the first byte; 
					//Since we are using only values 1-10 for the frequencies, using 200 as an identifier should be fine
		{
			freq_f = f_on; //We want the light ON at constant luminosity. Duty cycle matching has been done. 
					//So the freq does not matter. Just keep it constant
			for(k=0; k<250; k++ ) spin_wait_ms(100);
		}
		else
		{
			spin_wait_ms(150);//For 150ms - send out the OFF freq, so that when then syncs turn ON, all lights are already ON, no matter what the jitter is	
			// Send data...	
			for(bcnt=0; bcnt<Nbits; bcnt++ )
			{	
			
				switch(bcnt)
				{
				case 0:
					// preamble freq f1; 
					freq_f=2;//2
				break;
				
				case 1:
					freq_f=f_on;
				break;
				
				default://Data bits
					//Send f2 of f_inf depending on bit
					bit = (tx_val>>(Nbits-1-bcnt))&0x01;//MSB first
					if(bit==1)
						freq_f = f_on;
					else
						freq_f = f_off;
				}	

				if(bcnt == 0)	//Let sync preamble length remain 1 symbol - so that preamble peak is clear
				{
						spin_wait_ms(33);
				}	
				else		//Vary the symbol length. Set to 33 for symvol length = 1 frame length
				{	
						spin_wait_ms(SYMBOL_LEN);	
				}	
			}
		}
		// After data is done, we can printf the values
		// This can't be before the data or it will impact timing...
//		printf( "* f_on: %d f_off: %d value: %d jitter %d Rpt %d\r\n",f_on,f_off,tx_val,jitter_us,jitterRpt );
	}
}


/*
void Task3()
{
uint16_t cnt;
uint16_t i;
  printf( "Task3 PID=%d\r\n",nrk_get_pid());
  cnt=0;
  while(1) {
	printf( "Task3 cnt=%d\r\n",cnt );
	nrk_wait_until_next_period();
	cnt++;
	}
}*/



void
nrk_create_taskset()
{
/*  TaskOne.task = Task1;
  TaskOne.Ptos = (void *) &Stack1[NRK_APP_STACKSIZE];
  TaskOne.Pbos = (void *) &Stack1[0];
  TaskOne.prio = 1;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 0;
  TaskOne.period.nano_secs = 250*NANOS_PER_MS;//250
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs = 50*NANOS_PER_MS;//50
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs= 0;
  nrk_activate_task (&TaskOne);
*/
  TaskTwo.task = Task2;
  TaskTwo.Ptos = (void *) &Stack2[NRK_APP_STACKSIZE];
  TaskTwo.Pbos = (void *) &Stack2[0];
  TaskTwo.prio = 2;
  TaskTwo.FirstActivation = TRUE;
  TaskTwo.Type = BASIC_TASK;
  TaskTwo.SchType = PREEMPTIVE;
  TaskTwo.period.secs = 0;
  //TaskTwo.period.nano_secs = 0;//*NANOS_PER_MS;
  TaskTwo.period.nano_secs = 0;//67
  // Disable reserves
  TaskTwo.cpu_reserve.secs = 0;
  TaskTwo.cpu_reserve.nano_secs = 0;
  TaskTwo.offset.secs = 0;
  TaskTwo.offset.nano_secs= 0;
  nrk_activate_task (&TaskTwo);
/*

  TaskThree.task = Task3;
  TaskThree.Ptos = (void *) &Stack3[NRK_APP_STACKSIZE];
  TaskThree.Pbos = (void *) &Stack3[0];
  TaskThree.prio = 3;
  TaskThree.FirstActivation = TRUE;
  TaskThree.Type = BASIC_TASK;
  TaskThree.SchType = PREEMPTIVE;
  TaskThree.period.secs = 1;
  TaskThree.period.nano_secs = 0;
  TaskThree.cpu_reserve.secs = 0;
  TaskThree.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TaskThree.offset.secs = 0;
  TaskThree.offset.nano_secs= 0;
  nrk_activate_task (&TaskThree);*/




}

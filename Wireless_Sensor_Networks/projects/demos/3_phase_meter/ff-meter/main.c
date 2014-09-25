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

#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <avr/sleep.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>
#include <basic_rf.h>

#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <ade7878.h>

#define C_CircBuffSize	2016		//Make sure it is a multiple of 12 since we write 12 bytes at a time
#define C_SlipPktSize	96			//Make sure it is a multiple of 12. And safely keep it < 95
#define C_ElementSize 	12

#define C_MeterIden		0x3

NRK_STK Stack1[NRK_APP_STACKSIZE];	
nrk_task_type TaskOne;
void Task1(void);						// To initialize timer

NRK_STK Stack2[NRK_APP_STACKSIZE];
nrk_task_type TaskTwo;
void Task2 (void);					// Slow display of RMS values

NRK_STK Stack3[NRK_APP_STACKSIZE];
nrk_task_type TaskThree;
void Task3 (void);					// Checks if buffer has enough fast data, and then does a SLIP tx

void nrk_create_taskset();

//ADE7878 related
uint8_t F_ChkIntEntry_U8R, V_RdLcycmode_U8R;
uint32_t  V_Status1RdWr_U32R, V_DSPrstCnt_U32R = 0;  
uint16_t V_RdDSP_U16R; 
int32_t V_RdParam1_S32R = 0, V_RdParam2_S32R = 0, V_RdParam3_S32R = 0, V_RdParam4_S32R = 0 ;
uint32_t V_ArmsCurr_U32R, V_ArmsVolt_U32R , V_BrmsCurr_U32R, V_BrmsVolt_U32R;
int32_t V_Awatt_S32R, V_Bwatt_S32R, V_AwattHr_S32R, V_BwattHr_S32R, V_Ang_S32R;
uint16_t V_Period_U16R,  V_Pf_U16R;
uint32_t V_Freq_U32R;
uint32_t V_16to32_U32R;
uint8_t V_DummyCnt_U32R=0,F_1secDataReady_U8R=0,V_1secDummyCnt_U32R=0;
uint32_t V_IncCnt = 0;
uint32_t V_TINTcnt_U32R=0;

//RF realted
void my_callback(uint16_t global_slot );
RF_TX_INFO rfTxInfo;
RF_RX_INFO rfRxInfo;
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t tx_size;

//Buffer Handling related
uint8_t V_TxBuff_U8R[2048];
uint32_t V_WrPtr_U32R = 0, V_RdPtr_U32R = 0;
uint8_t F_AllowRead_U8R = 0, SlowBuff[RF_MAX_PAYLOAD_SIZE], vv =0,SBcnt=0, V_Fpktseq_U8R=0,V_Mpktseq_U8R=0, V_WrSeqNo_U8R = 0;

void StartADE7878(void);

nrk_sem_t *rf_semaphore; //Semaphore for wireless tx

int 
main ()
{
	//Initializing ports
	nrk_setup_ports(); 
   nrk_setup_uart (UART_BAUDRATE_115K2);
	nrk_kprintf( PSTR("Starting up..\r\n") );
	nrk_led_clr(RED_LED);nrk_led_clr(BLUE_LED);nrk_led_clr(ORANGE_LED);nrk_led_clr(GREEN_LED); 
	nrk_led_set(BLUE_LED);
	
//	nrk_init();

	//Initializing RF
	rfRxInfo.pPayload = rx_buf;
   rfRxInfo.max_length = RF_MAX_PAYLOAD_SIZE;
	nrk_int_enable();	 
   rf_init (&rfRxInfo, 13, 0x2420, 0x1214);
	TRX_CTRL_2 |= 0x01;//OQPSK_DATA_RATE0;	
	sprintf(tx_buf,"\r\nRestart");
	rfTxInfo.pPayload=tx_buf;
	rfTxInfo.length= strlen(tx_buf);
	rfTxInfo.destAddr = 0x1215;
	rfTxInfo.cca = 0;
	rfTxInfo.ackRequest = 0;

	if(rf_tx_packet(&rfTxInfo) != 1)
		printf("--- RF_TX ERROR ---\r\n");	

	StartADE7878();

	nrk_init();

	rf_semaphore = nrk_sem_create(1,20);
	if(rf_semaphore==NULL) nrk_kprintf( PSTR("Error creating sem\r\n"));
  	
	nrk_time_set(0,0);
	nrk_create_taskset ();
	
	nrk_start();
  	return 0;
}

//This timer interrupt occurs every 1ms. We read the Watt, VAR registers and write to buffer
void my_timer_callback()
{
	nrk_led_toggle(GREEN_LED);
	V_RdParam1_S32R = ade_read32(IAWV);
	V_RdParam2_S32R = ade_read32(VAWV);
	V_RdParam3_S32R = ade_read32(IBWV);
	V_RdParam4_S32R = ade_read32(VBWV);

/*	V_RdParam1_S32R = V_IncCnt++;
	if(V_IncCnt==0xFFFFFF) V_IncCnt = 0;*/ //Uncomment for filling first param with incremental numbers

	//Write the 12 new bytes to the buffer and increment Wr pointer
	memcpy(&V_TxBuff_U8R[V_WrPtr_U32R+0],&V_RdParam1_S32R,3);
	memcpy(&V_TxBuff_U8R[V_WrPtr_U32R+3],&V_RdParam2_S32R,3);
	memcpy(&V_TxBuff_U8R[V_WrPtr_U32R+6],&V_RdParam3_S32R,3);
	memcpy(&V_TxBuff_U8R[V_WrPtr_U32R+9],&V_RdParam4_S32R,3);
	V_WrPtr_U32R+=C_ElementSize;
	
	//If Wr Ptr reached end-of-buffer, set a flag to let the Rd pointer know
	if(V_WrPtr_U32R == C_CircBuffSize)
	{	
		V_WrPtr_U32R = 0;
		V_WrSeqNo_U8R = 1;
	}

	//Check if there is enough data to be read and set flag
	if(((V_WrSeqNo_U8R*C_CircBuffSize + V_WrPtr_U32R)-(V_RdPtr_U32R)) >= 2*C_SlipPktSize)//Ensuring that Rd is always more than 1 SLIPPkt behind Wr
	{
		F_AllowRead_U8R = 1;
	}
	
	//Every 1 second, we read RMS values. This is done inside the ISR and not as a separate task because both - the ISR and the lines below access the 7878 through SPI and we cant let one SPI instr interrupt another SPI instr
	//Cannot afford to read all these registers (which are needed for 1Hz data) in a single 1ms interrupt, so reading it over several interrupts
	V_TINTcnt_U32R++;
	switch(V_TINTcnt_U32R)
	{
		case 1:
			V_ArmsCurr_U32R = ade_read32(AIRMS);
			SBcnt=0;
			SlowBuff[SBcnt] = 'i';
			memcpy(&SlowBuff[SBcnt+1],&V_ArmsCurr_U32R,4);SBcnt=SBcnt+5;
			break;

		case 2:
			V_ArmsVolt_U32R = ade_read32(AVRMS);
			SlowBuff[SBcnt] = 'v';
			memcpy(&SlowBuff[SBcnt+1],&V_ArmsVolt_U32R,4);SBcnt=SBcnt+5;
			break;

		case 3:
			V_BrmsCurr_U32R = ade_read32(BIRMS);
			SlowBuff[SBcnt] = 'i';
			memcpy(&SlowBuff[SBcnt+1],&V_BrmsCurr_U32R,4);SBcnt=SBcnt+5;
			break;

		case 4:
			V_BrmsVolt_U32R = ade_read32(BVRMS);
			SlowBuff[SBcnt] = 'v';
			memcpy(&SlowBuff[SBcnt+1],&V_BrmsVolt_U32R,4);SBcnt=SBcnt+5;
		break;

		case 5:
			V_Period_U16R	= ade_read16(PERIOD);
			SlowBuff[SBcnt] = 'f';
			V_16to32_U32R = V_Period_U16R;
			memcpy(&SlowBuff[SBcnt+1],&V_16to32_U32R,4);SBcnt=SBcnt+5;
		break;

		case 6:
			V_Pf_U16R		= ade_read16(ANGLE0);
			SlowBuff[SBcnt] = 'a';
			V_16to32_U32R = V_Pf_U16R;
			memcpy(&SlowBuff[SBcnt+1],&V_16to32_U32R,4);SBcnt=SBcnt+5;
		break;

		case 7:
			V_Awatt_S32R	= ade_read32(AWATT);
			SlowBuff[SBcnt] = 'w';
			memcpy(&SlowBuff[SBcnt+1],&V_Awatt_S32R,4);SBcnt=SBcnt+5;
		break;

		case 8:
			V_Bwatt_S32R	= ade_read32(BWATT);
			SlowBuff[SBcnt] = 'w';
			memcpy(&SlowBuff[SBcnt+1],&V_Bwatt_S32R,4);SBcnt=SBcnt+5;
		break;

		case 9:
			V_AwattHr_S32R	= ade_read32(AWATTHR);
			SlowBuff[SBcnt] = 'e';
			memcpy(&SlowBuff[SBcnt+1],&V_AwattHr_S32R,4);SBcnt=SBcnt+5;
		break;

		case 10:
			V_BwattHr_S32R	= ade_read32(BWATTHR);
			SlowBuff[SBcnt] = 'e';
			memcpy(&SlowBuff[SBcnt+1],&V_BwattHr_S32R,4); SBcnt=SBcnt+5;
			F_1secDataReady_U8R=1;
		break;

		case 11:
			V_RdDSP_U16R = ade_read16(RUN);
			if(V_RdDSP_U16R == 0)
			{
				V_DSPrstCnt_U32R++;
				ade_write16(RUN,START);//if the DSP got reset, start it so that it continues to run
			}
		break;
	
		case 1000:
			V_TINTcnt_U32R = 0;
		break;

		default:
			break;
	}
}

//This task initializes the Timer
void Task1()
{
	uint16_t cnt;
	uint8_t val;

	printf( "My node's address is %d\r\n",NODE_ADDR );

  	printf( "Task1 PID=%d\r\n",nrk_get_pid());
  	cnt=0;

  	// Setup application timer with:
  	//       Prescaler = 2 
  	//       Compare Match = 2000
  	//       Sys Clock = 16000 MHz
  	// Prescaler 2 means divide sys clock by 8
  	// 16000000 / 8 = 2MHz clock
  	// 1 / 2MHz = 0.5 us per tick
  	// 0.5 us * 2000 = 1 ms / per interrupt callback

  	val=nrk_timer_int_configure(NRK_APP_TIMER_0, 2, 2000, &my_timer_callback );//value 2000 is for 1ms. Change to 16000 for 8ms
 	// val=nrk_timer_int_configure(NRK_APP_TIMER_0, 2, 8000, &my_timer_callback );//value 2000 is for 1ms. Change to 16000 for 8ms
  	if(val==NRK_OK) nrk_kprintf( PSTR("Callback timer setup\r\n"));
  	else nrk_kprintf( PSTR("Error setting up timer callback\r\n"));

  	nrk_timer_int_reset(NRK_APP_TIMER_0);//Zero the timer
  	nrk_timer_int_start(NRK_APP_TIMER_0);//Start the timer

  	while(1) 
  	{
		nrk_wait_until_next_period();
	}
}

// Slow display of RMS values - every 1 second
// Has been disabled
void Task2()
{
	uint8_t cnt, v_BufLen_u8r=0, v;
	printf( "Task2 PID=%d\r\n",nrk_get_pid());
	cnt=0;
  
	while(1) 
	{
		//Holding Semaphore
		v = nrk_sem_pend(rf_semaphore);
		if(v==NRK_ERROR) nrk_kprintf( PSTR("T1 error pend\r\n"));
//		nrk_kprintf( PSTR("T1 h\r\n"));

		if(F_1secDataReady_U8R==1)	//Check is the RMS data is available from the 1ms second timer
		{
			F_1secDataReady_U8R=0;	//Clear flag to avoid repeat of same data
			//Display data on PC connected to meter
	/*		printf("\r\nIA=%ld  ",V_ArmsCurr_U32R);
			printf("VA=%ld ",V_ArmsVolt_U32R);
			printf("IB=%ld  ",V_BrmsCurr_U32R);
			printf("VB=%ld",V_BrmsVolt_U32R);
	*/		
			tx_buf[0] = 'M';tx_buf[1] = C_MeterIden; tx_buf[2] = V_Mpktseq_U8R;
			memcpy(&tx_buf[3],&SlowBuff,SBcnt);
			v_BufLen_u8r = SBcnt+3;
				
			//Transmit the data through RF
			V_Mpktseq_U8R++;
	
			rfTxInfo.pPayload=tx_buf;
			rfTxInfo.length= v_BufLen_u8r;
			rfTxInfo.destAddr = 0x1215;
			rfTxInfo.cca = 0;
			rfTxInfo.ackRequest = 0;
			if(rf_tx_packet(&rfTxInfo) != 1)
				printf("--- RF_TX ERROR ---\r\n");
			cnt++;			
		}
		//Releasing Semaphore
		v = nrk_sem_post(rf_semaphore);
		if(v==NRK_ERROR) nrk_kprintf( PSTR("T1 error post\r\n"));
	//	nrk_kprintf( PSTR("T1 r\r\n"));

		nrk_wait_until_next_period();
		//cnt++;
	}
}
	
//Checks if buffer has enough fast data, and then does a rf tx
void Task3()
{
	uint8_t cnt,v_BufLen_u8r=0,v,icnt=0;

	printf( "Task3 PID=%d\r\n",nrk_get_pid());
	cnt=0;

	while(1) 
	{
		if(F_AllowRead_U8R==1)		//If there is enough data to do a SLIP tx. This flag is set by the 1ms timer interrupt
		{
			nrk_led_clr(RED_LED);
			F_AllowRead_U8R = 0; 	//Clear flag to avoid repeating same data
			tx_buf[0] = 'F';tx_buf[1] = C_MeterIden; tx_buf[2] = V_Fpktseq_U8R;
			memcpy(&tx_buf[3],&V_TxBuff_U8R[V_RdPtr_U32R],C_SlipPktSize);
			v_BufLen_u8r = C_SlipPktSize+3;
			V_RdPtr_U32R=V_RdPtr_U32R+C_SlipPktSize;	//Now that we just read data (in the slip_tx func), increment the Rd ptr
			if(V_RdPtr_U32R>=C_CircBuffSize)				//If end-of-buffer reached, roll over the pointer
			{
				V_RdPtr_U32R = V_RdPtr_U32R - C_CircBuffSize;
				V_WrSeqNo_U8R = 0;							//This ptr is set when Wr rolls over and Rd still not rolled over. Now that read too rolled over, clr this flag
			}

			for(icnt=0;icnt<1;icnt++)
			{
				//Holding semaphore
	//			v = nrk_sem_pend(rf_semaphore);
	//			if(v==NRK_ERROR) nrk_kprintf( PSTR("T2 error pend\r\n"));
			
				//Transmit the data 
				rfTxInfo.pPayload=tx_buf;
				rfTxInfo.length= v_BufLen_u8r;
				rfTxInfo.destAddr = 0x1215;
				rfTxInfo.cca = 0;
				rfTxInfo.ackRequest = 0;
				if(rf_tx_packet(&rfTxInfo) != 1)
					printf("--- RF_TX ERROR ---\r\n");
				nrk_led_set(RED_LED);

				//Releasing Semaphore
	//			v = nrk_sem_post(rf_semaphore);
	//			if(v==NRK_ERROR) nrk_kprintf( PSTR("T2 error post\r\n"));
			}
			V_Fpktseq_U8R++;
		}
		
		if(F_1secDataReady_U8R==1)	//Check is the RMS data is available from the 1ms second timer
		{
			nrk_led_toggle(BLUE_LED);
	//		v = nrk_sem_pend(rf_semaphore);
	///		if(v==NRK_ERROR) nrk_kprintf( PSTR("T1 error pend\r\n"));
			
			F_1secDataReady_U8R=0;	//Clear flag to avoid repeat of same data
	
			tx_buf[0] = 'M';tx_buf[1] = C_MeterIden; tx_buf[2] = V_Mpktseq_U8R;
			memcpy(&tx_buf[3],&SlowBuff,SBcnt);
			v_BufLen_u8r = SBcnt+3;
				
			//Transmit the data through RF
			V_Mpktseq_U8R++;
	
			rfTxInfo.pPayload=tx_buf;
			rfTxInfo.length= v_BufLen_u8r;
			rfTxInfo.destAddr = 0x1215;
			rfTxInfo.cca = 0;
			rfTxInfo.ackRequest = 0;
			if(rf_tx_packet(&rfTxInfo) != 1)
				printf("--- RF_TX ERROR ---\r\n");
			cnt++;			
	
			//Releasing Semaphore
	//		v = nrk_sem_post(rf_semaphore);
	//		if(v==NRK_ERROR) nrk_kprintf( PSTR("T2 error post\r\n"));
		}

		nrk_wait_until_next_period();
	}
}
//This external interrupt is sent by the 7878 when we reset it
ISR(INT1_vect)
{
	uint8_t i;

	EIMSK = 0x00;
	//nrk_led_set(RED_LED);
	//Set MOSI and SCK as output, others as input
	DDRB|=  _BV(PORTB0) | _BV(PORTB1) | _BV(PORTB2);
	DDRB&=  ~(_BV(PORTB3));
	PORTB|= _BV(PORTB0) | _BV(PORTB1) | _BV(PORTB2);
	
	for(i=0; i<20; i++)
		PORTB ^= _BV(PORTB0);	//We need to toggle this multiple times (>3) to set the SPI mode of communication

	F_ChkIntEntry_U8R = 1;
}

void
nrk_create_taskset()
{
  TaskOne.task = Task1;					// To initialize timer
  TaskOne.Ptos = (void *) &Stack1[NRK_APP_STACKSIZE];
  TaskOne.Pbos = (void *) &Stack1[0];
  TaskOne.prio = 1;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 0;
  TaskOne.period.nano_secs = 250*NANOS_PER_MS;
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs =  50*NANOS_PER_MS;
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs= 0;
  nrk_activate_task (&TaskOne);

  TaskTwo.task = Task2;					// Slow display of RMS values
  TaskTwo.Ptos = (void *) &Stack2[NRK_APP_STACKSIZE];
  TaskTwo.Pbos = (void *) &Stack2[0];
  TaskTwo.prio = 2;
  TaskTwo.FirstActivation = TRUE;
  TaskTwo.Type = BASIC_TASK;
  TaskTwo.SchType = PREEMPTIVE;
  TaskTwo.period.secs = 1;
  TaskTwo.period.nano_secs = 0;//100*NANOS_PER_MS;
  TaskTwo.cpu_reserve.secs = 0;
  TaskTwo.cpu_reserve.nano_secs = 50*NANOS_PER_MS;
  TaskTwo.offset.secs = 0;
  TaskTwo.offset.nano_secs= 0;
//  nrk_activate_task (&TaskTwo);

  TaskThree.task = Task3;					// Checks if buffer has enough fast data, and then does a SLIP tx
  TaskThree.Ptos = (void *) &Stack3[NRK_APP_STACKSIZE];
  TaskThree.Pbos = (void *) &Stack3[0];
  TaskThree.prio = 4;
  TaskThree.FirstActivation = TRUE;
  TaskThree.Type = BASIC_TASK;
  TaskThree.SchType = PREEMPTIVE;
  TaskThree.period.secs = 0;
  TaskThree.period.nano_secs = 8*NANOS_PER_MS;
  //TaskThree.period.nano_secs = 15*NANOS_PER_MS;
  TaskThree.cpu_reserve.secs = 0;
 // TaskThree.cpu_reserve.nano_secs = 8*NANOS_PER_MS;
  //TaskThree.cpu_reserve.nano_secs = 15*NANOS_PER_MS;
  TaskThree.offset.secs = 0;
  TaskThree.offset.nano_secs= 0;
  nrk_activate_task (&TaskThree);
}

void my_callback(uint16_t global_slot )
{
	static uint16_t cnt;

	printf( "callback %d %d\n",global_slot,cnt );
	cnt++;
}


// This is a callback if you require immediate response to a packet
RF_RX_INFO *rf_rx_callback (RF_RX_INFO * pRRI)
{
    // Any code here gets called the instant a packet is received from the interrupt   
    return pRRI;
}

void StartADE7878(void)
{
	DDRD |= _BV(PORTD3);		//ADE7878 Reset line 

	//Interrupt initialization for ADE7878
	sei();						//Enable Global interrupts
	DDRD &= ~(_BV(PORTD1));	//Making INT1 pin as input
	EIMSK = 0x00;				//Mask all
	EICRA = 0x08;				//EINT1 Falling Edge - changed for new FF

	PORTD |= _BV(PORTD3);	//Reset ADE7878
	//Set ADE in normal power mode PM0=1 (PE2), PM1=0 (PE3)
	PORTD &= ~_BV(PORTD3);
	//Set ADE in normal power mode PM0=1 (PE2), PM1=0 (PE3)
	DDRE 	|= _BV(PORTE2) | _BV(PORTE3);
	PORTE |= _BV(PORTE2);
	PORTE &= ~(_BV(PORTE3));

	//These bunch of lines below may not be needed since they have- added it while debugging a recent issue, did not try the code without these lines
	EIMSK = 0x02;	//Unmask EINT1
	PORTD |= _BV(PORTD3);
	PORTE |= _BV(PORTE2);
	PORTE &= ~(_BV(PORTE3));

	//Check if the interrupt really executed or not
	while(EIMSK !=0);

	//Enable SPI Master, Set Clock rate fck/4
	SPCR = 0x01;
	SPCR |= _BV(SPE) | _BV(MSTR) | _BV(CPOL) | _BV(CPHA);

	PORTB |= _BV(PORTB0);//default state of SS should be low

	//This segment below is added because it was observed that the 7878
	//SPI or the ATMEL SPI takes some time to start. So waiting till
	//some register gives its default value
	printf("Wait..");
	V_RdLcycmode_U8R=0;
	while(V_RdLcycmode_U8R!=0x78)
		V_RdLcycmode_U8R = ade_read8(LCYCMODE);
	printf("\r\n7878 Ready %x", V_RdLcycmode_U8R);

	//Initialize ADE7878 DSP and a few other registers
	if(ade_init() < 0)
		printf("\nInit failed");
	else
		printf("\nInit Success");

	//Read of this register - just for checking
	V_RdLcycmode_U8R = ade_read8(LCYCMODE);
	printf("\r\nLCYC =  %x", V_RdLcycmode_U8R);

	//Read the INT Status register to clear the interrupt
	V_Status1RdWr_U32R = ade_read32(STATUS1);
	ade_write32(STATUS1, V_Status1RdWr_U32R);
}

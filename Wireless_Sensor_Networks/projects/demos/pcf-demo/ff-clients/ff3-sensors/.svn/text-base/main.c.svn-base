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
#include <pcf_tdma.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_eeprom.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include <nrk_ext_int.h>
#include <ff_basic_sensor.h>
#include <pkt.h>


// if SET_MAC is 0, then read MAC from EEPROM
// otherwise use the coded value
#define SET_MAC  	0x0	
#define CHAN		26	

//#define DISABLE_SLEEP
//#define DISABLE_BUTTON


#define EEPROM_SLEEP_STATE_ADDR		0x100


PKT_T	tx_pkt;
PKT_T	rx_pkt;
uint8_t gpio_pin;
uint8_t send_ack;

uint8_t sbuf[4];
tdma_info tx_tdma_fd;
tdma_info rx_tdma_fd;

uint8_t rx_buf[TDMA_MAX_PKT_SIZE];
uint8_t tx_buf[TDMA_MAX_PKT_SIZE];

uint32_t mac_address;


nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);


nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

void nrk_create_taskset ();
void nrk_register_drivers();


void wakeup_func();
void deep_sleep_button();

static nrk_time_t tmp_time;
static int8_t snoozing=0;

uint8_t aes_key[] = {0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee, 0xff};


// This function will put the node into a low-duty checking mode to save power
void tdma_snooze()
{
int8_t v;
uint8_t i;

#ifdef DISABLE_SLEEP
return;
#endif
// stop the software watchdog timer so it doesn't interrupt
nrk_sw_wdt_stop(0);
// This flag is cleared only by the button interrupt
snoozing=1;
// Setup the button interrupt
#ifndef DISABLE_BUTTON
nrk_ext_int_configure( NRK_EXT_INT_1,NRK_LEVEL_TRIGGER, &wakeup_func);
// Clear it so it doesn't fire instantly
EIFR=0xff;
nrk_ext_int_enable( NRK_EXT_INT_1);
#endif

// Now loop and periodically check for packets
while(1)
{
nrk_led_clr(RED_LED);	
nrk_led_clr(GREEN_LED);	
	rf_power_up();
	rf_rx_on ();
// check return from button interrupt on next cycle
if(snoozing==0 ) return;
tmp_time.secs=0;     
tmp_time.nano_secs=10*NANOS_PER_MS;     
        nrk_led_set(RED_LED);
        // Leave radio on for two loops of 10ms for a total of 20ms every 10 seconds
	for(i=0; i<2; i++ )
	{
          v = _tdma_rx ();
	  if(v==NRK_OK) { 
		nrk_led_set(RED_LED);  
		nrk_ext_int_disable( NRK_EXT_INT_1);
   		nrk_sw_wdt_update(0);
   		nrk_sw_wdt_start(0);
		return NRK_OK;
	  }
	  nrk_wait(tmp_time);
	}
nrk_led_clr(RED_LED);
rf_power_down();
tmp_time.secs=10;     
tmp_time.nano_secs=0;     
nrk_sw_wdt_stop(0);
nrk_wait(tmp_time);

}

}



// This function gets called in a loop if sync is lost.
// It is passed a counter indicating how long it has gone since the last synchronization.
int8_t tdma_error(uint16_t cons_err_cnt)
{

  if(tdma_sync_ok()==0 && cons_err_cnt>50 )  nrk_led_set(RED_LED);
  else nrk_led_clr(RED_LED);

  // If there has been enough cycles without sync then snooze  
  if(cons_err_cnt>400) {
	//nrk_kprintf(PSTR("Entering TDMA snooze...\r\n" ));
	nrk_wait_until_next_period();
	tdma_snooze();
	return NRK_OK; 
	}

return NRK_ERROR;
}

int main ()
{
  uint8_t ds;
  nrk_setup_ports ();
  nrk_setup_uart (UART_BAUDRATE_115K2);


#ifndef DISABLE_BUTTON
  ds=nrk_eeprom_read_byte(EEPROM_SLEEP_STATE_ADDR);
  if(ds==1) deep_sleep_button();
#endif

  nrk_init ();

  nrk_led_clr (0);
  nrk_led_clr (1);
  nrk_led_clr (2);
  nrk_led_clr (3);

  nrk_time_set (0, 0);

  tdma_set_error_callback(&tdma_error);
  tdma_task_config();

  nrk_register_drivers();
  nrk_create_taskset ();
  nrk_start ();

  return 0;
}


void rx_task ()
{
  nrk_time_t t;
  uint16_t cnt;
  int8_t v;
  uint8_t len, i;
uint8_t chan;


  cnt = 0;
  nrk_kprintf (PSTR ("Nano-RK Version "));
  printf ("%d\r\n", NRK_VERSION);
 

  printf ("RX Task PID=%u\r\n", nrk_get_pid ());
  t.secs = 5;
  t.nano_secs = 0;

  chan = CHAN;
  if (SET_MAC == 0x00) {

    v = read_eeprom_mac_address (&mac_address);
    if (v == NRK_OK) {
      v = read_eeprom_channel (&chan);
      v=read_eeprom_aes_key(aes_key);
    }
    else {
      while (1) {
        nrk_kprintf (PSTR
                     ("* ERROR reading MAC address, run eeprom-set utility\r\n"));
        nrk_wait_until_next_period ();
      }
    }
  }
  else
    mac_address = SET_MAC;

  printf ("MAC ADDR: %x\r\n", mac_address & 0xffff);
  printf ("chan = %d\r\n", chan);
  len=0;
  for(i=0; i<16; i++ ) { len+=aes_key[i]; }
  printf ("AES checksum = %d\r\n", len);



  tdma_init (TDMA_CLIENT, chan, mac_address);

  tdma_aes_setkey(aes_key);
  tdma_aes_enable();


  while (!tdma_started ())
    nrk_wait_until_next_period ();

  // Mask off lower byte of MAC address for TDMA slot
  // FIXME: This should eventually be lower 2 bytes for larger TDMA cycles
  v = tdma_tx_slot_add (mac_address&0xFFFF);

  if (v != NRK_OK)
    nrk_kprintf (PSTR ("Could not add slot!\r\n"));

  while (1) {
    // Update watchdog timer
    // nrk_sw_wdt_update(0);
    v = tdma_recv (&rx_tdma_fd, &rx_buf, &len, TDMA_BLOCKING);
    if (v == NRK_OK) {
     // printf ("src: %u\r\nrssi: %d\r\n", rx_tdma_fd.src, rx_tdma_fd.rssi);
     // printf ("slot: %u\r\n", rx_tdma_fd.slot);
     // printf ("cycle len: %u\r\n", rx_tdma_fd.cycle_size);
      v=buf_to_pkt(&rx_buf, &rx_pkt);
/*
      printf("ActualRssi:  %d, energyDetectionLevel:  %d, linkQualityIndication:  %d\r\n",
             rx_tdma_fd.actualRssi,
             rx_tdma_fd.energyDetectionLevel,
             rx_tdma_fd.linkQualityIndication);
      printf ("raw len: %u\r\nraw buf: ", len);
      for (i = 0; i < len; i++)
        printf ("%d ", rx_buf[i]);
      printf ("\r\n");
*/
      if(rx_pkt.type==PING && (((rx_pkt.dst_mac&0xff) == (mac_address&0xff)) || ((rx_pkt.dst_mac&0xff)==0xff))) 
	{
		send_ack=1;
		nrk_led_clr(RED_LED);
		nrk_led_clr(GREEN_LED);
	if(rx_pkt.payload[0]==PING_1)
		{
		nrk_led_set(GREEN_LED);
		nrk_wait_until_next_period();
		nrk_wait_until_next_period();
		nrk_wait_until_next_period();
		nrk_led_clr(GREEN_LED);
		}
	if(rx_pkt.payload[0]==PING_2)
		{
		nrk_led_set(RED_LED);
		nrk_wait_until_next_period();
		nrk_wait_until_next_period();
		nrk_wait_until_next_period();
		nrk_led_clr(RED_LED);
		}
	if(rx_pkt.payload[0]==PING_PERSIST)
		{
		nrk_led_set(GREEN_LED);
		}
	

	}
      if(rx_pkt.type==NW_CONFIG && (((rx_pkt.dst_mac&0xff) == (mac_address&0xff)) || (rx_pkt.dst_mac==0xff))) 
      {

/*      printf ("payload len: %u\r\npayload: ", rx_pkt.payload_len);
      for (i = 0; i < rx_pkt.payload_len; i++)
        printf ("%d ", rx_pkt.payload[i]);
      printf ("\r\n");
*/
	// Sleep control packet
     if(rx_pkt.payload[1]==NW_CONFIG_SLEEP)
	{
	// sleep mode off
	if(rx_pkt.payload[2]==0 )
		{
		// Enter Deep Sleep
		deep_sleep_button();

		}
	if(rx_pkt.payload[2]==2 )
		{
		// Enter snooze mode
		tdma_disable(); 
		nrk_wait_until_next_period();
		tdma_snooze();
		tdma_enable(); 

		}


      	}

	}

    }

    //  nrk_wait_until_next_period();
  }

}

void tx_task ()
{
  uint8_t j, i, val, cnt;
  int8_t len;
  int8_t v,fd;
  nrk_sig_mask_t ret;
  nrk_time_t t;



  // Set Port D.0 as input
  // On the FireFly nodes we use this for motion or syntonistor input
  // It can be interrupt driven, but we don't do this with TDMA since updates are so fast anyway
//  DDRD &= ~(0x1);



  printf ("tx_task PID=%d\r\n", nrk_get_pid ());

   // setup a software watch dog timer
   t.secs=10;
   t.nano_secs=0;
   nrk_sw_wdt_init(0, &t, NULL);
   nrk_sw_wdt_start(0);

  while (!tdma_started ())
    nrk_wait_until_next_period ();

  // set port G as input for gpio
  //DDRG=0; 
  nrk_gpio_direction(NRK_PORTG_0,NRK_PIN_INPUT );

  send_ack=0;

  while (1) {
	nrk_led_clr(RED_LED);

	if(nrk_gpio_get(NRK_BUTTON)==0 )
	{
	nrk_led_set(GREEN_LED);
	do {
	nrk_sw_wdt_update(0);  
	nrk_wait_until_next_period();
	} while(nrk_gpio_get(NRK_BUTTON)==0);
	nrk_led_clr(GREEN_LED);

	}

    	tx_pkt.payload[0] = 1;  // ELEMENTS
   	 tx_pkt.payload[1] = 3;  // Key

    	fd=nrk_open(FIREFLY_3_SENSOR_BASIC,READ);
        if(fd==NRK_ERROR) nrk_kprintf(PSTR("Failed to open sensor driver\r\n"));


	val=nrk_set_status(fd,SENSOR_SELECT,MOTION);
	val=nrk_read(fd,&sbuf,2);
	tx_pkt.payload[28]=sbuf[1];
	tx_pkt.payload[29]=sbuf[0];



	val=nrk_set_status(fd,SENSOR_SELECT,BAT);
	val=nrk_read(fd,&sbuf,2);
	tx_pkt.payload[2]=sbuf[1];
	tx_pkt.payload[3]=sbuf[0];
	//printf( "Task bat=%d",sbuf);

//	tx_pkt.payload[2]=0;
//	tx_pkt.payload[3]=0;
	
	val=nrk_set_status(fd,SENSOR_SELECT,LIGHT);
	val=nrk_read(fd,&sbuf,2);
	tx_pkt.payload[4]=sbuf[1];
	tx_pkt.payload[5]=sbuf[0];
	//printf( " light=%d",sbuf);
	val=nrk_set_status(fd,SENSOR_SELECT,TEMP);
	val=nrk_read(fd,&sbuf,2);
	tx_pkt.payload[6]=sbuf[1];
	tx_pkt.payload[7]=sbuf[0];

	
	val=nrk_set_status(fd,SENSOR_SELECT,TEMP2);
	val=nrk_read(fd,&sbuf,4);
	tx_pkt.payload[20]=sbuf[3];
	tx_pkt.payload[21]=sbuf[2];
	tx_pkt.payload[22]=sbuf[1];
	tx_pkt.payload[23]=sbuf[0];


	val=nrk_set_status(fd,SENSOR_SELECT,PRESS);
	val=nrk_read(fd,&sbuf,4);
	tx_pkt.payload[24]=sbuf[3];
	tx_pkt.payload[25]=sbuf[2];
	tx_pkt.payload[26]=sbuf[1];
	tx_pkt.payload[27]=sbuf[0];
	//printf( " temp=%d",sbuf);
	
	
	val=nrk_set_status(fd,SENSOR_SELECT,ACC_X);
	val=nrk_read(fd,&sbuf,2);
	tx_pkt.payload[8]=sbuf[1];
	tx_pkt.payload[9]=sbuf[0];
	//printf( " acc_x=%d",sbuf);
	val=nrk_set_status(fd,SENSOR_SELECT,ACC_Y);
	val=nrk_read(fd,&sbuf,2);
	tx_pkt.payload[10]=sbuf[1];
	tx_pkt.payload[11]=sbuf[0];
	//printf( " acc_y=%d",sbuf);
	val=nrk_set_status(fd,SENSOR_SELECT,ACC_Z);
	val=nrk_read(fd,&sbuf,2);
	tx_pkt.payload[12]=sbuf[1];
	tx_pkt.payload[13]=sbuf[0];
	//printf( " acc_z=%d",sbuf);


        val=nrk_set_status(fd,SENSOR_SELECT,AUDIO_P2P);
	nrk_wait_until_ticks(60);	
	val=nrk_read(fd,&sbuf,2);
	tx_pkt.payload[14]=sbuf[1];
	tx_pkt.payload[15]=sbuf[0];

	// This needs to be last to let the sensor settle.	
	val=nrk_set_status(fd,SENSOR_SELECT,HUMIDITY);
	val=nrk_read(fd,&sbuf,4);
	tx_pkt.payload[16]=sbuf[3];
	tx_pkt.payload[17]=sbuf[2];
	tx_pkt.payload[18]=sbuf[1];
	tx_pkt.payload[19]=sbuf[0];


	// GPIO data
//	gpio_pin = !!(PINE & 0x4);
	//gpio_pin = PING & 0x1;
	gpio_pin = nrk_gpio_get(NRK_PORTG_0);
	tx_pkt.payload[30]=gpio_pin;
	//printf( " audio=%d\r\n",sbuf);
// Temporarily roll back Mike's updates... Add back later...
//        tx_pkt.payload[31] = rx_tdma_fd.actualRssi;
//        tx_pkt.payload[32] = rx_tdma_fd.energyDetectionLevel;
//        tx_pkt.payload[33] = rx_tdma_fd.linkQualityIndication;
//        tx_pkt.payload[34] = 0; // Reserved for the PCF host's actualRssi
//        tx_pkt.payload[35] = 0; // Reserved for the PCF host's energyDetectionLevel
//        tx_pkt.payload[36] = 0; // Reserved for the PCF host's linkQualityIndication
    	nrk_close(fd);

    	tx_pkt.payload_len=31;
    	tx_pkt.src_mac=mac_address;
    	tx_pkt.dst_mac=0;
    	tx_pkt.type=APP;

	if(send_ack==1)
	{
	tx_pkt.type=ACK;
	tx_pkt.payload_len=0;
	send_ack=0;
	}


    len=pkt_to_buf(&tx_pkt,&tx_buf );

if(len>0)
    {
    v = tdma_send (&tx_tdma_fd, &tx_buf, len, TDMA_BLOCKING);
    if (v == NRK_OK) {
    }
    } else { nrk_kprintf(PSTR("Pkt tx error\r\n")); nrk_wait_until_next_period(); }
   

	nrk_sw_wdt_update(0);  
  }



}



void wakeup_func()
{
if(snoozing==1) { nrk_led_set(RED_LED); snoozing=0; }

}


void deep_sleep_button()
{
int i,cnt;
		nrk_int_disable();
  		nrk_eeprom_write_byte(EEPROM_SLEEP_STATE_ADDR,1);
		nrk_led_set(GREEN_LED);
		nrk_spin_wait_us(50000);
		nrk_led_clr(0);
		nrk_led_clr(1);
		nrk_led_clr(2);
		nrk_led_clr(3);
		nrk_watchdog_disable();
		nrk_int_disable();
		_nrk_os_timer_stop();	
		nrk_gpio_direction(NRK_BUTTON, NRK_PIN_INPUT);
		rf_power_down();
		// Enable Pullup for button	
		//PORTD = 0xff;
		nrk_gpio_set(NRK_BUTTON);
		nrk_ext_int_configure( NRK_EXT_INT_1,NRK_LEVEL_TRIGGER, &wakeup_func);
		EIFR=0xff;
		nrk_ext_int_enable( NRK_EXT_INT_1);
		nrk_int_enable();
		DDRA=0x0;	
		DDRB=0x0;	
		DDRC=0x0;	
		ASSR=0;
		TRXPR = 1 << SLPTR;
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sleep_cpu();
		// reboot
		while(1) {
		//printf( "awake!\r\n" ); 
		nrk_led_clr(0);
		nrk_led_clr(1);
		nrk_led_set(1);
		cnt=0;
		for(i=0; i<100; i++ )
			{
			//if((PIND & 0x2)==0x2)cnt++;
			if(nrk_gpio_get(NRK_BUTTON)==0)cnt++;
			nrk_spin_wait_us(10000);
			}
		printf( "cnt=%d\n",cnt );
		if(cnt>=50 ) {
		// reboot
		nrk_led_clr(RED_LED);
		nrk_led_set(GREEN_LED);
  		nrk_eeprom_write_byte(EEPROM_SLEEP_STATE_ADDR,0);
		nrk_spin_wait_us(50000);
		nrk_spin_wait_us(50000);
		nrk_watchdog_enable();
		while(1);

		}
		nrk_led_clr(GREEN_LED);
		nrk_led_clr(RED_LED);
		TRXPR = 1 << SLPTR;
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sleep_cpu();
		}
}








void nrk_create_taskset ()
{


  RX_TASK.task = rx_task;
  nrk_task_set_stk (&RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
  RX_TASK.prio = 2;
  RX_TASK.FirstActivation = TRUE;
  RX_TASK.Type = BASIC_TASK;
  RX_TASK.SchType = PREEMPTIVE;
  RX_TASK.period.secs = 1;
  RX_TASK.period.nano_secs = 0;
  RX_TASK.cpu_reserve.secs = 2;
  RX_TASK.cpu_reserve.nano_secs = 0;
  RX_TASK.offset.secs = 0;
  RX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&RX_TASK);

  TX_TASK.task = tx_task;
  nrk_task_set_stk (&TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
  TX_TASK.prio = 3;
  TX_TASK.FirstActivation = TRUE;
  TX_TASK.Type = BASIC_TASK;
  TX_TASK.SchType = PREEMPTIVE;
  TX_TASK.period.secs = 1;
  TX_TASK.period.nano_secs = 0;
  TX_TASK.cpu_reserve.secs = 1;
  TX_TASK.cpu_reserve.nano_secs = 0;
  TX_TASK.offset.secs = 0;
  TX_TASK.offset.nano_secs = 0;
  nrk_activate_task (&TX_TASK);

}


void nrk_register_drivers()
{
	int8_t val;

	// Register the Basic FireFly Sensor device driver
	// Make sure to add: 
	//     #define NRK_MAX_DRIVER_CNT  
	//     in nrk_cfg.h
	// Make sure to add: 
	//     SRC += $(ROOT_DIR)/src/drivers/platform/$(PLATFORM_TYPE)/source/ff_basic_sensor.c
	//     in makefile
val=nrk_register_driver( &dev_manager_ff3_sensors,FIREFLY_3_SENSOR_BASIC);
if(val==NRK_ERROR) nrk_kprintf( PSTR("Failed to load sensor driver\r\n") );
}

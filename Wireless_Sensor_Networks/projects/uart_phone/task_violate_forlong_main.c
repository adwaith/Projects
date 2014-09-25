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
#include <bmac.h>
#include <avr/sleep.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include "rx_utils.h"
#include "global.h"

NRK_STK Stack1[NRK_APP_STACKSIZE];
nrk_task_type TaskOne;
void uart_task(void);

NRK_STK Stack2[NRK_APP_STACKSIZE];
nrk_task_type TaskTwo;
void discover_task(void);

NRK_STK Stack3[NRK_APP_STACKSIZE];
nrk_task_type TaskThree;
void rx_task (void);

NRK_STK Stack4[NRK_APP_STACKSIZE];
nrk_task_type TaskFour;
void retrans_task (void);

void version_task (void);

void nrk_create_taskset();

int
main ()
{
  nrk_setup_ports();
  nrk_setup_uart(UART_BAUDRATE_115K2);

	log_g = 1;
  if(log_g)printf("log:Starting up...\n" );

  nrk_init();
uint8_t i;
request_flag_g=0;
retransmit_count_g=0;
//added this 1
for(i=0;i<5;i++)
{
	version_g[i] = 0;
}
//added
	version_g[MAC_ADDR] = 1;

  nrk_led_clr(ORANGE_LED);
  nrk_led_clr(BLUE_LED);
  nrk_led_set(GREEN_LED);
  nrk_led_clr(RED_LED);
 	
	tx_sem = nrk_sem_create(1,4);
  if(tx_sem==NULL) nrk_kprintf( PSTR("log:Error creating sem\n" ));
	
	conn_sem = nrk_sem_create(1,4);
  if(conn_sem==NULL) nrk_kprintf( PSTR("log:Error creating sem\n" ));

	uart_sem = nrk_sem_create(1,4);
  if(conn_sem==NULL) nrk_kprintf( PSTR("log:Error creating sem\n" ));
	
	ack_sem = nrk_sem_create(1,4);
  if(conn_sem==NULL) nrk_kprintf( PSTR("log:Error creating sem\n" ));
  
	nrk_time_set(0,0);
  bmac_task_config ();
  nrk_create_taskset ();
  nrk_start();
 

  return 0;
}


void uart_task()
{
	char c;
	nrk_sig_t uart_rx_signal;
	//nrk_sig_mask_t sm;
	uint8_t buf_pos = 0;
	uint8_t vbuf_pos = 0;
	uint8_t pos = 0;
	uint8_t other_active = 1;  
	uint8_t version_buf[10];
	int16_t my_version = 0;
	uint8_t connection_l;  
	uint8_t activate_uart_l;  
	uint8_t ack_received_l;

  if(log_g) printf("log:uart_task PID=%d\n",nrk_get_pid());

  // Get the signal for UART RX  
  uart_rx_signal=nrk_uart_rx_signal_get();
  // Register your task to wakeup on RX Data 
  if(uart_rx_signal==NRK_ERROR) nrk_kprintf( PSTR("log:Get Signal ERROR!\n") );
  nrk_signal_register(uart_rx_signal);


  while(1) {

	nrk_sem_pend(conn_sem);
	connection_l = connection_g;  
	nrk_sem_post(conn_sem);
	nrk_sem_pend(ack_sem);
	ack_received_l = ack_received_g;
	nrk_sem_post(ack_sem);


	if(activate_uart_g == 1 && other_active == 1)
	{
		memset(uart_rx_buf,0,buf_pos); 
		buf_pos = 0;
		if(log_g) printf("log:Switching off logs\n");
		req_for_next_data();
		log_g = 0;
		other_active = 0;
	}
	else if(activate_uart_g == 0 && vertsk_active_g == 0)
	{
		nrk_wait_until_next_period();	
		continue;
	} else if(activate_uart_g == 0 && connection_l == 0 && vertsk_active_g == 1) { // assuming this completes in one period
			
		if(log_g)printf("verreq\n");
		log_g = 0;
		while(nrk_uart_data_ready(NRK_DEFAULT_UART)!=0)
		{
			// Read Character
			c=getchar();
			version_buf[vbuf_pos++] = c; 	
			nrk_led_toggle(GREEN_LED);
			if(c =='&')
			{
				log_g = 1;
				version_buf[vbuf_pos++] = '\0';
				pos = 0; 
				my_version = get_next_int(version_buf,&pos,vbuf_pos);
				if(log_g) printf("log:Version: %d\n",my_version);
				version_g[MAC_ADDR] = my_version; 
				vertsk_active_g = 0;
				vbuf_pos = 0; 
			}
		}		
		log_g = 1;
		if(log_g)printf("log:after v\n");
		nrk_wait_until_next_period();
		continue; 
	}
	
	nrk_sem_pend(conn_sem);
	connection_l = connection_g;  
	nrk_sem_post(conn_sem);
	nrk_sem_pend(ack_sem);
	ack_received_l = ack_received_g;
	nrk_sem_post(ack_sem);



	if(ack_received_l == 1 && connection_l == 1)  
	{
	
		while(nrk_uart_data_ready(NRK_DEFAULT_UART)!=0)
		{
			// Read Character
			c=getchar();
			uart_rx_buf[buf_pos++] = c; 	
			nrk_led_toggle(GREEN_LED);
			if(c =='$')
			{
				log_g = 1;
				uart_rx_buf[buf_pos++] = '\0';
				if(log_g) printf("log:From UART %d %d\n",ack_received_l,connection_l);
				if(log_g) printf("log:%s\n",uart_rx_buf);
				activate_uart_g = 0;
				other_active = 1;
				// tx a packet				
				bmac_tx_pkt(uart_rx_buf,buf_pos);
				nrk_sem_pend(ack_sem);
				ack_received_g = 0; 
				nrk_sem_post(ack_sem);
				ack_received_l = 0;
				pending_retransmit_g = 1;
			} else if (c == '&') {
				buf_pos = 0; 		
			}
		}
	}
	//sm=nrk_event_wait(SIG(uart_rx_signal));
	//if(sm != SIG(uart_rx_signal))
	//	nrk_kprintf( PSTR("RX signal error\n") );
	nrk_wait_until_next_period();	
  }

}

void retrans_task() {

  if(log_g) printf ("log:retrans_task PID=%d\n", nrk_get_pid ());
	uint8_t cnt = 0;
	uint8_t connection_l;
	uint8_t data_cnt; 
//	while (!bmac_started ())
    nrk_wait_until_next_period ();

	while(1) {
		nrk_sem_pend(conn_sem);
		connection_l = connection_g;
		nrk_sem_post(conn_sem);	

		if(pending_retransmit_g == 1)  {
				if(log_g) printf("log:Re-transmitting packet\n");	
				bmac_tx_pkt(uart_rx_buf, strlen(uart_rx_buf));
				retransmit_count_g++;
				if(retransmit_count_g==15)
				terminate_connection();
			} else {
				// this checks if there is a connection and one is receivng data then 
				// if the sender shuts down then the connection is terminated after a while	
				if(connection_l == 1) {
					if(recv_data_g == 1) 
						data_cnt = 0; 
					else if(recv_data_g == 0)
						data_cnt++;
					
					if(data_cnt > 10) {
						terminate_connection();
					}
			
				}
				
				cnt++;
				//if(log_g) printf("log:ver activ con:%d cnt:%d\n",connection_l,cnt);
				if(cnt>0 && connection_l == 0) {
				cnt = 0;
				vertsk_active_g = 1; 
				}
			}

		nrk_wait_until_next_period();

	}
}

void rx_task ()
{
	uint8_t pos,len,i,version;
	int8_t rssi;
  uint8_t *local_rx_buf;
  int16_t recipient,sender,dataseq;
	char type;
  if(log_g) printf ("log:rx_task PID=%d\n", nrk_get_pid ());
  // init bmac on channel 15 
  bmac_init (15);

  bmac_rx_pkt_set_buffer (rx_buf, RF_MAX_PAYLOAD_SIZE);

  while (1) {
    // Wait until an RX packet is received
    bmac_wait_until_rx_pkt ();
    // Get the RX packet 
    nrk_led_set (ORANGE_LED);
    local_rx_buf = bmac_rx_pkt_get (&len, &rssi);
    if(log_g) printf ("log:Packet recv\n");
    if(log_g) printf ("log:");
    for (i = 0; i < len; i++) {
      if(log_g) printf ("%c", rx_buf[i]);
		}
    if(log_g) printf ("\n");

		pos = 0; 
		recipient = get_next_int(local_rx_buf,&pos,len); 
		pos+=1;
		sender = get_next_int(local_rx_buf,&pos,len);
		pos+=1; 
		type = local_rx_buf[pos];
		pos+=2;

		if((recipient != 0 && recipient != MAC_ADDR) || sender == MAC_ADDR) {

			if(log_g) printf("log:Ignore\n");

		} else if(recipient == 0) {

			version =get_next_int(local_rx_buf,&pos,len);
			on_discover_pkt(sender, version);
 
		}	else if (type == 'C') {

			onConnectionReq(sender);

		} else if(type == 'D' ) {

			dataseq = get_next_int(local_rx_buf,&pos,len);	
			onData(sender,dataseq,local_rx_buf,len);

		}	else if(type == 'A') {

			dataseq = get_next_int(local_rx_buf,&pos,len);	
			onAck(sender, dataseq);

		} else {

			if(log_g) printf("log:Invalid MSG\n");

		}
    
		// Release the RX buffer so future packets can arrive 
    memset(local_rx_buf,0,len+1);
    bmac_rx_pkt_release ();
  	nrk_led_clr(ORANGE_LED);
	}
}

void discover_task()
{
	uint8_t len;
	uint8_t connection_l;
	while (!bmac_started ())
    nrk_wait_until_next_period ();		

	if(log_g) printf("log:discover_task PID=%d\n",nrk_get_pid());


	while(1) {
			nrk_sem_pend(conn_sem);
			connection_l = connection_g;
			nrk_sem_post(conn_sem);
			

			if(connection_l == 0 && vertsk_active_g == 0) {
				nrk_led_toggle(BLUE_LED);	
				if(log_g) printf("log:Sending discover pkt\n");
				nrk_sem_pend(tx_sem);
				sprintf(tx_buf,"0:%d:S:%d",MAC_ADDR,version_g[MAC_ADDR]);
				bmac_tx_pkt(tx_buf, strlen(tx_buf));
				nrk_sem_post(tx_sem);
				memset(tx_buf,0,strlen(tx_buf));
			}
    	nrk_wait_until_next_period ();
	
	}

}

void nrk_create_taskset()
{
  
  TaskOne.task = uart_task;
  nrk_task_set_stk( &TaskOne, Stack1, NRK_APP_STACKSIZE);
  TaskOne.prio = 3;
  TaskOne.FirstActivation = TRUE;
  TaskOne.Type = BASIC_TASK;
  TaskOne.SchType = PREEMPTIVE;
  TaskOne.period.secs = 0;
  TaskOne.period.nano_secs = 400*NANOS_PER_MS;
  TaskOne.cpu_reserve.secs = 0;
  TaskOne.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TaskOne.offset.secs = 0;
  TaskOne.offset.nano_secs= 0;
  nrk_activate_task (&TaskOne);
	
  TaskTwo.task = discover_task;
  nrk_task_set_stk( &TaskTwo, Stack2, NRK_APP_STACKSIZE);
  TaskTwo.prio = 2;
  TaskTwo.FirstActivation = TRUE;
  TaskTwo.Type = BASIC_TASK;
  TaskTwo.SchType = PREEMPTIVE;
  TaskTwo.period.secs = 4;
  TaskTwo.period.nano_secs = 0*NANOS_PER_MS;
  TaskTwo.cpu_reserve.secs = 0;
  TaskTwo.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
	// this had an offset for mac address
  TaskTwo.offset.secs = 0;
  TaskTwo.offset.nano_secs= 0;
  nrk_activate_task (&TaskTwo);

  TaskThree.task = rx_task;
  nrk_task_set_stk( &TaskThree, Stack3, NRK_APP_STACKSIZE);
  TaskThree.prio = 4;
  TaskThree.FirstActivation = TRUE;
  TaskThree.Type = BASIC_TASK;
  TaskThree.SchType = PREEMPTIVE;
  TaskThree.period.secs = 0;
  TaskThree.period.nano_secs = 400*NANOS_PER_MS;
  TaskThree.cpu_reserve.secs = 0;
  TaskThree.cpu_reserve.nano_secs = 200*NANOS_PER_MS;
  TaskThree.offset.secs = 0;
  TaskThree.offset.nano_secs= 0;
  nrk_activate_task (&TaskThree);

	TaskFour.task = retrans_task;
  nrk_task_set_stk( &TaskFour, Stack4, NRK_APP_STACKSIZE);
  TaskFour.prio = 1;
  TaskFour.FirstActivation = TRUE;
  TaskFour.Type = BASIC_TASK;
  TaskFour.SchType = PREEMPTIVE;
  TaskFour.period.secs = 4;
  TaskFour.period.nano_secs = 0*NANOS_PER_MS;
  TaskFour.cpu_reserve.secs = 0;
  TaskFour.cpu_reserve.nano_secs = 100*NANOS_PER_MS;
  TaskFour.offset.secs = 0;
  TaskFour.offset.nano_secs= 0;
  nrk_activate_task (&TaskFour);

}




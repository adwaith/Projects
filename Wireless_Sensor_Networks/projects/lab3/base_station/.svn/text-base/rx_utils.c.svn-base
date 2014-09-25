#include <stdio.h>
#include "global.h"
#include "rx_utils.h"

void terminate_connection() {
			connection_g = 0;
			recepient_mac_g = 0;
			data_index_g = -1;
			pending_retransmit_g = 0; 
}

int16_t get_next_int(char*rx_buf, uint8_t* pos,uint8_t len) {

	int16_t sum = 0;
	if(rx_buf[*pos] == '-')
	{
		*pos = *pos + 2;
		return -1;
	}
		
	while(*pos < len && rx_buf[*pos] != '\0' && rx_buf[*pos] >='0' && rx_buf[*pos]<='9') 
	{
		sum *= 10;
		sum += (rx_buf[*pos]-'0');
		*pos = *pos+1;
	}
	return sum;	
}

		
void on_discover_pkt(uint8_t sender) {
	uint8_t len;
	// send a connection request if u r not in a connection. 
	if(connection_g == 0) {
		if(log_g) printf("log:Sending Connection Req\n");
		nrk_sem_pend(tx_sem);
		len = sprintf(tx_buf,"%d:%d:C",sender,MAC_ADDR);
		bmac_tx_pkt(tx_buf, len);
		nrk_sem_post(tx_sem);
		memset(tx_buf,0,len);
	}
}

void req_for_next_data() {
	
	data_index_g++;
	if(log_g) printf("datareq:%d:%d:%d\n",data_index_g,recepient_mac_g,MAC_ADDR);
	
}

void onConnectionReq(uint8_t sender_mac) {
// accept a connecction is u r not in a connecction
	if(log_g) printf("log:On Cn Rq %d\n",connection_g);
	if(connection_g == 0) {
		if(log_g) printf("log:Set up new Connection\n");
		activate_uart_g = 1;
		ack_received_g = 1; 
		recepient_mac_g = sender_mac;
		connection_g = 1;	
		data_index_g = -1;
	}
}

void onAck(uint8_t sender_mac, int16_t ack_num) {

	uint8_t len;

	if(log_g) printf("log:Ack received\n");
	
	if(ack_num == data_index_g) 
	{
		ack_received_g = 1;
		activate_uart_g = 1; 
		pending_retransmit_g = 0; 
	}

	if(ack_num == 255)
	{

		if(sender_mac == recepient_mac_g)
		{
			terminate_connection();
		}
	}
}

void onData(uint8_t sender, int16_t dataseq) {

	uint8_t len; 
	if(log_g) printf("log:Data Received\n");	

	if(connection_g == 0 && dataseq != 255 && dataseq == 0)
	{
		connection_g = 1;
		recepient_mac_g = sender;
	}	else	if(connection_g == 1 && dataseq == 255)
	{
		terminate_connection();
	}
	
		if(log_g) printf("log:Before ack, %d, %d, %d\n",sender,recepient_mac_g,dataseq);

	if((sender == recepient_mac_g && connection_g == 1) || dataseq == 255)
	{// send ack
		if(log_g) printf("log:Sending ACK\n");
		nrk_sem_pend(tx_sem);
		len = sprintf(tx_buf,"%d:%d:A:%d",sender,MAC_ADDR,dataseq);
		bmac_tx_pkt(tx_buf, len);
		nrk_sem_post(tx_sem);
		memset(tx_buf,0,len);
	}

	if(dataseq == 255) {
		// get the version number of the data
	}

}

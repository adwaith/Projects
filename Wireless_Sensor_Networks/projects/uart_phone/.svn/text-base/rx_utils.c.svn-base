#include <stdio.h>
#include "global.h"
#include "rx_utils.h"

void terminate_connection() {
			if(log_g)nrk_kprintf(PSTR("log:TConn\n"));
			nrk_sem_pend(conn_sem);
			connection_g = 0;
			nrk_sem_post(conn_sem);
			recepient_mac_g = 0;
			data_index_g = -1;
			pending_retransmit_g = 0; 
			request_flag_g=0;
			retransmit_count_g=0;
			nrk_sem_pend(ack_sem);
			ack_received_g = 0;
			nrk_sem_post(ack_sem);
			activate_uart_g = 0;
			recv_data_seq_g = 0; 
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

		
void on_discover_pkt(uint8_t sender, uint8_t versionMSG) {
	uint8_t len;
	// send a connection request if u r not in a connection.
	uint8_t connection_l; 
	nrk_sem_pend(conn_sem);
	connection_l = connection_g;
	nrk_sem_post(conn_sem); 
	if(connection_l == 0) {
		if(log_g) printf("log: Got discover packet from %d\n",sender);

		if(version_g[sender] != versionMSG)
		{
			if(log_g) printf("log:Version numbers of %d dont match msg:%d,my:%d\n",sender,versionMSG,version_g[sender]);

			if(request_flag_g==0||(request_flag_g > 0 && sender>MAC_ADDR))
			{
				if(log_g) printf("log:Sending Connection Req\n");
				nrk_sem_pend(tx_sem);
				len = sprintf(tx_buf,"%d:%d:C:%d",sender,MAC_ADDR,version_g[sender]);
				bmac_tx_pkt(tx_buf, len);
				nrk_sem_post(tx_sem);
				memset(tx_buf,0,len);
				request_flag_g = sender;
			}
		}
		else
		{	
			if(log_g) printf("log:Version number received %d and existing %d match\n", versionMSG, version_g[sender]);
		}
	}
}
void req_for_next_data() {
	
	data_index_g++;
	if(log_g) printf("datareq:%d:%d:%d\n",data_index_g,recepient_mac_g,MAC_ADDR);
	
}

void onConnectionReq(uint8_t sender_mac, uint8_t version) {
// accept a connecction is u r not in a connecction
	uint8_t connection_l; 
	nrk_sem_pend(conn_sem);
	connection_l = connection_g;
	nrk_sem_post(conn_sem);
	
	if(log_g) printf("log:On Cn Rq %d\n",connection_l);
	if(connection_l == 0) {
		if(log_g) printf("conreq:%d:%d\n",sender_mac,version);
		activate_uart_g = 1;
		nrk_sem_pend(ack_sem);
		ack_received_g = 1; 
		nrk_sem_post(ack_sem);
		recepient_mac_g = sender_mac;
		nrk_sem_pend(conn_sem);
		connection_g = 1;	
		nrk_sem_post(conn_sem);
		data_index_g = -1;
	}
}

void onContactShare(uint8_t *rx_local_buf, uint8_t lenBuf) {
	int i; 
    for (i = 0; i < lenBuf; i++) {
      if(log_g) printf ("%c", rx_local_buf[i]);
		}
    if(log_g) printf ("\n");

} 
void onAck(uint8_t sender_mac, int16_t ack_num) {

	uint8_t len;
	
	if(log_g) printf("log:Ack received\n");
	
	if(ack_num == data_index_g) 
	{
		nrk_sem_pend(ack_sem);
		ack_received_g = 1;
		nrk_sem_post(ack_sem);
		activate_uart_g = 1; 
		pending_retransmit_g = 0; 
		retransmit_count_g=0;
	}

	if(ack_num == 255)
	{
		if(sender_mac == recepient_mac_g)
		{
			terminate_connection();
		}
	}
}

void onData(uint8_t sender, int16_t dataseq, uint8_t* rx_local_buf, uint8_t lenBuf) {

	uint8_t len,i;
	uint8_t pos,connection_l; 
	int16_t version;
	//if(log_g) printf("log:Data Received\n");	

	nrk_sem_pend(conn_sem);
	connection_l = connection_g;
	nrk_sem_post(conn_sem);

	if(connection_l == 0 && dataseq != 255 && dataseq == 0)
	{
		if(log_g) printf("log:New Con wi %d I am %d\n",sender,MAC_ADDR);
		nrk_sem_pend(conn_sem);
		connection_g = 1;
		nrk_sem_post(conn_sem);
		connection_l = 1; 
		recepient_mac_g = sender;
		recv_data_seq_g = 1; 	
		
		if(log_g) printf ("pkt:");
    for (i = 0; i < lenBuf; i++) {
      if(log_g) printf ("%c", rx_local_buf[i]);
		}
    if(log_g) printf ("\n");

	}	else	if(connection_l == 1 && dataseq == 255)
	{

		if(connection_l==1 && sender == recepient_mac_g) {
			if(log_g) printf ("pkt:");
    	for (i = 0; i < lenBuf; i++) {
      	if(log_g) printf ("%c", rx_local_buf[i]);
			}
    	if(log_g) printf ("\n");
		}

		terminate_connection();
	}

	nrk_sem_pend(conn_sem);
	connection_l = connection_g;
	nrk_sem_post(conn_sem);

	if(connection_l==1 && sender == recepient_mac_g) {
		recv_data_seq_g = dataseq; 	
		if(log_g) printf ("pkt:");
    for (i = 0; i < lenBuf; i++) {
      if(log_g) printf ("%c", rx_local_buf[i]);
		}
    if(log_g) printf ("\n");

	}

	
		//if(log_g) printf("log:Before ack, %d, %d, %d\n",sender,recepient_mac_g,dataseq);

	if((sender == recepient_mac_g && connection_l == 1) || dataseq == 255)
	{// send ack
		//if(log_g) printf("log:ACK %d\n",dataseq);
		nrk_sem_pend(tx_sem);
		len = sprintf(tx_buf,"%d:%d:A:%d",sender,MAC_ADDR,dataseq);
		bmac_tx_pkt(tx_buf, len);
		memset(tx_buf,0,len);
		nrk_sem_post(tx_sem);
	}

	if(dataseq == 255) {
		// get the version number of the data
		pos = lenBuf - 1;
		while(rx_local_buf[pos] != '-') pos--;
		pos++;
		version = get_next_int(rx_local_buf,&pos,lenBuf);	
		
		version_g[sender] = version;
		//if(log_g) printf("log:v:%d is %d\n",sender,version_g[sender]);
	}

}

void onBasketBallPkt() {
	
	if(log_g)nrk_kprintf(PSTR("basketball\n"));	
	
}

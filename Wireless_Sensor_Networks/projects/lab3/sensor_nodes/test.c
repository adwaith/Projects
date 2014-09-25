#include <stdio.h>
#include <string.h>
#define MAC_ADDR 3
#define NUM_NODES 10
#define NEIGHBOR_TTL 2
#define DATA_PERIOD 1

char tx_buf[128]; 
char rx_buf[128]; 
// neighbor variables

int neighbour_list[NUM_NODES] = {0};
int flood_seq_num = 1;
int flood_seq_nums[NUM_NODES] = {0};
int data_ack_seq = -1; 
int sampling_time = 30; // one sample every 30 seconds
int update_time = 30; // update once every 30 seconds
int route = 0; // stores the mac address of the node to send data to
int routing_received = 0; 

void ack_from_neighbour(int);
void tx_neighbour_list();
int get_next_int(char*,int*,int);
void recv_flood_msg();
void data_task();



void main()
{

	ack_from_neighbour(1);
	tx_neighbour_list();		
	ack_from_neighbour(5);
	tx_neighbour_list();		
	ack_from_neighbour(2);
	tx_neighbour_list();		
	ack_from_neighbour(4);
	tx_neighbour_list();		
	ack_from_neighbour(2);
	tx_neighbour_list();		
	ack_from_neighbour(6);
	tx_neighbour_list();		
	ack_from_neighbour(1);
	tx_neighbour_list();		

	// flooding for NL
	sprintf(rx_buf,"-1:2:10:NL:1,2,3,4");
	recv_flood_msg();
	printf("\n");	
	// flooding for routing table
	sprintf(rx_buf,"-1:2:11:RT:2,1:3,2:");
	recv_flood_msg();
	printf("\n");	
	sprintf(rx_buf,"-1:2:12:RT:");
	recv_flood_msg();
	printf("\n");	
	sprintf(rx_buf,"-1:2:13:RT:20,1:30,2:");
	recv_flood_msg();
	
	printf("\n");	
	// flooding for update rates
	sprintf(rx_buf,"-1:2:15:UR:43");
	recv_flood_msg();
	
	printf("\n");	
	// flooding for sampling rates
	sprintf(rx_buf,"-1:2:16:SR:10");
	recv_flood_msg();
	
	printf("\n");	
	// flooding for ack of data
	sprintf(rx_buf,"-1:2:17:DA:1:34,2:33,3:33,4:34");
	recv_flood_msg();

	printf("\n");	
}

void recv_flood_msg() {

	int pos = 3;
	int len = strlen(rx_buf);
	int sender = get_next_int(rx_buf,&pos,len); pos++;
	printf("Sender: %d, pos: %d\n",sender,pos);
	int seq_num = get_next_int(rx_buf,&pos,len); pos++;
	printf("Seq num: %d, pos: %d\n",seq_num,pos);
	int mac_addr; 

	if(sender >= NUM_NODES)
	{
		printf("Invalid sender Number\n");
		return;
	}

	// the flood index in msg is lower ignore msg. 
	if(flood_seq_nums[sender] >= seq_num)
	{
		printf("the flood index in msg is lower ignore msg\n");
		return;
	}
	// update the flood index from new seq number
	flood_seq_nums[sender] = seq_num;
	if(pos >= len)
	{
		printf("pos >= len\n");
		return;
	}

	switch(rx_buf[pos])
	{
		case 'N':
			printf("Neighbor list bcast\n");
			//sprintf(rx_buf,"-1:2:13:NL:1,2,3,3,4,,");
			pos+=3;
			//while(pos < len && rx_buf[pos] >= '0' && rx_buf[pos] <= '9')
			//	neighbour = get_next_int(rx_buf,&pos,len);
			//	add_new_neighbour()
			break;
		case 'U':
			pos+=3;
			printf("Change update rate to %d\n",get_next_int(rx_buf,&pos,len));
			// send ack of update rate
			break;
		case 'S':
			pos+=3;
			printf("Change sampling rate to %d\n",get_next_int(rx_buf,&pos,len));
			// send ack of sampling rate
			break;
		case 'D':
			pos+=3; 
			while(get_next_int(rx_buf,&pos,len) != MAC_ADDR)
			{
				pos++; 
				get_next_int(rx_buf,&pos,len);	
				pos++;
			}
			pos++;
			seq_num = get_next_int(rx_buf,&pos,len);
			if(seq_num > data_ack_seq)
				data_ack_seq = seq_num; 
			printf("The seq num of data pkt being acked is %d\n",seq_num);
			// update the data seq number of the ack thing	
			break;
		case 'R':
			pos+=3; 
			printf("Rounting table info\n");
			if(pos >= len)	
			{
				printf("No table attached\r\n");
				break;
			}
			while(pos < len && get_next_int(rx_buf,&pos,len) != MAC_ADDR)
			{
				pos++; 
				get_next_int(rx_buf,&pos,len);	
				pos++;
			}
			pos++;
			route = get_next_int(rx_buf,&pos,len);
			if(route == 0)
				printf("Routing not received\n");
			else
			{
				printf("Route is %d",route);
				routing_received = 1; 
			}
			break;
		default:
			printf("invalid message");
			break;
	}	
	

}

int get_next_int(char*rx_buf,int* pos,int len) {
	
	int sum = 0;
	while(*pos < len && rx_buf[*pos] != '\0' && rx_buf[*pos] >='0' && rx_buf[*pos]<='9') 
	{
		sum *= 10;
		sum += (rx_buf[*pos]-'0');
		*pos = *pos+1;
	}
	return sum;	
}

void ack_from_neighbour(int neighbour) {
	int i =0;
	if(neighbour >= NUM_NODES)
	{
		printf("invalid neighbour number\n");
		return; 
	}
	
	neighbour_list[neighbour] = NEIGHBOR_TTL;
}

void tx_neighbour_list() {

	sprintf(tx_buf, "-1:%d:%d:NL:",MAC_ADDR,flood_seq_num);
	flood_seq_num++;
	int i=0;
	int neighbour = 0; 
	for(i=0;i<NUM_NODES;i++)
	{
		if(neighbour_list[i] > 0)
		{
			neighbour = 1; 
			neighbour_list[i]--;
			sprintf(tx_buf+strlen(tx_buf),"%d,",i);
		}
	}
	if(neighbour==0)
		return;
	printf("%s\n",tx_buf);	
}

void wait_until_next_period(){}
// period = 1s
// reserve = 10ms
void data_task() {

	int sample_value; 
	int data_wakeups = 0; 
	int data_seq_num = 0;

	while(route == 0)
			wait_until_next_period();

	while(1) 
	{
		if( data_wakeups < data_seq_num*sampling_time && data_ack_seq == data_seq_num)
		{
			wait_until_next_period();
		}
		else if( data_ack_seq < data_seq_num && data_wakeups < data_seq_num * sampling_time)
		{
			sprintf(tx_buf,"%d:%d:DM:%d:%d:%d",route,MAC_ADDR,MAC_ADDR,data_seq_num,sample_value);
			printf("Transmiting %s",tx_buf);
			wait_until_next_period();
		}
		else if(data_wakeups == data_seq_num * sampling_time)
		{	
			// sample data
			sample_value = 1023; 
			sprintf(tx_buf,"%d:%d:DM:%d:%d:%d",route,MAC_ADDR,MAC_ADDR,data_seq_num,sample_value);
			printf("Transmiting %s",tx_buf);
			data_seq_num++; 
			wait_until_next_period();
		}
		else
		{
			printf("Invalid state of data task\n");
			wait_until_next_period();
			
		}
		data_wakeups++;
	}
}


//period = 1s
// reserve = 10ms 
void discover_task() {
	int discover_wakeups = 0; 
	int data_seq_num = 0;
	int toggle_sw = 0; 

	while(route == -1)
			wait_until_next_period();

	while(1) 
	{
		discover_wakeups++;
		if( discover_wakeups < update_time && routing_received == 1)
		{
			wait_until_next_period();
		}
		else if(routing_received == 0)
		{
			if(toggle_sw)
			{
				toggle_sw = 0;
				tx_neighbour_list(); 
			}
			else
			{
				toggle_sw = 1; 
				sprintf(tx_buf,"0:%d:DM",MAC_ADDR);
				printf("%s\n",tx_buf);
			}
			wait_until_next_period();
		}
		else if(discover_wakeups == update_time && routing_received == 1)
		{
			discover_wakeups = 0; 
			routing_received = 0; 
			
			sprintf(tx_buf,"0:%d:DM",MAC_ADDR);
			printf("%s\n",tx_buf);
			wait_until_next_period();	
		}
	}

}

void waitforpacket(){}
void recv_task() {

	waitforpacket();
	
		

}



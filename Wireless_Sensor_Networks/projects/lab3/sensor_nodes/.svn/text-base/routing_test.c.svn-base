#include<stdio.h>

#define NUM_NODES 10


void process_neighbour_list(int sender, char*list,int len);

int neighbour_list[NUM_NODES][NUM_NODES];
int routing_table[NUM_NODES] = {0,1,-1,-1,-1,-1,-1,-1,-1,-1};

void route()
{
	int level = 1,i,j; 
	int curr_level[NUM_NODES] = {0};
	int routed[NUM_NODES] = {0};
	curr_level[1] = level; 	

	while(level != NUM_NODES) 
	{
		for(i=1;i<NUM_NODES;i++) // for all nodes in current level calculate the neighbour. 
		{
			if(curr_level[i] == level)
			{
				for(j=2;j<NUM_NODES;j++)
				{
					if(neighbour_list[i][j] == 1 && routed[j] == 0)
					{
						routed[j] = 1;
						routing_table[j] = i; 
						curr_level[j] = level + 1; 
					}
				}
			}
		}
		level++;
//	printf("Routing Table: ");
//	for(i=0;i<NUM_NODES;i++)
//		printf("%d->%d ",i,routing_table[i]);
//	printf("\r\n");
//	printf("Level: [");
//	for(i=0;i<NUM_NODES;i++)
//		printf("%d%d ",i,routing_table[i]);
//	printf("]\r\n");
//	printf("\n");
	}
	printf("Routing Table: ");
	for(i=0;i<NUM_NODES;i++)
		printf("%d->%d ",i,routing_table[i]);
	printf("\r\n");
	


}

int main() 
{
		
	process_neighbour_list(2,"1,",2);	
	route();
	process_neighbour_list(2,"1,3,",4);	
	route();
	process_neighbour_list(3,"2,",2);	
	route();

}

int get_next_int(char*rx_buf, int* pos,int len) 
{

	if(rx_buf[*pos] == '-')
	{
		*pos = *pos + 2;
		return -1;
	}
		
	int sum = 0;
	while(*pos < len && rx_buf[*pos] != '\0' && rx_buf[*pos] >='0' && rx_buf[*pos]<='9') 
	{
		sum *= 10;
		sum += (rx_buf[*pos]-'0');
		*pos = *pos+1;
	}
	return sum;	
}

void process_neighbour_list(int sender, char*list,int len)
{
	int pos = 0; 
	int neighbor; 
	while(pos<len)
	{
		neighbor = get_next_int(list,&pos,len);
		pos++;
		neighbour_list[sender][neighbor] = 1;
		neighbour_list[neighbor][sender] = 1;	
	}					
}

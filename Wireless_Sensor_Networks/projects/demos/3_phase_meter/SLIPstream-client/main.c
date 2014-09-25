#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <slipstream.h>
#include <time.h>

#define NONBLOCKING  0
#define BLOCKING     1

#define Angle_MF	0.084375
#define Curr_MF	1.63
#define Curr_Div  88600
#define Volt_MF	121.8
#define Volt_Div 3941000
#define Freq_MF 256000
#define Power_MF	57.13
#define Power_Div 11800
#define Energy_MF 1.00
#define Freq_MF 256000


//FILE *fFastRaw, *NoTimeData;
//FILE *fFastRaw;
//FILE *fSlowRaw;
FILE *fFastData, *fSlowData;
int main (int argc, char *argv[])
{
	unsigned char buffer[128];
	int v,cnt,i,intval,v_ParamCnt=0;
	//signed int watt, var, data;
	int v_PktCnt_u8r = 0;
	char f_WrTime_u8r = 0;
	time_t PCtime;
	struct tm *timeinfo;
	char TimeStr[100];
	float flval;
	long int fDataSeq = 0, fPktSeqOld=0, fPktSeqNew=0;

	if (argc != 3) 
	{
		printf ("Usage: server port\n");
		exit (1);
	}
//	fFastRaw = fopen("fFastRaw.txt","wb");
//	fFastWtime = fopen("fFastWtime.txt","wb");
	fFastData = fopen("fFastData.txt","wb");
	fSlowData	= fopen("fSlowData.txt","wb");
//	fSlowRaw		= fopen("fSlowRaw.txt","wb");

	v=slipstream_open(argv[1],atoi(argv[2]),NONBLOCKING);
	sprintf (buffer, "This is a sample slip string: Count %d\n", cnt);
	v=slipstream_send(buffer,strlen(buffer)+1);
	if (v == 0) printf( "Error sending\n" );
	cnt = 0;
	while (1) 
	{
		cnt++;
		v=slipstream_receive( buffer );
		if (v > 0) 
		{
			time( &PCtime);
			printf ("Got: ");
			for (i = 0; i < v; i++)
			{
				printf ("%x ", (uint8_t)buffer[i]);
			}
			
	/*		if(buffer[0] == 0x4D)
			{
				fprintf(fSlowRaw,"\r\n%ld, %d",PCtime,v);	
				fprintf(fSlowRaw,",%d,%d,",buffer[1],buffer[2]);
				for (i=0;i<v;i++)
					fprintf(fSlowRaw,",%d",buffer[i]);
			}
			fflush(fSlowRaw);			
	*/		printf ("\n");
			if(buffer[0] == 0x4D && v==53) //slow data - sometimes incomplete packet is rcvd, so checking for size
			{
				fprintf(fSlowData,"\r\n%ld",PCtime);
				fprintf(fSlowData,",%d,%d",buffer[1],buffer[2]);//Meter addr, packet sequence number

				for(i=3;i<v;i=i+5)
				{
					memcpy(&intval,buffer+i+1,4);
				//	fprintf(fSlowRaw,"%c,",buffer[i]);
				//	fprintf(fSlowRaw,"%d,",intval);
				
					switch(buffer[i])
					{
						case 'i':	//Current
							if((intval&0x800000)>>23 == 1)
								intval |= 0xff000000;
							flval = ((float)intval)/Curr_Div;
							flval = flval*Curr_MF;
							break;

						case 'v':	//Voltage
								if((intval&0x800000)>>23 == 1)
									intval |= 0xff000000;
								flval = ((float)intval)/Volt_Div;
								flval = flval*Volt_MF;
								break;

						case 'f':	//Frequency
								flval = Freq_MF/((float)intval);//We have to divide the value of period register
								break;

						case 'a':	//Angle (PF)
								flval = ((float)intval)*Angle_MF;
								if(flval > 180)
									flval = flval - 360;
								break;

						case 'w':	//Power
								flval = ((float)intval)/Power_Div;
								flval = flval*Power_MF;
								break;

						case 'e':	//Energy
								flval = ((float)intval)*Energy_MF;
								break;

						default://Should not occur
								flval = 0;
								break;
					}
					fprintf(fSlowData,",%.03f",flval);
				}
				fflush(fSlowData);
			}
			else //Fast data
			{
				//fprintf(fFastRaw,"\r\n %s",asctime(timeinfo));
			//	fprintf(fFastRaw,"\r\n%ld",PCtime);
				//fprintf(fFastWtime,"\r\n %s",asctime(timeinfo));
				//fprintf(fFastWtime,"\r\n%ld",PCtime);
							
		/*		fprintf(fFastRaw,"\n%ld Got: ",PCtime);
				for (i = 0; i < v; i++)
						fprintf(fFastRaw,"%x ", (uint8_t)buffer[i]);
				fprintf(fFastRaw,"\n");
				fflush(fFastRaw);
*/
				fPktSeqOld = fPktSeqNew;
				//fprintf(fFastWtime,",%d,%d",buffer[1],buffer[2]);
				fPktSeqNew = buffer[2];
				if(fPktSeqNew!=fPktSeqOld)
				{
					//In each packet we have 8 sets of 'fast data'. Each 'fast' data' has IA,VA,IB,VB
					if(fPktSeqNew>fPktSeqOld)
						fDataSeq = fDataSeq+8*(fPktSeqNew-fPktSeqOld-1); //If pkts are consecutive, this is just prev cnt+1; if single pkt misses it increments by 9
					else
						fDataSeq = fDataSeq+8*(fPktSeqNew-fPktSeqOld+255); //If pkts are consecutive, this is just prev cnt+1; if single pkt misses it increments by 9
					
					v_ParamCnt = 0;
					for(i=3;i<v;i=i+3)
					{
						intval = 0;//To clear all 4 bytes;
						memcpy(&intval,buffer+i,3);
						if((intval&0x800000)>>23 == 1)
							intval |=0xff000000;
						//fprintf(fFastWtime,",%d",intval);

						if(v_ParamCnt%4==0)//MAKE AS 4 for 4 data
						{
							fprintf(fFastData,"\n%ld,%d,%ld",PCtime,buffer[2],fDataSeq);
							//fprintf(fFastData,"\n%d",buffer[2]);
							fDataSeq++;
						}
						if(v_ParamCnt%2==0) //Current
						  {
						  flval = ((float)intval)/Curr_Div;
						  flval = flval*Curr_MF;
						  }
						  else //Voltage
						  {			
						  flval = ((float)intval)/Volt_Div;
						  flval = flval*Volt_MF;
						  }
						  fprintf(fFastData,",%.03f",flval);
//						fprintf(fFastData,",%d",intval);
						v_ParamCnt++;
					}
					//fflush(fFastRaw);
					//fflush(fFastWtime);
					fflush(fFastData);
				}
		}
		}
	// Pause for 1 second 
	// sleep (1);
	}
}


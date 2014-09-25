#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <slipstream.h>

#define NONBLOCKING  0
#define BLOCKING     1


//#define Angle_MF	0.084375
#define Curr_MF	1.63
#define Curr_Div  88600
#define Volt_MF	121.8
#define Volt_Div 3941000
//#define Freq_MF 256000
//#define Power_MF	57.13
//#define Power_Div 11800
//#define Energy_MF 1.00
//#define Freq_MF 256000
//
FILE *fFastData, *fSlowData;
long int vDataSeq=0;
int main (int argc, char *argv[])
{
	unsigned char buffer[128];
	int v,cnt,i,v_ParamCnt;
	time_t PCtime;
	int intval;
	float flval;

	if (argc != 3) 
	{
		printf ("Usage: server port\n");
		exit (1);
	}

	v=slipstream_open(argv[1],atoi(argv[2]),NONBLOCKING);

	fFastData = fopen("fFastData.txt","wb");
	fSlowData	= fopen("fSlowData.txt","wb");

		sprintf (buffer, "This is a sample slip string: Count %d\n", cnt);
		v=slipstream_send(buffer,strlen(buffer)+1);
		if (v == 0) printf( "Error sending\n" );

	cnt = 0;
	while (1) 
	{
		cnt++;
	//	sprintf (buffer, "This is a sample slip string: Count %d\n", cnt);
	///	v=slipstream_send(buffer,strlen(buffer)+1);
	//	if (v == 0) printf( "Error sending\n" );

		v=slipstream_receive( buffer );
		if (v > 0) 
		{
			printf ("Got: ");
			for (i = 0; i < v; i++)
				printf("%d,", buffer[i]);
			printf ("\n");

			time(&PCtime);
	//		for (i = 0; i < v; i++)
	////			fprintf(fFastData,"%d,", buffer[i]);
	//		fprintf (fFastData,"\n %ld : ",PCtime);
		//	time(&PCtime);
			
			fprintf(fFastData,"\n%ld,%d,%ld",PCtime,buffer[1],vDataSeq);
			vDataSeq++;
			v_ParamCnt=0;
			for(i=2;i<10;i=i+3)
			{
				intval = 0;//To clear all 4 bytes;
				memcpy(&intval,buffer+i,3);
				if((intval&0x800000)>>23 == 1)
					intval |=0xff000000;
			
				if(v_ParamCnt==0 || v_ParamCnt==2) //Current
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
				//fprintf(fFastData,",%d",intval);
				v_ParamCnt++;
			}
			//fflush(fFastRaw);
			//fflush(fFastWtime);
		fflush(fFastData);
		}
		// Pause for 1 second 
		//sleep (1);
	}
}
	

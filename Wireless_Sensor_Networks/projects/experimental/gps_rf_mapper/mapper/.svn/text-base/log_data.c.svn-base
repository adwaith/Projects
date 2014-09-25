#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>


#define BUF_LEN 160
#define BAUDRATE B115200
#define DATABITS CS8
#define STOPBITS CSTOPB
#define PARITYON 0
#define PARITY 0
#define USB_PORT "/dev/ttyUSB0"


int port_fd = -1;
char lineBuffer[BUF_LEN];
char lastGPS[BUF_LEN];
struct termios serial_tio;
FILE * file;
void read_packets();

void main() {

	
	bzero(&serial_tio, sizeof(serial_tio));

	port_fd = open(USB_PORT, O_RDWR | O_NOCTTY);
	if (port_fd < 0) {
		perror("Failed to open serial port");
		return;
	}

	serial_tio.c_cflag = BAUDRATE | DATABITS | STOPBITS | PARITYON | PARITY | CLOCAL| CREAD;
	serial_tio.c_iflag = IGNPAR;
	serial_tio.c_lflag = ICANON;
	serial_tio.c_cc[VMIN]= 1;
	serial_tio.c_cc[VTIME] = 0;
	tcflush(port_fd, TCIFLUSH);
	tcsetattr (port_fd, TCSANOW, &serial_tio);

	file = fopen("PacketLog.txt","w+");
	if(file < 0) {
	  perror("Failed to open file for writing");
	  return;
	}
	read_packets();
}



void read_packets() {
	int bytes = 0;

	while(1){
		bytes = read(port_fd,lineBuffer, BUF_LEN-1);
		if(bytes > 0){
			lineBuffer[bytes]= '\0';
			if(strncmp(lineBuffer, "RSSI:",5) == 0) {
				printf("%s", lineBuffer);
				fprintf(file, "%s", lineBuffer);
				fprintf(file, "%s", lastGPS);
				fflush(file);
			}
			else if(strncmp(lineBuffer, "$GPGGA",6) == 0) {
				strcpy(lastGPS,lineBuffer);
		    	//printf("%s",lineBuffer);
			}
		}
	}
}

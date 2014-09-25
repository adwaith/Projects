#include<bmac.h>
#define MAC_ADDR 2
uint8_t activate_uart_g; 
uint8_t connection_g; 
uint8_t ack_received_g; 
uint8_t discover_pid_g; 
uint8_t rx_pid_g; 
uint8_t retrans_pid_g; 
int16_t data_index_g; 
uint8_t recepient_mac_g;
uint8_t pending_retransmit_g;
uint8_t version_g;
uint8_t log_g;

// buffers
nrk_sem_t *tx_sem;
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t uart_rx_buf[RF_MAX_PAYLOAD_SIZE];



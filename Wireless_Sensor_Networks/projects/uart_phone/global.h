#include<bmac.h>
#define MAC_ADDR 1
#define NUM_NODES 5
#define VERSION_BUF_SIZE 40
nrk_sem_t *uart_sem;
uint8_t activate_uart_g; 
nrk_sem_t *conn_sem;
uint8_t connection_g; 
nrk_sem_t *ack_sem;
uint8_t ack_received_g; 
int16_t data_index_g; 
uint8_t recepient_mac_g;
uint8_t pending_retransmit_g;
uint8_t log_g;
uint8_t request_flag_g;
uint8_t retransmit_count_g;
uint8_t vertsk_active_g;
uint8_t	recv_data_seq_g;  
// buffers
nrk_sem_t *tx_sem;
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t uart_rx_buf[RF_MAX_PAYLOAD_SIZE];

//added struct

int16_t version_g[NUM_NODES];

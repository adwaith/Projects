void req_for_next_data();
int16_t get_next_int(char*rx_buf, uint8_t* pos,uint8_t len);
void on_discover_pkt(uint8_t sender);
void onAck(uint8_t sender_mac, int16_t ack_num);
void onData(uint8_t sender, int16_t dataseq);
void onConnectionReq(uint8_t sender_mac);

#ifndef __RABIN_H_
#define __RABIN_H_

void retrieve_hash_table_from_ssd();
//void save_hash_table_to_ssd();

int rabin(char fname[MAX_PATH_LEN], int avg_seg_size, int window_size);
int read_from_proxy_file(char *fname, char *buf, off_t start_offset, size_t no_of_bytes);
void remove_from_hash_table(char *fpath);
void add_to_hash_table(char *fname, char *md5, int segment_len);
void add_to_proxy_file(char *fname, char *md5, int segment_len);
void path_to_proxypath(char *fname, char *proxy_file);

#endif
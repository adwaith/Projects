/* 
 * NAME:        Adwaith Venkataraman
 * 
 * ANDREW ID:   adwaithv
 */

#define _XOPEN_SOURCE 500

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <time.h>
#include <openssl/md5.h>
#include "cloudapi.h"
#include "dedup.h"
#include "uthash.h"
#include "cloudfs.h"
#include "zlib.h"
#include "compressapi.h"

#define UNUSED __attribute__((unused))

extern struct cloudfs_state state_; 

/* Function Details
 * 
 * Purpose:           Open a buffer to read contents from.
 *
 * Name:              put_buffer
 *
 * Input Arguments:   Character pointer to the buffer;
 *                    Buffer length.
 *
 * Return value:      Number of bytes read from the buffer.
 */

static FILE *infile;
static int put_buffer(char *buffer, int bufferLength) 
{
  fprintf(stdout, "put_buffer %d \n", bufferLength);
  return fread(buffer, 1, bufferLength, infile);
}

/* Function Details
 * 
 * Purpose:           Open a buffer to write contents to.
 *
 * Name:              get_buffer
 *
 * Input Arguments:   Character pointer to the buffer;
 *                    Buffer length.
 *
 * Return value:      Number of bytes written into the buffer.
 */

static FILE *outfile;
static int get_buffer(const char *buffer, int bufferLength) 
{
  return fwrite(buffer, 1, bufferLength, outfile);  
}

/* Function Details
 * 
 * Purpose:           Open a buffer to read contents from;
 *										Used to read the contents of the compressed segment.					
 *
 * Name:              compression_put_buffer
 *
 * Input Arguments:   Character pointer to the buffer;
 *                    Buffer length.
 *
 * Return value:      Number of bytes read from the buffer.
 */

static FILE *compression_infile;
static int compression_put_buffer(char *buffer, int bufferLength) 
{
	fprintf(stdout, "put_buffer %d \n", bufferLength);
	return fread(buffer, 1, bufferLength, compression_infile);
}

/* Function Details
 * 
 * Purpose:           Open a buffer to write contents to;
 *										Used to write contented of the compressed segment.
 *
 * Name:              compression_get_buffer
 *
 * Input Arguments:   Character pointer to the buffer;
 *                    Buffer length.
 *
 * Return value:      Number of bytes written into the buffer.
 */

static FILE *compression_outfile;
static int compression_get_buffer(const char *buffer, int bufferLength) 
{
  return fwrite(buffer, 1, bufferLength, compression_outfile);  
}

/* Structure Details
 * 
 * Purpose:           To hold each element of the hash table entry.
 *
 * Name:              my_struct
 *
 * Members:					 	MD5 hash value;
 *										Reference count;
 *										Cached flag to indicate if cached or not;
 *										Hash handle.
 */

struct my_struct 
{
	char md5[2*MD5_DIGEST_LENGTH + 1];
	int ref_count;
	int cached;
	UT_hash_handle hh;
};

/* Structure Details
 * 
 * Purpose:           Structure of segment entries in the proxy file.
 *
 * Name:              md5_in_proxy_file
 *
 * Members:					 	MD5 hash value;
 *										Segment Length.
 */

struct md5_in_proxy_file 
{
	char md5[2*MD5_DIGEST_LENGTH + 1];
	int segment_len;
};

struct my_struct *s, *hash_map = NULL;

/* Function Details
 * 
 * Purpose:           Save the hash table onto SSD, to maintain persistency;
 *										Save it to file '.hash' relative to mount point.
 *
 * Name:              save_hash_table_to_ssd
 *
 * Input Arguments:   none
 *
 * Return value:      none
 */

void save_hash_table_to_ssd()
{
	struct my_struct *p = NULL;
	char hash_table_path[MAX_PATH_LEN];
	sprintf(hash_table_path, "%s.%s", state_.ssd_path, "hash");

	FILE *fp = fopen (hash_table_path, "w");
	p = hash_map;

	if (fp == NULL)
		return;
	while (p != NULL)
	{
		fwrite(p, sizeof(struct my_struct), 1, fp);
		p = p->hh.next;
	}
	fclose(fp);
}
	
/* Function Details
 * 
 * Purpose:           Retrieve hash table from SSD, to maintain persistency;
 *										Retrieve it from file '.hash' relative to mount point.
 *
 * Name:              retrieve_hash_table_from_ssd
 *
 * Input Arguments:   none
 *
 * Return value:      none
 */

void retrieve_hash_table_from_ssd()
{
	char hash_table_path[MAX_PATH_LEN];
	sprintf(hash_table_path, "%s.%s", state_.ssd_path, "hash");

	char md5[2*MD5_DIGEST_LENGTH+1];
	struct my_struct *p = NULL;

	FILE *fp = fopen (hash_table_path, "r");
	if (fp == NULL)
		return;

	int flag = 0;
	do
	{
		p = (struct my_struct*)malloc(sizeof(struct my_struct));
		flag = fread(p, sizeof(struct my_struct), 1, fp);
		strcpy(md5, p->md5);
		
		if (flag > 0)
		{
			HASH_ADD_STR(hash_map, md5, p);
		}
		p = NULL;
	}while(flag > 0);

	fclose(fp);
}

/* Function Details
 * 
 * Purpose:           Convert SSD non-hidden path to hidden format;
 *										A '.' is appended after the last '/'.
 *
 * Name:              path_to_proxypath
 *
 * Input Arguments:   character pointer to path, relative to mount point;
 *                    character pointer to fullpath, to store the proxy path.
 *
 * Return value:      none
 */

void path_to_proxypath(char *fname, char *proxy_file)
{
	int flag, i = 0;
	int j;
	flag = -1;
	while (fname[i] != '\0')
	{
		if (fname[i] == '/')
			flag = i;
		i++;
	}
	strcpy(proxy_file, fname);
	for (j=i+1; j > flag+1; j--)
	{
		proxy_file[j] = fname[j-1];
	}
	proxy_file[flag+1] = '.';
	proxy_file[i+2] = '\0';
}

/* Function Details
 * 
 * Purpose:           Update the hash table entries;
 *										If entry exists, increment reference count;
 *										Otherwise, add a new entry.
 *
 * Name:              add_to_hash_table
 *
 * Input Arguments:   character pointer to path, relative to mount point;
 *                    character pointer to the md5 hash value;
 *										Value of the segment length.
 *
 * Return value:      none
 */

void add_to_hash_table(char *fname UNUSED, char *md5hash, int segment_len)
{
	char compressed_md5_path[MAX_PATH_LEN];
	struct my_struct *p = NULL;
	HASH_FIND_STR(hash_map, md5hash, p);
	
	if (p)
	{
		p->ref_count ++;
		fseek(infile, segment_len, SEEK_CUR);
	}

	else
	{
		p = (struct my_struct*)malloc(sizeof(struct my_struct));
		strncpy(p->md5, md5hash, 2*MD5_DIGEST_LENGTH + 1);
		p->ref_count = 1;
		p->cached = 0;

		if (state_.no_compress == 0)
		{
			sprintf(compressed_md5_path, "%s.temp_compressed_file", state_.ssd_path);
			compression_infile = fopen(compressed_md5_path, "w");
			def(infile, compression_infile, segment_len, Z_DEFAULT_COMPRESSION);
			fclose(compression_infile);

			struct stat compressed_file_statbuf;
			lstat(compressed_md5_path, &compressed_file_statbuf);

			compression_infile = fopen(compressed_md5_path, "r");
			cloud_put_object("test", md5hash, compressed_file_statbuf.st_size, \
				compression_put_buffer);
			fclose(compression_infile);
			unlink(compressed_md5_path);
		}
		else
			cloud_put_object("test", md5hash, segment_len, put_buffer);
		
		HASH_ADD_STR(hash_map, md5, p);
	}
	p = NULL;
	save_hash_table_to_ssd();
}

/* Function Details
 * 
 * Purpose:           Update the proxy file of the actual file;
 *										Appends the new segment hash and the length.
 *
 * Name:              add_to_proxy_file
 *
 * Input Arguments:   character pointer to path, relative to mount point;
 *                    character pointer to the md5 hash value;
 *										Value of the segment length.
 *
 * Return value:      none
 */

void add_to_proxy_file(char *fname, char *md5, int segment_len)
{
	char proxy_file[MAX_PATH_LEN];
	path_to_proxypath(fname, proxy_file);
	
	struct md5_in_proxy_file proxy_md5_entry;
	proxy_md5_entry.segment_len = segment_len;
	strcpy(proxy_md5_entry.md5, md5);

	FILE *fp;
	fp = fopen(proxy_file, "a");
	
	fwrite(&proxy_md5_entry, sizeof(struct md5_in_proxy_file), 1, fp);
	fclose(fp);
}

/* Function Details
 * 
 * Purpose:           Update the hash table when unlinking a file;
 *										If reference count of hash is 1, delete from cloud;
 *										If cached and ref count is 1, remove from cache;
 *										If reference count > 1, decrement it by 1;
 *										Save the hash table to SSD for persistency.
 *
 * Name:              remove_from_hash_table
 *
 * Input Arguments:   character pointer to path, relative to mount point.
 *
 * Return value:      none
 */

void remove_from_hash_table(char *fname)
{
	struct my_struct *p = NULL;
	char proxy_file[MAX_PATH_LEN];
	path_to_proxypath(fname, proxy_file);
	char md5_path[MAX_PATH_LEN];
	char md5_path_final[MAX_PATH_LEN];

	sprintf(md5_path, "%s", state_.ssd_path);

	struct md5_in_proxy_file proxy_md5_entry;
	FILE *fp = fopen(proxy_file, "rb");

	int attribute_return;
	off_t file_size = 0;

	int flag = fread(&proxy_md5_entry, sizeof(struct md5_in_proxy_file), 1, fp);

	while (flag > 0)
	{
		HASH_FIND_STR(hash_map, proxy_md5_entry.md5, p);
	
		if (p)
		{
			if (p->ref_count == 1)
			{
				HASH_DEL(hash_map, p);
				cloud_delete_object("test", proxy_md5_entry.md5);
				free(p);
				
				if (state_.no_cache == 0)
				{
					strcat(md5_path, ".cache/");
					if (p->cached == 1)
					{
						attribute_return = lgetxattr(md5_path, "user.adw", &file_size, \
							sizeof(off_t));
						file_size -= proxy_md5_entry.segment_len;
	  				lsetxattr(md5_path, "user.adw", &file_size, sizeof(off_t), 0);
	  			}
				}
				strcat(md5_path, proxy_md5_entry.md5);
				path_to_proxypath(md5_path, md5_path_final);
				unlink(md5_path_final);
			}
			else
			{
				p->ref_count--;
			}
		}
		p = NULL;
		flag = fread(&proxy_md5_entry, sizeof(struct md5_in_proxy_file), 1, fp);
	}
	save_hash_table_to_ssd();
	unlink(proxy_file);
}

/* Function Details
 * 
 * Purpose:           Update the cache by unlinking hash segments when 
 *										space is required for another hash segment.
 *
 * Policy Followed:		Find hash segment with lowest reference count;
 *										If two records with same ref count, find one with earlier
 *										access time;
 *										If same access time, find which is larger in size.
 *
 * Name:              unlink_cached_hash
 *
 * Input Arguments:   none
 *
 * Return value:      none
 */

void unlink_cached_hash()
{
	struct my_struct *p = NULL;
	struct stat statbuf_p, statbuf_temp;
	char md5hash [2*MD5_DIGEST_LENGTH + 1];
	char p_md5_path_on_cache [MAX_PATH_LEN];
	char temp_md5_path_on_cache [MAX_PATH_LEN];
	int md5_ref_count = 1;
	p = hash_map;
	char cache_path[MAX_PATH_LEN];
	sprintf(cache_path, "%s.cache/", state_.ssd_path);
	off_t file_size = 0;
	lgetxattr(cache_path, "user.adw", &file_size, sizeof(off_t));
	int cache_flag = 1;
	
	while (p != NULL)
	{
		if (p->cached == 1)
		{
			if (cache_flag == 1)
			{
				strcpy(md5hash, p->md5);
				md5_ref_count = p->ref_count;
				cache_flag = 0;
			}
			else
			{
				if (p->ref_count < md5_ref_count)
				{
					strcpy(md5hash, p->md5);
					md5_ref_count = p->ref_count;
				}
				else if (p->ref_count == md5_ref_count)
				{
					sprintf(p_md5_path_on_cache, "%s.cache/.", state_.ssd_path);
					strcat(p_md5_path_on_cache, p->md5);
					lstat(p_md5_path_on_cache, &statbuf_p);

					sprintf(temp_md5_path_on_cache, "%s.cache/.", state_.ssd_path);
					strcat(temp_md5_path_on_cache, md5hash);
					lstat(temp_md5_path_on_cache, &statbuf_temp);

					if (statbuf_p.st_atime < statbuf_temp.st_atime)
					{
						strcpy(md5hash, p->md5);
						md5_ref_count = p->ref_count;				
					}
					else if (statbuf_p.st_atime == statbuf_temp.st_atime)
					{
						if (statbuf_p.st_size > statbuf_temp.st_size)
						{
							strcpy(md5hash, p->md5);
							md5_ref_count = p->ref_count;					
						}
					}
				}
			}
		}	
		p = p->hh.next;
	}
	p = NULL;
	HASH_FIND_STR(hash_map, md5hash, p);
	p->cached = 0;
	save_hash_table_to_ssd();
	sprintf(temp_md5_path_on_cache, "%s.cache/.", state_.ssd_path);
	strcat(temp_md5_path_on_cache, md5hash);
	lstat(temp_md5_path_on_cache, &statbuf_temp);
	file_size -= statbuf_temp.st_size;
	lsetxattr(cache_path, "user.adw", &file_size, sizeof(off_t), 0);
	unlink(temp_md5_path_on_cache);		
}

/* Function Details
 * 
 * Purpose:           Depending on the length of the segment coming in
 *										that much of space will be made in the cache by evicting
 *										previously fetched/stored hash segments.
 *
 * Name:              cache_eviction
 *
 * Input Arguments:   segment length of incoming hash segment.
 *
 * Return value:      none
 */

void cache_eviction(int segment_len)
{
	char cache_path[MAX_PATH_LEN];
	sprintf(cache_path, "%s.cache/", state_.ssd_path);
	off_t file_size = 0;
	int attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
		sizeof(off_t));
				
	int req_space = (file_size + segment_len) - state_.cache_size;

	while (req_space >= 0)
	{
		unlink_cached_hash();
		attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
			sizeof(off_t));
		req_space = (file_size + segment_len) - state_.cache_size;			
	}
}

/* Function Details 
 *
 * Purpose:           Reads data segment-wise from proxy file and segments;
 *										Calculates starting segment based on overall offset
 *										and also computes local offset within that segment;
 *										If cached, opens it from SSD and reads into buffer;
 *										If not cached, fetches from cloud;
 *										If compressed, decompress and read into buffer;
 *										Whenever fetching from cloud, and if caching is required
 *										If sufficient space not available in cache, perform
 *										eviction until enough space is available.
 *
 * Name:              read_from_proxy_file
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *                    Pointer to buffer to read data into;
 *                    Offset within the file to read from;
 *                    Number of bytes to read from file.
 *
 * Return value:      Number of bytes read.
 */

int read_from_proxy_file(char *fname, char *buf, off_t start_offset, \
	size_t no_of_bytes)
{
	if (fname[0] == '\0')
	{
		return 0;
	}
	char proxy_file[MAX_PATH_LEN];
	path_to_proxypath(fname, proxy_file);
	char compressed_md5_path[MAX_PATH_LEN];
			
	int local_offset;
	int fread_ret = 0;
	int pread_ret = 0;
	int pread_ret_cumulative = 0;
	int bytes_to_read = (int)no_of_bytes;

	char md5_path[MAX_PATH_LEN];
	char md5_path_final[MAX_PATH_LEN];
	char cache_path[MAX_PATH_LEN];

	sprintf(md5_path, "%s", state_.ssd_path);

	local_offset = (int)start_offset;
	struct md5_in_proxy_file proxy_md5_entry;

	FILE *fp;
	fp = fopen(proxy_file, "r");

	while (1)
	{
		fread_ret = fread(&proxy_md5_entry, sizeof(struct md5_in_proxy_file), 1, \
			fp);
		local_offset -= proxy_md5_entry.segment_len;

		if (local_offset < 0)
		{
			local_offset += proxy_md5_entry.segment_len;
			break;
		}
	}

	int flag = 1;
	int attribute_return;
	off_t file_size = 0;
	struct my_struct *p = NULL;

	if (state_.no_cache == 0)
	{
		strcat(md5_path, ".cache/");
		strcpy(cache_path, md5_path);
	}
	strcat(md5_path, proxy_md5_entry.md5);
	path_to_proxypath(md5_path, md5_path_final);

	if (access(md5_path_final, F_OK) == -1)
	{
		if (state_.no_compress == 0)
		{
			if (state_.no_cache == 0)
			{
				attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
					sizeof(off_t));
				if ((file_size + proxy_md5_entry.segment_len) >= state_.cache_size)
				{
					cache_eviction(proxy_md5_entry.segment_len);
				}
				p = NULL;
				HASH_FIND_STR(hash_map, proxy_md5_entry.md5, p);
				p->cached = 1;
				attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
					sizeof(off_t));
				file_size += proxy_md5_entry.segment_len;
				lsetxattr(cache_path, "user.adw", &file_size, sizeof(off_t), 0);
			}		
			sprintf(compressed_md5_path, "%s.temp_compressed_file", state_.ssd_path);
			compression_outfile = fopen(compressed_md5_path, "wb");
			cloud_get_object("test", proxy_md5_entry.md5, compression_get_buffer);
			fclose(compression_outfile);
			compression_outfile = fopen(compressed_md5_path, "r");
		
			outfile = fopen(md5_path_final, "wb");
			inf(compression_outfile, outfile);
			fclose(compression_outfile);
			fclose(outfile);
			unlink(compressed_md5_path);
		}
		else
		{
			if (state_.no_cache == 0)
			{
				attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
					sizeof(off_t));
				if ((file_size + proxy_md5_entry.segment_len) >= state_.cache_size)
				{
					cache_eviction(proxy_md5_entry.segment_len);
				}
				p = NULL;
				HASH_FIND_STR(hash_map, proxy_md5_entry.md5, p);
				p->cached = 1;
				attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
					sizeof(off_t));
				file_size += proxy_md5_entry.segment_len;
				lsetxattr(cache_path, "user.adw", &file_size, sizeof(off_t), 0);
			}	
			outfile = fopen(md5_path_final, "wb");
			cloud_get_object("test", proxy_md5_entry.md5, get_buffer);
			fclose(outfile);
		}
	}
	struct stat statbuf;
	int fd = open(md5_path_final, O_RDONLY);
	int buf_offset = 0;

	while (bytes_to_read > 0 && fread_ret > 0)
	{
		if (flag == 1)
		{	
			lstat(md5_path_final, &statbuf);
			pread_ret = pread(fd, buf, bytes_to_read, local_offset);
			flag = 0;
		}
		else
		{ 
			pread_ret = pread(fd, buf+buf_offset, bytes_to_read, 0);
		}
		pread_ret_cumulative += pread_ret;
		bytes_to_read -= pread_ret;
		buf_offset += pread_ret;
		close(fd);

		fread_ret = fread(&proxy_md5_entry, sizeof(struct md5_in_proxy_file), 1, \
			fp);
		if (fread_ret <= 0 || bytes_to_read <= 0)
		{
			break;
		}
		sprintf(md5_path, "%s", state_.ssd_path);
		if (state_.no_cache == 0)
		{
			strcat(md5_path, ".cache/");
			strcpy(cache_path, md5_path);
		}
		strcat(md5_path, proxy_md5_entry.md5);
		path_to_proxypath(md5_path, md5_path_final);

		if (access(md5_path_final, F_OK) == -1)
		{
			if (state_.no_compress == 0)
			{
				if (state_.no_cache == 0)
				{
					attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
						sizeof(off_t));
					if ((file_size + proxy_md5_entry.segment_len) >= state_.cache_size)
					{
						cache_eviction(proxy_md5_entry.segment_len);
					}
					p = NULL;
					HASH_FIND_STR(hash_map, proxy_md5_entry.md5, p);
					p->cached = 1;
					attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
						sizeof(off_t));
					file_size += proxy_md5_entry.segment_len;
					lsetxattr(cache_path, "user.adw", &file_size, sizeof(off_t), 0);
				}
				sprintf(compressed_md5_path, "%s.temp_compressed_file", \
					state_.ssd_path);
				compression_outfile = fopen(compressed_md5_path, "wb");
				cloud_get_object("test", proxy_md5_entry.md5, compression_get_buffer);
				fclose(compression_outfile);
				compression_outfile = fopen(compressed_md5_path, "r");
		
				outfile = fopen(md5_path_final, "wb");
				inf(compression_outfile, outfile);
				fclose(compression_outfile);
				fclose(outfile);
				unlink(compressed_md5_path);
			}
			else
			{
				if (state_.no_cache == 0)
				{
					attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
						sizeof(off_t));
					if ((file_size + proxy_md5_entry.segment_len) >= state_.cache_size)
					{
						cache_eviction(proxy_md5_entry.segment_len);
					}
					p = NULL;
					HASH_FIND_STR(hash_map, proxy_md5_entry.md5, p);
					p->cached = 1;
					attribute_return = lgetxattr(cache_path, "user.adw", &file_size, \
						sizeof(off_t));
					file_size += proxy_md5_entry.segment_len;
					lsetxattr(cache_path, "user.adw", &file_size, sizeof(off_t), 0);
				}
				outfile = fopen(md5_path_final, "wb");
				cloud_get_object("test", proxy_md5_entry.md5, get_buffer);
				fclose(outfile);
			}
		}
		fd = open(md5_path_final, O_RDONLY);
	}

	fclose(fp);
	return pread_ret_cumulative;
}

/* Function Details
 * 
 * Purpose:           Rabinizes the file based on content and generates
 *										segments;
 *										These segment entries are updated in the hash table.
 *
 * Name:              rabin
 *
 * Input Arguments:   Pointer to the path relative to mount point;
 *										Average segment size;
 *										Window size.
 *
 * Return value:      Zero
 */

int rabin(char fname[MAX_PATH_LEN], int avg_seg_size, int window_size)
{
	log_open();
  
	int min_seg_size = 2048;
	int max_seg_size = 8192;
	int fd;

	if (fname[0]) 
	{
		fd = open(fname, O_RDONLY);
		if (fd == -1) 
		{
			exit(2);
		}
	} 
	else 
	{
		fd = STDIN_FILENO;
	}

	rabinpoly_t *rp = rabin_init( window_size, avg_seg_size, 
								  min_seg_size, max_seg_size);

	if (!rp) 
	{
		exit(1);
	}

	MD5_CTX ctx;
	unsigned char md5[MD5_DIGEST_LENGTH];	
	char temp[2 * MD5_DIGEST_LENGTH + 1];	
	int new_segment = 0;
	int len, segment_len = 0, b;
	char buf[1024];
	int bytes;
	infile = fopen(fname, "rb");
	MD5_Init(&ctx);

	while( (bytes = read(fd, buf, sizeof buf)) > 0 ) 
	{
		char *buftoread = (char *)&buf[0];
		while ((len = rabin_segment_next(rp, buftoread, bytes, 
											&new_segment)) > 0) 
		{
			MD5_Update(&ctx, buftoread, len);
			segment_len += len;
			
			if (new_segment) 
			{
				MD5_Final(md5, &ctx);

				for (b=0; b < MD5_DIGEST_LENGTH; b++)
					sprintf(&temp[b*2], "%02x", md5[b]);
	
				add_to_hash_table(fname, temp, segment_len);
				add_to_proxy_file(fname, temp, segment_len);
				MD5_Init(&ctx);
				segment_len = 0;
			}

			buftoread += len;
			bytes -= len;

			if (!bytes) 
			{
				break;
			}
		}
		if (len == -1) 
		{
			exit(2);
		}
	}
	MD5_Final(md5, &ctx);
	for (b=0; b < MD5_DIGEST_LENGTH; b++)
		sprintf(&temp[b*2], "%02x", md5[b]);
	
	add_to_hash_table(fname, temp, segment_len);
	add_to_proxy_file(fname, temp, segment_len);

	rabin_free(&rp);
	fclose(infile);
	return 0;
}
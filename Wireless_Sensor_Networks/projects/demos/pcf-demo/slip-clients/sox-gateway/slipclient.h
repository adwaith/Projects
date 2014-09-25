/******************************************************************************
 *  Nano-RK, a real-time operating system for sensor networks.
 *  SLIPstream Client
 *  Copyright (C) 2011, Real-Time and Multimedia Lab, Carnegie Mellon University
 *  All rights reserved.
 *
 *  This is the Open Source Version of Nano-RK included as part of a Dual
 *  Licensing Model. If you are unsure which license to use please refer to:
 *  http://www.nanork.org/nano-RK/wiki/Licensing
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2.0 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Contributing Authors (specific to this file):
 *  Patrick Lazik
 *******************************************************************************/

#ifndef SLIPCLIENT_H_
#define SLIPCLIENT_H_

/* SOX related inclusions */
#ifdef USE_SOX
#include <strophe.h>
#include <sox_handlers.h>
#include <common.h>
#include <sox.h>
#endif

#ifdef __LP64__ // if 64 bit environment
#define FMT_SIZE_T "li"
#else
#define FMT_SIZE_T "lli"
#endif

#define MAX_STRING_SHORT 8
#define MAX_STRING_LONG 128
#define MAX_STRING_VLONG 512

#define NONBLOCKING  0
#define BLOCKING     1
#define SLIP_KEEP_ALIVE_PERIOD 100000000

#define REFRESH 100	// Refresh rate in µs
#define POW_THRESHOLD_POWER 80000
#define POW_LOW_TO_HIGH_FACTOR 21

#define MAX_USER_INPUT_CHARS 128

#define ACTUATION_RETRIES 3

/* Logging definitions */
typedef enum {
	GW_LEVEL_ERROR, GW_LEVEL_DEBUG
} gw_log_level_t;

#define gw_debug(...) \
		do { if(_gw_log_level >= GW_LEVEL_DEBUG){ fprintf(stderr, "GW WARNING: %s:%d:%s(): ", __FILE__, \
		                        __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }} while (0)

#define gw_error(...) \
		do { if(_gw_log_level >= GW_LEVEL_ERROR){ fprintf(stderr, "GW ERROR: %s:%d:%s(): ", __FILE__, \
		                        __LINE__, __func__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}} while (0)

/* Error definitions */
enum {
	GW_OK = 1, GW_ERROR = -1, GW_ERROR_FILE_NOT_FOUND = -2
};

/* Structure definitions */
typedef struct cal_data {
	int64_t raw_val;
	float typed_val;
} cal_data_t;

typedef struct transducer {
	char *name;
	uint32_t id;
	int64_t raw_val;
	float val;
	uint32_t cal_data_len;
	cal_data_t *cal_data;
	uint8_t publish;
	UT_hash_handle hh;
} transducer_t;

typedef struct proto_transducer {
	char *name;
	uint8_t data_len;
	uint8_t data_offset;
	uint32_t cal_data_len;
	cal_data_t *cal_data;
	UT_hash_handle hh;
} proto_transducer_t;

typedef struct proto_node proto_node_t;

struct proto_node {
	uint8_t type;
	uint16_t num_transducers;
	proto_transducer_t *transducers;
	proto_node_t *next;
};

typedef struct node {
	uint8_t type;
	char *node;
	char *timestamp;
	uint32_t mac;
	uint16_t num_transducers;
	transducer_t *transducers;
	proto_node_t *proto_node;
	uint8_t publish;
	UT_hash_handle hh_node, hh_mac;
} node_t;

typedef struct parser_data{
	proto_node_t *proto_node;
	node_t *node;
} parser_data_t;

#endif /* SLIPCLIENT_H_ */

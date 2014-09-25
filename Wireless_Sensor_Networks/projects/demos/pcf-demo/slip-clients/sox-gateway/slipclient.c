/******************************************************************************
 *  Nano-RK, a real-time operating system for sensor networks.
 *  SLIPstream Client
 *  Copyright (C) 2013, Real-Time and Multimedia Lab, Carnegie Mellon University
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <slipstream.h>
#include <expat.h>
#include <pthread.h>
#include <math.h>
#include "../../common/pkt.h"
#include <slipclient.h>
#include <uthash.h>
#include <signal.h>

#define PACKET_CFG "config/packet.cfg"

#define POWER_AMMETER_NAME "RMS Ammeter"
#define POWER_POWERFACTOR_NAME "Power Factor Meter"
#define POWER_ENERGYMETER_NAME "Energy Meter"
#define POWER_APPARENTPOWER_NAME "Apparent Power Meter"
#define POWER_TRUEPOWER_NAME "True Power Meter"
#define POWER_TRUEPOWER_LOW_NAME "_true_power_low"
#define POWER_TRUEPOWER_HIGH_NAME "_true_power_high"
#define POWER_P2P_CURRENT_HIGH_NAME "_p2p_current_high"
#define POWER_P2P_CURRENT_LOW_NAME "_p2p_current_low"
#define POWER_RMS_CURRENT_HIGH_NAME "_rms_current_high"
#define POWER_RMS_CURRENT_LOW_NAME "_rms_current_low"
#define POWER_VOLTAGE_NAME "RMS Voltmeter"
#define POWER_CURRENT_CENTER "_current_center"
#define POWER_CURRENT_SELECTION_THRESHOLD 900
#define POWER_CURRENT_MEASURMENT_THRESHOLD 22

#define PAYLOAD_TYPE_POWER 1
#define PAYLOAD_TYPE_ENVIRONMENTAL 3

/* Function Prototypes */
static void XMLCALL XMLstart_read_conf_default(void *, const char *,
		const char **);
static void XMLCALL XMLstart_read_conf_optional(void *, const char *,
		const char **);
static void XMLCALL XMLstart_read_conf_mac_node(void *data,
		const char *element_name, const char **attr);
static void XMLCALL endElement(void *, const char *);
void print_usage(char *);
uint8_t read_conf(char *, void (*f)(void *, const char *, const char **));
void *slip_watcher(void *ud);
//void write_csv(uint8_t *);
void *slip_keep_alive(void *);
void *actuation_watcher(void *ud);
int outlet_actuate(uint32_t mac, uint8_t socket, uint8_t state);
int gpio_actuate(uint32_t mac, uint8_t mode,uint8_t chan, uint8_t mask);
void write_vals_csv(node_t *n);
void node_free(node_t *n);
node_t *node_new();
proto_node_t *proto_node_new();
void proto_node_free(proto_node_t *n);
proto_node_t *proto_node_tail_get(proto_node_t *n);
transducer_t *transducer_new();
void transducer_free(transducer_t *t);
proto_transducer_t *proto_transducer_new();
void proto_transducer_free(proto_transducer_t *t);
uint8_t read_cal(char* file_loc, proto_transducer_t *pt, transducer_t *t);
uint8_t publish_node_sox(sox_conn_t *conn, node_t *n);
node_t *process_pkt(PKT_T *pkt);
parser_data_t *parser_data_new();
void parser_data_free(parser_data_t *pd);
proto_node_t *proto_node_find(proto_node_t* start, uint8_t type);
void int_handler(int s);
int send_ping(uint32_t mac, uint8_t mode);
void apply_calibration(transducer_t *t, proto_transducer_t *pt, node_t *n);
void write_raw_value(PKT_T *pkt, proto_transducer_t *pt, int64_t *raw_val);
void process_transducers_power(PKT_T *pkt);

/* Global Variables */
proto_node_t *proto_nodes = NULL; // prototype node structures
node_t *nodes = NULL;
node_t *nodes_mac = NULL;
uint8_t verbose = 0;
uint8_t use_sox = 0;
#ifdef USE_SOX
sox_conn_t * global_conn = NULL;
#endif

time_t raw_time;
FILE *csv_fp;
char name[MAX_STRING_LONG] = "";
char curr_day[3] = "0";
char prev_day[3] = "0";

node_t temp_node;
transducer_t temp_transducer;

char *username = NULL;
char *csv = NULL;
char *opt = NULL;
char *mac_node = NULL;

gw_log_level_t _gw_log_level = GW_LEVEL_ERROR;

pthread_t slip_watcher_thread;

node_t *node_new() {
	node_t *n = malloc(sizeof(node_t));
	memset(n, 0, sizeof(node_t));
	return n;
}

void node_free(node_t *n) {
	transducer_t *t, *tmp;
	if (n->node != NULL )
		free(n->node);
	if (n->timestamp != NULL )
		free(n->timestamp);
	HASH_ITER(hh, n->transducers, t, tmp)
	{
		transducer_free(t);
	}
	free(n);
}

proto_node_t *proto_node_new() {
	proto_node_t *n = malloc(sizeof(proto_node_t));
	memset(n, 0, sizeof(proto_node_t));
	return n;
}

void proto_node_free(proto_node_t *n) {
	proto_transducer_t *t;
	for (t = n->transducers; t != NULL ; t = t->hh.next)
		proto_transducer_free(t);
}

proto_node_t *proto_node_tail_get(proto_node_t *n) {
	proto_node_t *tail = NULL;

	while (n != NULL ) {
		tail = n;
		n = n->next;
	}
	return tail;
}

proto_node_t *proto_node_find(proto_node_t* start, uint8_t type) {
	proto_node_t *pn = start;
	while (pn != NULL ) {
		if (pn->type == type)
			return pn;
		else
			pn = pn->next;
	}
	return pn;
}

transducer_t *transducer_new() {
	transducer_t *t = malloc(sizeof(transducer_t));
	memset(t, 0, sizeof(transducer_t));
	return t;
}

void transducer_free(transducer_t *t) {
	if (t->name != NULL )
		free(t->name);
	free(t);
}

proto_transducer_t *proto_transducer_new() {
	proto_transducer_t *t = malloc(sizeof(proto_transducer_t));
	memset(t, 0, sizeof(proto_transducer_t));
	return t;
}

void proto_transducer_free(proto_transducer_t *t) {
	if (t->name != NULL )
		free(t->name);
	free(t);
}

parser_data_t *parser_data_new() {
	parser_data_t *pd = malloc(sizeof(parser_data_t));
	memset(pd, 0, sizeof(parser_data_t));
	return pd;
}

void parser_data_free(parser_data_t *pd) {
	free(pd);
}

/* Function to parse default configuration data */
static void XMLCALL XMLstart_read_conf_default(void *data,
		const char *element_name, const char **attr) {

	uint16_t i, cal_file_len;
	uint8_t got_type_flag = 0;
	parser_data_t *pd = (parser_data_t*) data;
	proto_node_t *pn = pd->proto_node, *temp_pn = NULL;
	proto_transducer_t *t, *temp_t = NULL;
	char *cal_file;

	if (strcmp(element_name, "payload") == 0) {
		// Create new proto_node at end of proto_nodes list
		if (proto_nodes == NULL ) {
			proto_nodes = proto_node_new();
			pd->proto_node = proto_nodes;
			pn = pd->proto_node;
		} else {
			pd->proto_node->next = proto_node_new();
			pd->proto_node = pd->proto_node->next;
			pn = pd->proto_node;
		}
		// Copy node type into pn
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "type") == 0) {
				pn->type = atoi(attr[i + 1]);
				// Check if this is a duplicate. First call to proto_node_find will return current node, second will return duplicate if it exists
				temp_pn = proto_node_find(proto_nodes, pn->type);
				temp_pn = proto_node_find(temp_pn->next, pn->type);
				if (temp_pn != NULL ) {
					gw_error("Duplicate payloads of type %u exist in %s",
							pn->type, PACKET_CFG);
					exit(1);
				}
				got_type_flag = 1;
			}
		}
		if (!got_type_flag) {
			gw_error("Payload missing type in %s, check XML", PACKET_CFG);
			exit(1);
		}
	}

	else if (strcmp(element_name, "transducer") == 0) {
		if (proto_nodes == NULL ) {
			gw_error(
					"Found transducer stanza without parent payload stanza in %s, check XML",
					opt);
			exit(1);
		}
		// Create a new proto_transducer and fill out its members
		t = proto_transducer_new();

		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "name") == 0) {
				t->name = malloc(sizeof(char) * strlen(attr[i + 1]) + 1);
				strcpy(t->name, attr[i + 1]);
			} else if (strcmp(attr_name, "dataLen") == 0) {
				t->data_len = atoi(attr[i + 1]);
			} else if (strcmp(attr_name, "dataOffset") == 0) {
				t->data_offset = atoi(attr[i + 1]);
			}
		}
		// Check if all members of t are valid
		if (t->name == NULL ) {
			gw_error(
					"Transducer in %s of payload type %d missing name, check XML",
					PACKET_CFG, pn->type);
			exit(1);
		}
		if (t->data_len == 0) {
			gw_error(
					"Transducer %s in %s of payload type %d is missing or has 0 dataLen, check XML",
					t->name, PACKET_CFG, pn->type);
			exit(1);
		}
		// Check for duplicate transducers
		HASH_FIND(hh, pn->transducers, t->name, sizeof(char) * strlen(t->name),
				temp_t);
		if (temp_t != NULL ) {
			gw_error("Duplicate transducers \"%s\" found in %s", t->name,
					PACKET_CFG);
			exit(1);
		}
		// Add t to transducers hash table
		HASH_ADD_KEYPTR(hh, pn->transducers, t->name,
				sizeof(char) * strlen(t->name), t);
		pn->num_transducers++;

		// Read config file for transducer
		// Allocate 32 extra bytes for length of pn->type
		cal_file_len = sizeof(char)
				* (strlen(t->name) + strlen("cal/def//") + 33);
		cal_file = malloc(cal_file_len);
		if (snprintf(cal_file, cal_file_len, "cal/def/%u/%s.cal", pn->type,
				t->name) < 0) {
			gw_error(
					"Error forming file location of cal_file, perhaps transducer name %s is too long?",
					t->name);
			exit(1);
		}
		read_cal(cal_file, t, NULL );
		free(cal_file);
	}
}

/* Function to parse optional configuration data */
static void XMLCALL XMLstart_read_conf_optional(void *data,
		const char *element_name, const char **attr) {

	uint16_t i, cal_file_len;
	uint8_t got_id_flag = 0;
	parser_data_t *pd = (parser_data_t*) data;
	node_t *n = pd->node, *temp_n = NULL;
	proto_node_t *pn;
	transducer_t *t, *temp_t = NULL;
	proto_transducer_t *pt = NULL;
	char *cal_file;

	if (strcmp(element_name, "regMap") == 0) {
		// Find and copy node information into nodes
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "node") == 0) {
				pd->node = node_new();
				n = pd->node;
				n->node = malloc(sizeof(char) * strlen(attr[i + 1]) + 1);
				strcpy(n->node, attr[i + 1]);
			}
		}
		// Check if n's members were filled out correctly and add to nodes hash table
		if (n->node == NULL ) {
			gw_error("regMap missing node attribute in %s, check XML", opt);
			exit(1);
		} else {
			HASH_FIND(hh_node, nodes, n->node, sizeof(char) * strlen(n->node),
					temp_n);
			if (temp_n != NULL ) {
				gw_error("Duplicate nodes \"%s\" found in %s", n->node, opt);
				exit(1);
			}
			HASH_ADD_KEYPTR(hh_node, nodes, n->node,
					sizeof(char) * strlen(n->node), n);
		}

	} else if (strcmp(element_name, "transducer") == 0) {
		if (n == NULL ) {
			gw_error(
					"Found transducer stanza without parent regMap stanza in %s, check XML",
					opt);
			exit(1);
		}
		// Create new transducer struct and fill its members
		t = transducer_new();
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			if (strcmp(attr_name, "name") == 0) {
				t->name = malloc(sizeof(char) * strlen(attr[i + 1]) + 1);
				strcpy(t->name, attr[i + 1]);
			} else if (strcmp(attr_name, "id") == 0) {
				t->id = atoi(attr[i + 1]);
				got_id_flag = 1;
			}
		}
		// Check if t's members were filled out correctly and add to n's transducer hash table
		if (t->name == NULL ) {
			gw_error(
					"Transducer in regMap of node %s in %s missing name, check XML",
					n->node, opt);
			exit(1);
		}
		if (!got_id_flag) {
			gw_error(
					"Transducer %s in regMap of node %s in %s missing id, check XML",
					t->name, n->node, opt);
			exit(1);
		}
		HASH_FIND(hh, n->transducers, t->name, sizeof(char) * strlen(t->name),
				temp_t);
		if (temp_t != NULL ) {
			gw_error("Duplicate transducers \"%s\" found in %s", t->name, opt);
			exit(1);
		}
		// Set the publish flag on the transducer to true so that it will be published via SOX
		t->publish = 1;
		HASH_ADD_KEYPTR(hh, n->transducers, t->name,
				sizeof(char) * strlen(t->name), t);
		n->num_transducers++;

		// Read calibration file for transducer
		cal_file_len = sizeof(char)
				* (strlen(n->node) + strlen(t->name) + strlen("cal/opt//.cal")
						+ 1);
		cal_file = malloc(cal_file_len);
		if (snprintf(cal_file, cal_file_len, "cal/opt/%s/%s.cal", n->node,
				t->name) < 0) {
			gw_error(
					"Error forming file location of cal_file, perhaps transducer name %s is too long?",
					t->name);
			exit(1);
		}

		// If we can't find the calibration file, look up the default calibration values
		if (read_cal(cal_file, NULL, t) == GW_ERROR_FILE_NOT_FOUND) {
			pn = proto_node_find(proto_nodes, n->type);
			if (pn == NULL ) {
				gw_error(
						"No corresponding payload exists in %s for regMap of type %u in %s",
						PACKET_CFG, n->type, opt);
				exit(1);
			}
			HASH_FIND_STR(pn->transducers, t->name, pt);
			if (pt == NULL ) {
				gw_error(
						"No corresponding transducer exists in %s for transducer %s of regMap of type %u in %s",
						PACKET_CFG, t->name, n->type, opt);
				exit(1);
			}
			t->cal_data = pt->cal_data;
			t->cal_data_len = pt->cal_data_len;
		}
		free(cal_file);
	}
}

/* Function to parse mac address/node/type mapping */
static void XMLCALL XMLstart_read_conf_mac_node(void *data,
		const char *element_name, const char **attr) {

	uint8_t temp_type = 0, got_type_flag = 0, got_mac_flag = 0;
	uint16_t i;
	uint32_t temp_mac;
	node_t *n = NULL, *temp_n = NULL;
	proto_node_t *pn = proto_nodes;

	if (strcmp(element_name, "macMap") == 0) {
		for (i = 0; attr[i]; i += 2) {
			const char* attr_name = attr[i];
			// Read in mac address of node as hex or decimal
			if (strcmp(attr_name, "mac") == 0) {
				if (strstr(attr[i + 1], "0x") != NULL )
					sscanf(attr[i + 1], "%x", &temp_mac);
				else
					temp_mac = atoi(attr[i + 1]);
				got_mac_flag = 1;
			} else if (strcmp(attr_name, "node") == 0) {
				// Check if node was read in from optional config file
				HASH_FIND(hh_node, nodes, attr[i + 1],
						sizeof(char) * strlen(attr[i + 1]), n);
				if (n == NULL ) {
					gw_error("Node %s defined in %s not found in %s, check XML",
							attr[i + 1], mac_node, opt);
					exit(1);
				}
			} else if (strcmp(attr_name, "type") == 0) {
				temp_type = atoi(attr[i + 1]);
				got_type_flag = 1;
			}
		}
		if (n == NULL ) {
			gw_error("macMap missing node attribute in %s, check XML",
					mac_node);
			exit(1);
		}
		if (!got_mac_flag) {
			gw_error("macMap of node %s missing mac attribute in %s, check XML",
					n->node, mac_node);
			exit(1);
		}
		if (!got_type_flag) {
			gw_error(
					"macMap of node %s missing type attribute in %s, check XML",
					n->node, mac_node);
			exit(1);
		}
		n->mac = temp_mac;
		n->type = temp_type;
		// Find proto_node corresponding to type of n, and copy into proto_node member of m
		while (pn != NULL ) {
			if (pn->type == n->type) {
				n->proto_node = pn;
				break;
			} else
				pn = pn->next;
		}
		if (n->proto_node == NULL ) {
			gw_error("Type %d of node %s in %s not found in %s, check XML",
					n->type, n->node, mac_node, opt);
			exit(1);
		}
		// Check for duplicates and hash node by mac into nodes_mac table
		HASH_FIND(hh_mac, nodes_mac, &n->mac, sizeof(uint32_t), temp_n);
		if (temp_n != NULL ) {
			gw_error("Duplicate nodes of mac %u found in %s", n->mac, mac_node);
			exit(1);
		}
		HASH_ADD(hh_mac, nodes_mac, mac, sizeof(uint32_t), n);
	}
}

// XMLParser function called whenever an end of element is encountered
static void XMLCALL endElement(void *data, const char *element_name) {

}

/* Function to read calibration file into transducer or proto_transducer struct */
uint8_t read_cal(char* file_loc, proto_transducer_t *pt, transducer_t *t) {
	FILE *fp;
	uint32_t read_lines = 1;
	int16_t v;

	fp = fopen(file_loc, "r");
	if (fp == NULL ) {
		gw_debug("Calibration file not found at %s, skipping", file_loc);
		return GW_ERROR_FILE_NOT_FOUND;
	}
	// Count number of lines and allocate memory accordingly
	while (EOF != (v = fscanf(fp, "%*[^\n]"), fscanf(fp, "%*c"))) {
		if (v == EOF)
			break;
		read_lines++;
	}

	if (read_lines < 2) {
		gw_error(
				"Calibration file %s requires at least two tuples of rawVal,typedVal on separate lines",
				file_loc);
		fclose(fp);
		return GW_ERROR;
	}
	// Return to beginning of file and read in values
	rewind(fp);

	if (pt != NULL ) {
		pt->cal_data = malloc(sizeof(cal_data_t) * read_lines);
		while (!feof(fp)) {
			if (fscanf(fp, "%"FMT_SIZE_T"%*c%f\n",
					&pt->cal_data[pt->cal_data_len].raw_val,
					&pt->cal_data[pt->cal_data_len].typed_val) != 2) {
				gw_error(
						"Error on line %d of calibration file %s. File requires at least two tuples of rawVal,typedVal on separate lines",
						pt->cal_data_len + 1, file_loc);
				free(pt->cal_data);
				pt->cal_data_len = 0;
				return GW_ERROR;
			} else
				pt->cal_data_len++;
		}
	} else {
		t->cal_data = malloc(sizeof(cal_data_t) * read_lines);
		while (!feof(fp)) {
			if (fscanf(fp, "%"FMT_SIZE_T"%*c%f\n",
					&t->cal_data[t->cal_data_len].raw_val,
					&t->cal_data[t->cal_data_len].typed_val) != 2) {
				gw_error(
						"Error on line %d of calibration file %s. File requires at least two tuples of rawVal,typedVal on separate lines",
						t->cal_data_len + 1, file_loc);
				free(t->cal_data);
				t->cal_data_len = 0;
				return GW_ERROR;
			} else
				t->cal_data_len++;
		}
	}
	fclose(fp);
	gw_debug("Calibration file %s read successfully", file_loc);
	return GW_OK;
}

/* Function to read configuration files */
uint8_t read_conf(char* file_loc,
		void (*parser)(void *, const char *, const char **)) {
	FILE * pFile;
	char Buff[512];
	XML_Parser p = XML_ParserCreate(NULL );
	parser_data_t *pd = parser_data_new();
	sox_response_t *response;
	sox_packet_t *pkt;
	sox_affiliation_t *aff;
	node_t *n;

	if (!p) {
		gw_error("Couldn't allocate memory for XML parser");
		return GW_ERROR;
	}

	XML_SetElementHandler(p, parser, endElement);
// Pass reused variable(s) into parsing function as parser_data_t struct
	XML_SetUserData(p, pd);

	pFile = fopen(file_loc, "r");
	if (pFile == NULL ) {
		gw_error("Error opening file %s", file_loc);
		return GW_ERROR;
	}

	for (;;) {
		int done;
		int len;

		if (fgets(Buff, sizeof(Buff), pFile) != NULL ) {
			len = strlen(Buff);
			if (ferror(pFile)) {
				gw_error("XML read error in %s", file_loc);
				return GW_ERROR;
			}
			done = feof(pFile);
			if (!XML_Parse(p, Buff, len, done)) {
				gw_error("In config file %s: Parse error at line %d:\n%s",
						file_loc, (int ) XML_GetCurrentLineNumber(p),
						XML_ErrorString(XML_GetErrorCode(p)));
				return GW_ERROR;
			}

			if (done)
				break;
		} else
			break;
	}

	XML_ParserFree(p);
	parser_data_free(pd);

	if (opt != NULL && parser == XMLstart_read_conf_default)
		return read_conf(opt, XMLstart_read_conf_optional);
	else if (mac_node == NULL && parser == XMLstart_read_conf_optional) {
		gw_error(
				"Missing mac_node.cfg file required when opt.cfg is specified");
		return GW_ERROR;
	}
#ifdef USE_SOX
	else if (mac_node != NULL && parser == XMLstart_read_conf_optional)
		return read_conf(mac_node, XMLstart_read_conf_mac_node);

	if (username != NULL ) {
		// Check if we have publishing privileges to all nodes in opt.cfg
		response = sox_response_new();
		sox_acl_affiliations_query(global_conn, NULL, response);
		if (response->response_type == SOX_RESPONSE_PACKET) {
			pkt = (sox_packet_t*) response->response;
			aff = (sox_affiliation_t*) pkt->payload;
			for (n = nodes; n != NULL ; n = n->hh_node.next) {
				if (n->node != NULL ) {
					while (aff != NULL ) {

						if (strcmp(aff->affiliation, n->node) == 0) {
							if (aff->type == SOX_AFFILIATION_OWNER
									|| aff->type == SOX_AFFILIATION_PUBLISHER
									|| aff->type
											== SOX_AFFILIATION_PUBLISH_ONLY) {
								n->publish = 1;
								aff = (sox_affiliation_t*) pkt->payload;
								break;
							}
						} else
							aff = aff->next;

					}
					if (aff == NULL ) {
						gw_error(
								"No publishing privileges for node %s, incoming data will not be published",
								n->node);
					}
				}
			}

		} else
			gw_error(
					"Unable to verify publishing privileges for nodes in %s, proceeding anyway",
					opt);
		sox_response_free(response);
	}
#endif

	return GW_OK;
}

node_t *process_pkt(PKT_T *pkt) {
	node_t *n = NULL;
	transducer_t *t = NULL;
	proto_node_t *pn = NULL;
	proto_transducer_t *pt = NULL;
	char *timestamp = NULL;

	// Sanity checks on received packet
	if (pkt->payload_len < MIN_PAYLOAD || pkt->payload_len > MAX_PAYLOAD) {
		gw_error("Payload not within expected length");
		return NULL ;
	}
	// Check if payload type is known, else return error
	pn = proto_node_find(proto_nodes, pkt->payload[PAYLOAD_TYPE_INDEX]);
	if (pn == NULL ) {
		gw_error("Unknown payload type %u received from node with MAC %u",
				pkt->payload[PAYLOAD_TYPE_INDEX], pkt->src_mac);
		return NULL ;
	}

	// Find node in nodes_mac table and add it if it isn't there
	HASH_FIND(hh_mac, nodes_mac, &pkt->src_mac, sizeof(uint32_t), n);
	if (n == NULL ) {
		gw_debug("Received packet from new node with MAC %u, adding node table",
				pkt->src_mac);
		n = node_new();
		n->mac = pkt->src_mac;
		n->proto_node = pn;
		n->type = pkt->payload[PAYLOAD_TYPE_INDEX];

		// Copy transducer data from prototype nodes
		for (pt = pn->transducers; pt != NULL ; pt = pt->hh.next) {
			t = transducer_new();
			t->name = malloc(sizeof(char) * strlen(pt->name) + 1);
			strcpy(t->name, pt->name);
			HASH_ADD_KEYPTR(hh, n->transducers, t->name,
					sizeof(char) * strlen(t->name), t);
		}
		HASH_ADD(hh_mac, nodes_mac, mac, sizeof(uint32_t), n);
	}
	// Update timestamp
	timestamp = sox_timestamp_create();
	if (n->timestamp != NULL )
		free(n->timestamp);
	n->timestamp = timestamp;

	// Write RSSI value (containted in pkt and not pkt->payload
	HASH_FIND_STR(n->transducers, "RSSI", t);
	t->raw_val = (int64_t) pkt->rssi;
	t->val = (float) pkt->rssi;

	// Write remaining transducer values
	for (t = n->transducers; t != NULL ; t = t->hh.next) {
		HASH_FIND_STR(pn->transducers, t->name, pt);
		// If transducer value is not defined in packet.cfg, skip it for now. This is probably a calculated value which will be filled later
		if (pt != NULL ) {
			// Skip rssi since we already copied it
			if (strcmp("RSSI", t->name) == 0)
				continue;
			write_raw_value(pkt, pt, &t->raw_val);
		}
		apply_calibration(t, pt, n);
	}
	if (pkt->payload[PAYLOAD_TYPE_INDEX] == PAYLOAD_TYPE_POWER)
		process_transducers_power(pkt);

	return n;
}

/* Function to apply calibration data to raw_value to calculate typed_value */
void apply_calibration(transducer_t *t, proto_transducer_t *pt, node_t *n) {
	cal_data_t *cal;
	uint32_t cal_len, bound_l = 0, bound_r, cal_i = 0;
	int64_t curr_raw = 0;

	// Apply calibration if it exists
	if (t->cal_data == NULL ) {
		if (pt == NULL )
			return;
		cal = pt->cal_data;
		cal_len = pt->cal_data_len;
	} else {
		cal = t->cal_data;
		cal_len = t->cal_data_len;
	}

	if (cal != NULL ) {
		if (t->raw_val > cal[cal_len - 1].raw_val
				|| t->raw_val < cal[0].raw_val) {
			gw_error(
					"Raw value %"FMT_SIZE_T" of transducer %s of node with MAC %u is outside of the corresponding calibration file's range, skipping calibration",
					t->raw_val, t->name, n->mac);
			t->val = (float) t->raw_val;
		} else {
			// Binary search for calibration values
			bound_r = cal_len - 1;
			bound_l = 0;
			while (cal_len > 1) {
				cal_i = (bound_r - bound_l) / 2 + bound_l;
				curr_raw = cal[cal_i].raw_val;
				if (curr_raw > t->raw_val)
					bound_r = bound_l + cal_len / 2;
				else if (curr_raw < t->raw_val)
					bound_l = bound_r - cal_len / 2;
				else
					break;
				cal_len = cal_len / 2;
			}
			// Pick the calibration value which closest to t->raw_val, but lower than it for the left interpolation point
			if (curr_raw > t->raw_val && cal_i > 0) {
				cal_i--;
				curr_raw = cal[cal_i].raw_val;
			}
			// Perform linear interpolation
			t->val = ((t->raw_val - cal[cal_i].raw_val)
					* (float) (cal[cal_i + 1].typed_val - cal[cal_i].typed_val)
					/ (cal[cal_i + 1].raw_val - cal[cal_i].raw_val))
					+ cal[cal_i].typed_val;
		}
	}
	// If there is no calibration data available, copy the raw_value into the typed_value
	else
		t->val = (float) t->raw_val;
}

/* Function to copy raw_value from packet to transducer */
void write_raw_value(PKT_T *pkt, proto_transducer_t *pt, int64_t *raw_val) {
	uint8_t i, j;

	*raw_val = 0;
	j = pt->data_len - 1;
	for (i = pt->data_offset; i < pt->data_offset + pt->data_len; i++) {
		*raw_val |= (int64_t) pkt->payload[i] << (j * 8);
		j--;
	}
}

/* Function to calculate and write calculated transducer values from FireFly Power Meter*/
void process_transducers_power(PKT_T *pkt) {
	node_t *n = NULL;
	proto_node_t *pn = NULL;
	transducer_t *t = NULL, *t_current = NULL, *t_voltage = NULL, *t_apparent =
			NULL, *t_true = NULL;
	proto_transducer_t *pt_p2p_current_high = NULL, *pt_current = NULL,
			*pt_voltage = NULL, *pt_true = NULL;
	int16_t p2p_current_high = 0;
	uint16_t cal_file_len;
	char *cal_file = NULL;

	// Find node in nodes_mac table
	HASH_FIND(hh_mac, nodes_mac, &pkt->src_mac, sizeof(uint32_t), n);
	// Find prototype nodes for high and low gain current values
	pn = proto_node_find(proto_nodes, PAYLOAD_TYPE_POWER);
	HASH_FIND_STR(pn->transducers, POWER_P2P_CURRENT_HIGH_NAME,
			pt_p2p_current_high);

	// Extract raw value of P2P current, so that we can determine if it saturated
	write_raw_value(pkt, pt_p2p_current_high, (int64_t*) &p2p_current_high);

	// Determine which RMS current value to select, copy it and apply calibration
	if (p2p_current_high >= POWER_CURRENT_SELECTION_THRESHOLD) {
		// Check if we already have a current transducer for this node, add one if not
		HASH_FIND_STR(n->transducers, POWER_RMS_CURRENT_LOW_NAME, t_current);
		if (t_current == NULL ) {
			t_current = transducer_new();
			t_current->name = malloc(
					sizeof(char) * strlen(POWER_RMS_CURRENT_LOW_NAME) + 1);
			strcpy(t_current->name, POWER_RMS_CURRENT_LOW_NAME);
			HASH_ADD_KEYPTR(hh, n->transducers, t_current->name,
					sizeof(char) * strlen(t_current->name), t_current);
			// Read calibration file for transducer
			cal_file_len = sizeof(char)
					* (strlen(n->node) + strlen(t_current->name)
							+ strlen("cal/opt//") + 1);
			cal_file = malloc(cal_file_len);
			if (snprintf(cal_file, cal_file_len, "cal/opt/%s/%s.cal", n->node,
					t_current->name) < 0) {
				gw_error(
						"Error forming file location of cal_file, perhaps transducer name %s is too long?",
						t_current->name);
			}
			read_cal(cal_file, NULL, t_current);
			free(cal_file);
		}
		HASH_FIND_STR(pn->transducers, POWER_RMS_CURRENT_LOW_NAME, pt_current);

		// Check if we already have a true power transducer for this node, add one if not
		HASH_FIND_STR(n->transducers, POWER_TRUEPOWER_LOW_NAME, t_true);
		if (t_true == NULL ) {
			t_true = transducer_new();
			t_true->name = malloc(
					sizeof(char) * strlen(POWER_TRUEPOWER_LOW_NAME) + 1);
			strcpy(t_true->name, POWER_TRUEPOWER_LOW_NAME);
			HASH_ADD_KEYPTR(hh, n->transducers, t_true->name,
					sizeof(char) * strlen(t_true->name), t_true);
			// Read calibration file for transducer
			cal_file_len = sizeof(char)
					* (strlen(n->node) + strlen(t_true->name)
							+ strlen("cal/opt//") + 1);
			cal_file = malloc(cal_file_len);
			if (snprintf(cal_file, cal_file_len, "cal/opt/%s/%s.cal", n->node,
					t_true->name) < 0) {
				gw_error(
						"Error forming file location of cal_file, perhaps transducer name %s is too long?",
						t_true->name);
			}
			read_cal(cal_file, NULL, t_true);
			free(cal_file);
		}
		HASH_FIND_STR(pn->transducers, POWER_TRUEPOWER_LOW_NAME, pt_true);

	} else {
		// Check if we already have a current transducer for this node, add one if not
		HASH_FIND_STR(n->transducers, POWER_RMS_CURRENT_HIGH_NAME, t_current);
		if (t_current == NULL ) {
			t_current = transducer_new();
			t_current->name = malloc(
					sizeof(char) * strlen(POWER_RMS_CURRENT_HIGH_NAME) + 1);
			strcpy(t_current->name, POWER_RMS_CURRENT_HIGH_NAME);
			HASH_ADD_KEYPTR(hh, n->transducers, t_current->name,
					sizeof(char) * strlen(t_current->name), t_current);
			// Read calibration file for transducer
			cal_file_len = sizeof(char)
					* (strlen(n->node) + strlen(t_current->name)
							+ strlen("cal/opt//") + 1);
			cal_file = malloc(cal_file_len);
			if (snprintf(cal_file, cal_file_len, "cal/opt/%s/%s.cal", n->node,
					t_current->name) < 0) {
				gw_error(
						"Error forming file location of cal_file, perhaps transducer name %s is too long?",
						t_current->name);
			}
			read_cal(cal_file, NULL, t_current);
			free(cal_file);
		}
		HASH_FIND_STR(pn->transducers, POWER_RMS_CURRENT_HIGH_NAME, pt_current);

		// Check if we already have a true power transducer for this node, add one if not
		HASH_FIND_STR(n->transducers, POWER_TRUEPOWER_HIGH_NAME, t_true);
		if (t_true == NULL ) {
			t_true = transducer_new();
			t_true->name = malloc(
					sizeof(char) * strlen(POWER_TRUEPOWER_HIGH_NAME) + 1);
			strcpy(t_true->name, POWER_TRUEPOWER_HIGH_NAME);
			HASH_ADD_KEYPTR(hh, n->transducers, t_true->name,
					sizeof(char) * strlen(t_true->name), t_true);
			// Read calibration file for transducer
			cal_file_len = sizeof(char)
					* (strlen(n->node) + strlen(t_true->name)
							+ strlen("cal/opt//") + 1);
			cal_file = malloc(cal_file_len);
			if (snprintf(cal_file, cal_file_len, "cal/opt/%s/%s.cal", n->node,
					t_true->name) < 0) {
				gw_error(
						"Error forming file location of cal_file, perhaps transducer name %s is too long?",
						t_true->name);
			}
			read_cal(cal_file, NULL, t_true);
			free(cal_file);
		}
		HASH_FIND_STR(pn->transducers, POWER_TRUEPOWER_HIGH_NAME, pt_true);
	}

	// Write current value and apply calibration
	write_raw_value(pkt, pt_current, &t_current->raw_val);
	write_raw_value(pkt, pt_true, &t_true->raw_val);
	// Zero out negligible currents
	if (t_current->raw_val < POWER_CURRENT_MEASURMENT_THRESHOLD) {
		t_current->raw_val = 0;
		t_current->val = 0;
		t_true->raw_val = 0;
		t_true->val = 0;
	} else {
		apply_calibration(t_current, pt_current, n);
		apply_calibration(t_true, pt_true, n);
	}

	// Copy current data over into transducer which will be published (if it exists
	HASH_FIND_STR(n->transducers, POWER_AMMETER_NAME, t);
	if (t != NULL ) {
		t->raw_val = t_current->raw_val;
		t->val = t_current->val;
	}

	HASH_FIND_STR(n->transducers, POWER_TRUEPOWER_NAME, t);
	if (t != NULL ) {
		t->raw_val = t_true->raw_val;
		t->val = t_true->val;
	}

	// Check if we already have a voltage transducer for this node, add one if not
	HASH_FIND_STR(n->transducers, POWER_VOLTAGE_NAME, t_voltage);
	if (t_voltage == NULL ) {
		t_voltage = transducer_new();
		t_voltage->name = malloc(sizeof(char) * strlen(POWER_VOLTAGE_NAME) + 1);
		strcpy(t_voltage->name, POWER_VOLTAGE_NAME);
		HASH_ADD_KEYPTR(hh, n->transducers, t_voltage->name,
				sizeof(char) * strlen(t_voltage->name), t_voltage);
		// Read calibration file for transducer
		cal_file_len = sizeof(char)
				* (strlen(n->node) + strlen(t_voltage->name)
						+ strlen("cal/opt//") + 1);
		cal_file = malloc(cal_file_len);
		if (snprintf(cal_file, cal_file_len, "cal/opt/%s/%s.cal", n->node,
				t_voltage->name) < 0) {
			gw_error(
					"Error forming file location of cal_file, perhaps transducer name %s is too long?",
					t_voltage->name);
		}
		read_cal(cal_file, NULL, t_voltage);
		free(cal_file);
	}
	HASH_FIND_STR(pn->transducers, POWER_VOLTAGE_NAME, pt_voltage);

	// Write voltage value and apply calibration
	write_raw_value(pkt, pt_voltage, &t_voltage->raw_val);
	apply_calibration(t_voltage, pt_voltage, n);

	// Check if we already have an apparent power transducer for this node, add one if not
	HASH_FIND_STR(n->transducers, POWER_APPARENTPOWER_NAME, t_apparent);
	if (t_apparent == NULL ) {
		t_apparent = transducer_new();
		t_apparent->name = malloc(
				sizeof(char) * strlen(POWER_APPARENTPOWER_NAME) + 1);
		strcpy(t_apparent->name, POWER_APPARENTPOWER_NAME);
		HASH_ADD_KEYPTR(hh, n->transducers, t_apparent->name,
				sizeof(char) * strlen(t_apparent->name), t_apparent);
	}
	t_apparent->raw_val = t_current->raw_val * t_voltage->raw_val;
	t_apparent->val = t_current->val * t_voltage->val;

	// If needed, calculate power factor
	HASH_FIND_STR(n->transducers, POWER_POWERFACTOR_NAME, t);
	if (t != NULL ) {
		if (t_true->val == 0 && t_apparent->val == 0)
			t->val = 0;
		else
			t->val = t_true->val / t_apparent->val;
		// Can't have a power factor larger than 1
		if (t->val > 1) {
			t->val = 1;
		}
		t->raw_val = (int64_t) t->val;

	}
}

#ifdef USE_SOX
/* Function to publish node/transducer data to XMPP server */
uint8_t publish_node_sox(sox_conn_t *conn, node_t *n) {
	char raw_val[MAX_STRING_LONG];
	char typed_val[MAX_STRING_LONG];
	char id[MAX_STRING_LONG];
	sox_stanza_t *item;
	transducer_t *t;

	if (n->node == NULL )
		return GW_ERROR;

// Create new stanza to publish and add transducerValues
	item = sox_pubsub_item_data_new(conn);
	for (t = n->transducers; t != NULL ; t = t->hh.next) {
// Only publish transducervalue if it's publish flag is set
		if (t->publish) {
			snprintf(id, MAX_STRING_LONG, "%u", t->id);
			snprintf(raw_val, MAX_STRING_LONG, "%"FMT_SIZE_T"", t->raw_val);
			snprintf(typed_val, MAX_STRING_LONG, "%f", t->val);
			sox_item_transducer_value_add(item, NULL, t->name, id, typed_val,
					raw_val, n->timestamp);
		}
	}
// Publish and free stanza
	sox_item_publish_nonblocking(conn, item, n->node);
	sox_stanza_free(item);
	return GW_OK;
}
#endif

/* Function to write CSV data */

void write_vals_csv(node_t *n) {
	transducer_t *t;
	char tempstr[MAX_STRING_VLONG];

	time(&raw_time);
	strftime(curr_day, 100, "%d", localtime(&raw_time));

	if (atoi(curr_day) != atoi(prev_day)) {
		strcpy(prev_day, curr_day);
		strcpy(tempstr, csv);
		strftime(tempstr + strlen(tempstr), 100, "_%Y_%m_%d.out",
				localtime(&raw_time));
		if (csv_fp != NULL )
			fclose(csv_fp);
		csv_fp = fopen(tempstr, "a");
		if (csv_fp == NULL ) {
			fprintf(stderr, "Error opening or creating file %s\n", tempstr);
			exit(1);
		}
	}

	fprintf(csv_fp, "%s,%u,%u", n->timestamp, n->mac, n->type);
	if (n->node != NULL )
		fprintf(csv_fp, "%s,", n->node);
	else
		fprintf(csv_fp, ",none");

	for (t = n->transducers; t != NULL ; t = t->hh.next)
		fprintf(csv_fp, ",%"FMT_SIZE_T",%f", t->raw_val, t->val);
	fprintf(csv_fp, "\n");
	fflush(csv_fp);

}

/* Thread to send periodic messages to the SLIPstream server to keep the connection alive */
void *slip_keep_alive(void *ud) {
	int size = 0;
	PKT_T my_pkt;
	char buffer[128];
	memset(buffer, 0, 128);

	/* FIXME: This is a hack to keep SLIP alive while not crashing the master node with an invalid packet */
	my_pkt.src_mac = 0x00000000;
	my_pkt.dst_mac = 0x00000000;
	my_pkt.type = APP;
	my_pkt.payload_len = 0;
	my_pkt.rssi = 0;
	my_pkt.checksum = 0;
	size = pkt_to_buf(&my_pkt, buffer);

	while (1) {
		slipstream_send(buffer, size);
		usleep(SLIP_KEEP_ALIVE_PERIOD);
	}
	return (void*) GW_OK;
}

void *actuation_watcher(void *ud) {
	sox_data_t *data = NULL;
	sox_packet_t *packet = NULL;
	sox_transducer_value_t *tv = NULL;
	sox_response_t *response;
	node_t *n = NULL;
	transducer_t *t = NULL;
	int v;
	int err;
	//char *timestamp = NULL;

	while (1) {
		if (global_conn->xmpp_conn->state == XMPP_STATE_CONNECTED) {
			response = sox_response_new();
			err = sox_pubsub_data_receive(global_conn, response);

			if(err == SOX_ERROR_NO_RESPONSE){
				sox_response_free(response);
				continue;
			}
			/*if (timestamp != NULL )
				free(timestamp);
			timestamp = sox_timestamp_create();*/

			packet = (sox_packet_t*) response->response;
			data = (sox_data_t*) packet->payload;
			for (tv = data->transducers; tv != NULL ; tv = tv->next) {
				if (tv->type == SOX_TRANSDUCER_SET_VALUE) {
					// Search for MAC of node corresponding to its id if we haven't found it already
					HASH_FIND(hh_node, nodes, data->event,
							sizeof(char) * strlen(data->event), n);
					if (n != NULL && tv->name != NULL ) {
						if (strcmp(tv->name, "Socket State") == 0) {
							fprintf(stderr, "Got socket state transducerSetValue packet\n");
							// Actuate outlet if we can find it as a transducer of the node
							// TODO: Add actuation code for more actuator types
							HASH_FIND_STR(n->transducers, "Socket State", t);
							if (t != NULL ) {
								if (strcmp(tv->raw_value, "0") == 0) {
									outlet_actuate(n->mac, 0, 0);
									t->raw_val = 0;
									t->val = 0;
									/*if (n->timestamp == NULL )
										free(n->timestamp);
									n->timestamp = timestamp;*/
								} else if (strcmp(tv->raw_value, "1") == 0) {
									outlet_actuate(n->mac, 0, 1);
									t->raw_val = 1;
									t->val = 1;
									/*if (n->timestamp == NULL )
										free(n->timestamp);
									n->timestamp = timestamp;*/
								}
							}
						}
						else if (strcmp(tv->name, "GPIO Mask") == 0) {
								v=atoi(tv->raw_value);
								gpio_actuate(n->mac, 0,0, v);
						}
						else if (strcmp(tv->name, "GPIO Set0") == 0) {
								v=atoi(tv->raw_value);
								gpio_actuate(n->mac, 1,0, v);
						}
						else if (strcmp(tv->name, "GPIO Set1") == 0) {
								v=atoi(tv->raw_value);
								gpio_actuate(n->mac, 1,1, v);
						}
						// If we receive a ping command, send it out
						else if (strcmp(tv->name, "Ping") == 0)
							send_ping(n->mac, atoi(tv->raw_value));

					}
				}
			}
			sox_response_free(response);
		} else
			usleep(10000);
	}
	return (void*) GW_OK;
}

/* Thread to receive and process SLIPstream data */
void *slip_watcher(void *ud) {
	PKT_T pkt;
	char buffer[128];
	int8_t v;
	node_t *n;

	if (username != NULL ) {
#ifdef USE_SOX
		while (1) {
			v = slipstream_receive(buffer);
			if (v > 0) {
				buf_to_pkt(buffer, &pkt);
				n = process_pkt(&pkt);
				if (n != NULL ) {
					if (csv != NULL )
						write_vals_csv(n);
					if (n->publish)
						publish_node_sox(global_conn, n);
				}
			}
		}
#endif
	} else {
		while (1) {
			v = slipstream_receive(buffer);
			if (v > 0) {
				buf_to_pkt(buffer, &pkt);
				n = process_pkt(&pkt);
				if (n != NULL ) {
					write_vals_csv(n);
				}
			}

		}
	}
	return (void*) GW_OK;
}

/* Function to print command line usage */
void print_usage(char *prog_name) {
#if defined(USE_SOX)
	fprintf(stdout, "\"%s\" SLIPstream SOX-CSV client\n", prog_name);
	fprintf(stdout,
			"Usage: %s <-s SLIPstream_server_address> <-port SLIPstream_server_port> [-sox sox_configuration_file] [-mac_node mac_node_configuration_file] [-u XMPP username -p XMPP password] [-csv CSV output file] [-verbose]\n",
			prog_name);
#else
	fprintf(stdout, "\"%s\" SLIPstream CSV client\n", prog_name);
	fprintf(
			stdout,
			"Usage: %s <-s SLIPstream_server_address> <-port SLIPstream_server_port> [-mac_node mac_node_configuration_file] [-csv CSV output_file] [-verbose]\n",
			prog_name);
#endif

	fprintf(stdout, "\t-s SLIPstream server IP address or domain name\n");
	fprintf(stdout, "\t-port SLIPstream server port\n");
	fprintf(stdout,
			"\t-mac_node MAC to event node mapping configuration file\n");
	fprintf(stdout, "\t-sox SOX configuration file\n");
#ifdef USE_SOX
	fprintf(stdout,
			"\t-u username = JID to authenticate (give the full JID, i.e. user@domain)\n");
	fprintf(stdout, "\t-p password = JID user password\n");
#endif
	fprintf(stdout,
			"\t-csv CSV file output directory, leave blank for current directory\n");
	fprintf(stdout, "\t-help = print this usage and exit\n");
	fprintf(stdout, "\t-verbose = print info\n");
}


int gpio_actuate (uint32_t mac, uint8_t mode,uint8_t chan, uint8_t value_mask)
{
  int v, size;
  PKT_T my_pkt;
  char buffer[128];

if(mode==0)
{
  my_pkt.src_mac = 0x00000000;
  my_pkt.dst_mac = mac;
  my_pkt.rssi = 0;
  my_pkt.type = APP;
  my_pkt.payload[0] = 1;        // Number of Elements
  my_pkt.payload[1] = PKT_GPIO_CTRL;        // KEY (2->actuate, 1-> Power Packet) 
  my_pkt.payload[2] = mode;   // Outlet number 0/1
  my_pkt.payload[3] = value_mask;    // Outlet state 0/1
  my_pkt.payload_len = 4;
  size = pkt_to_buf (&my_pkt, buffer);
  if(size>0 )
        v = slipstream_send (buffer, size);
  else printf( "Malformed format...\n" );
}

if(mode==1)
{
  my_pkt.src_mac = 0x00000000;
  my_pkt.dst_mac = mac;
  my_pkt.rssi = 0;
  my_pkt.type = APP;
  my_pkt.payload[0] = 1;        // Number of Elements
  my_pkt.payload[1] = PKT_GPIO_CTRL;        // KEY (2->actuate, 1-> Power Packet) 
  my_pkt.payload[2] = mode;   // Outlet number 0/1
  my_pkt.payload[3] = chan;    // Outlet state 0/1
  my_pkt.payload[4] = value_mask;    // Outlet state 0/1
  my_pkt.payload_len = 5;
  size = pkt_to_buf (&my_pkt, buffer);
  if(size>0 )
        v = slipstream_send (buffer, size);
  else printf( "Malformed format...\n" );
}



  return v;
}


int outlet_actuate(uint32_t mac, uint8_t socket, uint8_t state) {
	int v, size;
	PKT_T my_pkt;
	char buffer[128];

	gw_debug("Actuating node with mac %u to state %u", mac, state);
	my_pkt.src_mac = 0x00000000;
	my_pkt.dst_mac = mac;
	my_pkt.rssi = 0;
	my_pkt.type = APP;
	my_pkt.payload[0] = 1; // Number of Elements
	my_pkt.payload[1] = 2; // KEY (2->actuate, 1-> Power Packet)
	my_pkt.payload[2] = socket; // Outlet number 0/1
	my_pkt.payload[3] = state; // Outlet state 0/1
	my_pkt.payload_len = 4;
	size = pkt_to_buf(&my_pkt, buffer);
	v = slipstream_send(buffer, size);
	return v;
}

int send_ping(uint32_t mac, uint8_t mode) {
	int v, size;
	PKT_T my_pkt;
	char buffer[128];
	if (mode > 3) {
		gw_error("Unsupported ping mode %u cannot be sent to node with mac %u",
				mode, mac);
		return GW_ERROR;
	}
// Send a ping at startup
	my_pkt.src_mac = 0x00000000;
	my_pkt.dst_mac = mac;
	my_pkt.type = PING;
	my_pkt.payload[0] = mode;
	my_pkt.payload_len = 1;
	size = pkt_to_buf(&my_pkt, buffer);
	v = slipstream_send(buffer, size);
	if (v == 0)
		gw_error("Error sending actuation command to node with mac %u", mac);
	return GW_OK;
}

// Handler to catch SIGINT (Ctrl+C)
void int_handler(int s) {
// Set our presence status to unavailable and disconnect xmpp connection
	if (global_conn != NULL ) {
		sox_disconnect(global_conn);
		sox_conn_free(global_conn);
	}
	exit(1);
}

/* Main */
int main(int argc, char *argv[]) {
	pthread_t slip_ka, actuation_watcher_thread;
	int err = 0;
	uint8_t current_arg_num = 1;
	uint16_t slip_port = 0;
	char *current_arg_name = NULL;
	char *current_arg_val = NULL;
	char *password = NULL;
	char *slip_server = NULL;
	struct sigaction sig_int_handler;
	pthread_attr_t attr;
	sox_conn_t *conn = sox_conn_new();

// Add SIGINT handler
	sig_int_handler.sa_handler = int_handler;
	sigemptyset(&sig_int_handler.sa_mask);
	sig_int_handler.sa_flags = 0;
	sigaction(SIGINT, &sig_int_handler, NULL );

	if (argc == 1) {
		print_usage(argv[0]);
		return GW_ERROR;
	}

	if (!strcmp(argv[1], "-help")) {
		print_usage(argv[0]);
		return GW_ERROR;
	}

	while (current_arg_num < argc) {
		current_arg_name = argv[current_arg_num++];

		if (strcmp(current_arg_name, "-help") == 0) {
			print_usage(argv[0]);
			return GW_ERROR;
		}
		if (strcmp(current_arg_name, "-verbose") == 0) {
			verbose = 1;
			continue;
		}
		if (current_arg_num == argc) {
			print_usage("");
			return GW_ERROR;
		}

		current_arg_val = argv[current_arg_num++];

		if (strcmp(current_arg_name, "-u") == 0) {
			username = current_arg_val;
		} else if (strcmp(current_arg_name, "-p") == 0) {
			password = current_arg_val;
		} else if (strcmp(current_arg_name, "-s") == 0) {
			slip_server = current_arg_val;
		} else if (strcmp(current_arg_name, "-mac_node") == 0) {
			mac_node = current_arg_val;
		} else if (strcmp(current_arg_name, "-sox") == 0) {
			opt = current_arg_val;
		} else if (strcmp(current_arg_name, "-port") == 0) {
			slip_port = atoi(current_arg_val);
		} else if (strcmp(current_arg_name, "-csv") == 0) {
			csv = current_arg_val;
		} else {
			fprintf(stderr, "Unknown argument: %s\n", current_arg_name);
			print_usage(argv[0]);
			return GW_ERROR;
		}
	}
	if (username == NULL && password != NULL ) {
		fprintf(stderr, "Username missing\n");
		print_usage(argv[0]);
		return GW_ERROR;
	} else if (username != NULL && password == NULL ) {
		fprintf(stdout, "%s's ", username);
		fflush(stdout);
		password = getpass("password: ");
		if (password == NULL ) {
			fprintf(stderr, "Invalid password\n");
			print_usage(argv[0]);
			return GW_ERROR;
		}
	} else if (slip_server == NULL ) {
		fprintf(stderr, "SLIPstream server address missing\n");
		print_usage(argv[0]);
		return GW_ERROR;
	} else if (slip_port == 0) {
		fprintf(stderr, "SLIPstream server port\n");
		print_usage(argv[0]);
		return GW_ERROR;
	}

#ifdef USE_SOX
	else if (csv == NULL && username == NULL ) {
		fprintf(stderr,
				"Either SOX and/or CSV output functionality is required\n");
		print_usage(argv[0]);
		return GW_ERROR;
	}
#else
	else if (csv == NULL) {
		fprintf(stderr,
				"CSV output functionality is required\n");
		print_usage(argv[0]);
		return return GW_ERROR;
	}
#endif

	slipstream_open(slip_server, slip_port, BLOCKING);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&slip_ka, &attr, slip_keep_alive, NULL );

#ifdef USE_SOX
	if (username != NULL ) {
		if (verbose == 0)
			err = sox_connect(username, password, SOX_LEVEL_ERROR, NULL, NULL,
					conn);
		else
			err = sox_connect(username, password, SOX_LEVEL_DEBUG, NULL, NULL,
					conn);
	}
	if (err != SOX_OK) {
		gw_error("Error connecting to XMPP server");
		return GW_ERROR;
	}
	global_conn = conn;

	if ((err = read_conf(PACKET_CFG, XMLstart_read_conf_default)) == GW_OK) {
		pthread_create(&actuation_watcher_thread, &attr, actuation_watcher,
				NULL );
		pthread_attr_destroy(&attr);
// Run slip_watcher function to receive and process data from slipstream
		slip_watcher(NULL );
	} else {
		sox_disconnect(conn);
		sox_conn_free(conn);
		return GW_ERROR;
	}

#else
	if ((err = read_conf(PACKET_CFG, XMLstart_read_conf_default)) == GW_OK) {
		pthread_create(&actuation_watcher_thread, &attr, actuation_watcher,
				NULL );
		pthread_attr_destroy(&attr);
// Run slip_watcher function to receive and process data from slipstream
		slip_watcher(NULL );
	}
	else
	return GW_ERROR;
#endif

	return err;
}


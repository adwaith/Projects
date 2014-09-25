#ifndef _POWER_VARS_H_
#define _POWER_VARS_H_


#define DEFAULT_POWER_THRESH	(uint32_t)25000

// Define this if using an external two pin dual color LED
//#define TWO_PIN_LED

#define EEPROM_STATE_ADDR	0x100
#define EEPROM_CAL_V_MSB_ADDR	0x101
#define EEPROM_CAL_V_LSB_ADDR	0x102
#define EEPROM_CAL_C1_MSB_ADDR	0x103
#define EEPROM_CAL_C1_LSB_ADDR	0x104
#define EEPROM_CAL_C2_MSB_ADDR	0x105
#define EEPROM_CAL_C2_LSB_ADDR	0x106
#define EEPROM_ENERGY1_0_ADDR	0x107
#define EEPROM_ENERGY1_1_ADDR	0x108
#define EEPROM_ENERGY1_2_ADDR	0x109
#define EEPROM_ENERGY1_3_ADDR	0x10a
#define EEPROM_ENERGY1_4_ADDR	0x10b
#define EEPROM_ENERGY1_5_ADDR	0x10c
#define EEPROM_ENERGY1_6_ADDR	0x10d
#define EEPROM_ENERGY1_7_ADDR	0x10e
#define EEPROM_ENERGY2_0_ADDR	0x10f
#define EEPROM_ENERGY2_1_ADDR	0x110
#define EEPROM_ENERGY2_2_ADDR	0x111
#define EEPROM_ENERGY2_3_ADDR	0x112
#define EEPROM_ENERGY2_4_ADDR	0x113
#define EEPROM_ENERGY2_5_ADDR	0x114
#define EEPROM_ENERGY2_6_ADDR	0x115
#define EEPROM_ENERGY2_7_ADDR	0x116

#define EEPROM_POWER_THRESH_ADDR_LSB 0x117
#define EEPROM_POWER_THRESH_ADDR_MID 0x118
#define EEPROM_POWER_THRESH_ADDR_MSB 0x119


#define VOLTAGE_LOW_THRESHOLD	200
#define VOLTAGE_ZERO_THRESHOLD  512	
#define CYCLE_UNKNOWN	0
#define CYCLE_LOW	1
#define CYCLE_HIGH	2

#define TRUE_POWER_ON_THRESH	1000


#define EVENT_DETECTOR_STACKSIZE  256

#define socket_0_enable()       PORTE |= 0x10 
#define socket_0_disable()      PORTE &= ~(0x10) 

#ifdef TWO_PIN_LED
extern uint8_t led_state;
#define plug_led_green_set() \
       do { if(led_state != 2){ led_state = 1; PORTE &= ~(0x04); PORTE |= 0x08;}} while (0)

#define plug_led_green_clr() \
        do{ if(led_state != 2){ led_state = 0; PORTE &= ~(0x08);}} while(0)

#define plug_led_red_set() \
        do{ led_state = 2; PORTE |= 0x04; PORTE &= ~(0x08);} while(0)

#define plug_led_red_clr() \
        do{ if(PORTE & 0x10){ led_state = 1; PORTE &= ~(0x08); plug_led_green_set();} else{ led_state = 0; PORTE &= ~(0x08);}} while(0)
#else
#define plug_led_green_set()	PORTE |= 0x08
#define plug_led_green_clr()	PORTE &= ~(0x08)

#define plug_led_red_set()	PORTE |= 0x04
#define plug_led_red_clr()	PORTE &= ~(0x04)
#endif




#define VOLTAGE_CHAN	0
#define CURRENT_LOW	1
#define CURRENT_HIGH	2

NRK_STK event_detector_stack[EVENT_DETECTOR_STACKSIZE];
nrk_task_type event_detector;
void event_detector_task();
nrk_sig_t update_energy_sig;

void event_detector_task();
void power_sample ();
void power_init ();
int8_t power_socket_enable (uint8_t socket);
int8_t power_socket_disable (uint8_t socket);
int8_t power_socket_throttle(uint8_t socket);
int8_t set_power_thresh(uint32_t power);

uint32_t true_power_thresh;

uint16_t v_center,c_center,c2_center;
uint16_t center_chan;
int16_t v,v_last;
int16_t c1,c2;
uint16_t l_v_p2p_low,l_v_p2p_high;
uint16_t  l_c_p2p_low, l_c_p2p_high;
uint16_t  l_c_p2p_low2, l_c_p2p_high2;
uint16_t v_p2p_low,v_p2p_high;
uint16_t c_p2p_low, c_p2p_high;
uint16_t c_p2p_low2, c_p2p_high2;
uint8_t socket_0_active;
uint8_t socket_1_active;
uint16_t c1_center;
uint16_t rms_current;
uint16_t rms_current2;
uint16_t rms_voltage;
uint32_t true_power;
uint32_t true_power2;
//double cummulative_energy;
//double cummulative_energy2;

union
{
	unsigned long long total;
	uint8_t byte[8];  // 0 is the LSB, 7 is the MSB
} cummulative_energy;

union
{
	unsigned long long total;
	uint8_t byte[8];  // 0 is the LSB, 7 is the MSB
} cummulative_energy2;


union
{
unsigned long long total;
uint8_t byte[8];  // 0 is the LSB, 7 is the MSB
} tmp_energy;

union
{
unsigned long long total;
uint8_t byte[8];  // 0 is the LSB, 7 is the MSB
} tmp_energy2;


uint32_t total_secs;

uint16_t ticks;
uint16_t ticks_last;
uint8_t cycle_state;
uint8_t cycle_state_last;
uint16_t cycle_cnt;
uint16_t cycle_avg;
uint16_t freq;
uint8_t cycle_started;

int32_t energy_cycle;
int32_t energy_cycle2;
uint32_t energy_total;
uint32_t energy_total2;
uint32_t current_total;
uint32_t current_total2;
uint32_t current_total_last;
uint32_t current_total2_last;
uint32_t energy_total_last;
uint32_t energy_total2_last;
uint32_t voltage_total_last;

double tmp_d;

uint32_t voltage_total;
uint16_t voltage_avg;

uint32_t freq_total;
uint16_t freq_avg;
uint16_t freq_cnt;
uint8_t triggered;

#endif

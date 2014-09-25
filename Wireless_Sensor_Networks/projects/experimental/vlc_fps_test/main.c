/******************************************************************************
 *  Nano-RK, a real-time operating system for sensor networks.
 *  Copyright (C) 2007, Real-Time and Multimedia Lab, Carnegie Mellon University
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
 *******************************************************************************/

#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>
#include <nrk_stats.h>

//Define FPS here:
#define FPS FPS_30

//Do not modify
#define FPS_30 16667
#define FPS_25 20000
#define FPS_15 33333
#define FPS_10 50000

int main() {
	uint8_t led = 0;
	nrk_setup_ports();

	nrk_led_clr(ORANGE_LED);
	nrk_led_clr(BLUE_LED);
	nrk_led_clr(GREEN_LED);
	nrk_led_clr(RED_LED);

	while (1) {
		switch (led) {

		case 0:
			nrk_led_clr(BLUE_LED);
			nrk_led_set(RED_LED);
			led++;
			break;
		case 1:
			nrk_led_clr(RED_LED);
			nrk_led_set(GREEN_LED);
			led++;
			break;
		case 2:
			nrk_led_clr(GREEN_LED);
			nrk_led_set(ORANGE_LED);
			led++;
			break;
		case 3:
			nrk_led_clr(ORANGE_LED);
			nrk_led_set(BLUE_LED);
			led = 0;
			break;
		default:
			led++;
			break;
		}

		// Two spin loops to prevent integer overflow for FPS_15 and FPS_10
		nrk_spin_wait_us(FPS);
		nrk_spin_wait_us(FPS);

	}

	return 0;
}

/*
rf_rx_slip_timer_0:
rf_tx runs as a task, executing every 1second

v1:
1ms timer runs
Independently, task4 runs every4ms tx data on wireless

v2
1. SLIP added
2. 1ms timer fills buffer
3. Every 15ms, slip_tx called
4. slip_tx writes to buffer instead of sending to uart
5. Then this buffer sent to RF tx
Works with actual slip client and server!

rf_tx_ade_0:
1. ADE part added - basic only

rf_tx_ade_1 :
1. Reading watt, var regs inside the timer ISR to check funcition under that added elay
2. correted bug - in memcpy & was missing!

rf_tx_ade_2:
1. Slow 1sec data is sent out independentl of the fast data

rf_tx_ade_3:
1. Fast data sent at 1kHz
Left the system running for long, with the rx talking to SLIP server, works fine!
2. Commented the code
SEARCH FOR //temp IN THE CODE. THOSE SECTIONS ARE TEMPORARY, WHICH NEED TO BE MODIFIED ONCE WE GET THE ADE7878 RUNNING

rf_tx_ade_4:
This code runs with the ADE7878 functioning properly
final code at the end of first semester!

15_rf_tx
This code is for the new FF. Works fine with new meter (tested with Patrick). Had to make the RESET pin as output from FF and make it high. In older meter, RESET was not conencted to FF, so it was okay.

16_rf_tx_v1
Added MF for V, I

18_NewMeter
The working code given to Samsung

19_NewMEter
1. Reading AWatt and checking if even tdetection si happening
Event detection ahppens fine.
DSP RUN register is read. Is this register s read outside, it causes nrk error, probabbly because I2C interrupts one I2C. So reading it inside the interrupt.

The DSP resets when meter FF is connected to PC, but even the FF resets. Dunno why!

20_NewMwter_v2
Changed all task timings. Made is same as in v19_MeasureTaskTimes.
If you want t check the task timings, go to code 19 and check by setting, clearing RED led and probing

21_NewMeter_v3
1. Reading DSP RUN register inside the 1ms timer interrrupt, every 1 sec
Changed the timer interrupt. Every alternate 500ms (1sec period), V,I rms are read. Every alternate 500ms (1sec period) RUN is read.
If RUN is 0, counter is increased
2. DSP reset counter is sent over wireless

24_NewMeter_v6
Created from 21_NewMeter_v3 directly
1. Commented out task 5 since it was doing nothing

24s_NewMeter_v7
Created from 24_..v6 - was created to get the code working on the micro FF, which seemed to have some timing issue with respect to getting the meter to boot in the correct power mode.
1. Change made in the way we are putting the ADE in PSM0 mode.
RST is made high, low, then power mode is set, EINT enabled, then bring the chip out of reset. then PSM0 is set again.

This code works fine with normal and micro FF. But its been made oversafe (high delays)

25_NewMeter_v8
For putting ADE in correct mode, removed the loop delay since printf was giving enough delay

25a_NewMeter_v8
Comments added

28_Meter2phase_v0
Created from 25a

The slow and fast data are modified now to:
1. Fast data - instead of 2 registers, I now read 4 registers (A,B phase V,I)
2. Slow data - Reading some 10 parameters

The 1ms timer is modified so that:
Every 1ms, the 4 fast parameters are read. Every 1ms for the first 10ms, a single slow param is read

Dummy values assigned to the 4 fast parameters, just to check if they fine.
Values incremented every 1ms

29_Meter2phase_v1
1. Modified slip.c so that the slip packet transmits the packet type (slow or fast data) and the meter address.

30_Meter2phase_v2
Fast, slip data sent - final
No MFs applied to any data within code since we are applying it in the SLIP client side.

31_Meter2phase_v3
Made the whole ADE7878 init part into a function
.................................
3_phase_meter_2g
1. Fast data at 1kHz. Two phases read.
2. Slow data every 1 second
3. Wireless speed increased from 250kbs to 500kbps
4. timings of tasks changed
5. semaphore added so that slow and fast data can transmit without clashing

4July
3_phase_meter_6
1. Slow data task disabled
2. Slow data sent in same func as fast.
3. Changed UART comm from gateway to Slip Ser to 230K4 bd
4. slip client converts to V, I and writes
*/


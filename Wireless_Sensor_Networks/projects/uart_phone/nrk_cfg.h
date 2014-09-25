/***************************************************************
*                            NanoRK CONFIG                     *
***************************************************************/

#ifndef __nrk_cfg_h	
#define __nrk_cfg_h

// NRK_REPORT_ERRORS will cause the kernel to print out information about
// missed deadlines or reserve violations
#define NRK_REPORT_ERRORS
// NRK_HALT_ON_ERRORS will cause the kernel to freeze on errors so that
// it is easier to see debugging messages.
#define NRK_HALT_ON_ERROR
#define IGNORE_BROWN_OUT_ERROR
#define IGNORE_EXT_RST_ERROR

#define NRK_NO_BOUNDED_CONTEXT_SWAP

#define NRK_WATCHDOG
#define NRK_REBOOT_ON_ERROR
// NRK_STACK_CHECK adds a little check to see if the bottom of the stack
// has been over written on all suspend calls
#define NRK_STACK_CHECK

#define RADIO_PRIORITY_CEILING		10
//#define KERNEL_STK_ARRAY
// Leave NRK_NO_POWER_DOWN define in if the target can not wake up from sleep 
// because it has no asynchronously clocked
#define NRK_NO_POWER_DOWN

// Max number of tasks in your application
// Be sure to include the idle task
// Making this the correct size will save on BSS memory which
// is both RAM and ROM...
#define NRK_MAX_TASKS       		7    
#define	NRK_N_RES			1	
#define NRK_MAX_DRIVER_CNT		1
                           
// Enable buffered and signal controlled serial RX
#define NRK_UART_BUF   1
#define MAX_RX_UART_BUF 128

#define NRK_TASK_IDLE_STK_SIZE         128 // Idle task stack size min=32 
#define NRK_APP_STACKSIZE              256 
#define NRK_KERNEL_STACKSIZE           256 
#define BMAC_STACKSIZE                 256 

#define NRK_MAX_RESOURCE_CNT	5



#endif
#include <string.h>
#include "FreeRTOSConfig.h"
#include "lpc17xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "fio.h"
#include "pinsel.h"
#include "core_cm3.h"
#include "uart.h"
#include "semphr.h"
#include "irq.h"

/* extern public functions */
extern void uart_app_init (void);
extern void uart_app_transmit (char * p_data, uint32_t data_len);
extern void uart_app_receive (uint8_t * p_data, uint32_t data_len);
extern void xPortPendSVHandler(void) __irq;
extern void xPortSysTickHandler(void) __irq;
extern void vPortSVCHandler(void) __irq;

/* Private variables */
static _S_UART_INFO * g_p_uart_handle = NULL;
static uint32_t g_blink_ms = 1000;

void vBlinkTask( void *pvParameter )
{	
//	const char * c_p_string = "Hello World\n";
	uart_app_init();
	
	while(1)
	{
		fio_toggle_bit(PORT4, 28);
//		uart_app_transmit((char *)c_p_string, strlen(c_p_string));
		
		vTaskDelay(g_blink_ms);	
	}
}

void irq_initialization(void)
{
	irq_init(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
	irq_setup_svc_handler(vPortSVCHandler);
	irq_setup_systick_handler(xPortSysTickHandler);
	irq_setup_pendsv_handler(xPortPendSVHandler);
}

void main_set_blink_rate (uint32_t ms)
{
	g_blink_ms = ms;
}

int main( void )
{
	SystemCoreClockUpdate();
	
	pinsel_init();
	fio_init();
	irq_initialization();
	
	xTaskCreate(vBlinkTask, (const signed char *)"Blink", 240, NULL, 2, NULL);
	vTaskStartScheduler();
	
	return 0;
}

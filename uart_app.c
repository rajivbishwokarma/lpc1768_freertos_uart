#include <string.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "uart0.h"
#include "pinsel-name.h"


/* extern public functions */
extern void main_set_blink_rate (uint32_t ms);

/* public variables */
const _S_UART_BAUDRATES uart_baudrates[] =
{
		{10, 229, 115200}
};

/* private variables */
static _S_UART_INFO * g_p_uart_handle = NULL;
static xSemaphoreHandle g_sem_handle = NULL;
static char g_rx_buffer[20];
static int32_t g_rx_buffer_index = 0;
static int32_t g_num_let_base = 0;
static int32_t g_interrupt_counter = 0;
static int32_t g_blinkrate = 0;

/* Private function declaration */
static void _uart_task (void * p_pv);
static void _configure_uart (void);
static void _uart_isr(_S_UART_INFO * const u, uint8_t received_byte, void * upcall_arg);



/*************************************** Public function definitions *******************************/
void uart_app_init (void)
{
	g_sem_handle = xSemaphoreCreateBinary();
	_configure_uart();
	
	xTaskCreate(_uart_task, (const signed char *)"uartTask", 200, NULL, 3, NULL);
}

void uart_app_transmit (uint8_t * p_data, uint32_t data_len)
{
	if (g_p_uart_handle)
	{
		uart_tx(g_p_uart_handle, p_data, data_len);
	}
}

void uart_app_receive (uint8_t * p_data, uint32_t data_len)
{
	if (g_p_uart_handle)
	{
		uart_rx_data(g_p_uart_handle, p_data, data_len);
	}
}


/*************************************** Private function definitions *******************************/

static void _uart_task (void * p_pv)
{
	char buffer[20] = {0};
	const char * p_string_frmt = "blink_rate=";
	
	do
	{
		xSemaphoreTake(g_sem_handle, portMAX_DELAY);
		// do some receive buffer processing
		
		
		uint32_t data_len = uart_is_received(g_p_uart_handle);
		
		if (data_len)
		{
			uart_rx_data(g_p_uart_handle, buffer, data_len);
			uart_app_transmit((uint8_t *)buffer, data_len);
			
			char * p_cur = strstr(buffer, p_string_frmt);
			
			if (p_cur > NULL)
			{
				p_cur += strlen(p_string_frmt);
				uint32_t rate = strtoul(p_cur, NULL, 10);
				main_set_blink_rate(rate);				
			}
		}
		
	} while (1);
}

static void _configure_uart (void)
{
	static char rx_buffer[128];
	static _S_UART_LCR s_uart_lcr = 
  {
		.bits = 8,
		.stop_bit = 1,
		.parity_enable = 0,
	};
	
	const _S_UART_PINS s_uart_pins = 
	{
		.tx = TXD0,
		.rx = RXD0
	};
	
	uart0_init();
	g_p_uart_handle = uart0_lock(rx_buffer, sizeof(rx_buffer), portMAX_DELAY, NULL, _uart_isr);
	
	if (g_p_uart_handle)
	{
		g_p_uart_handle->init(g_p_uart_handle, uart_baudrates[0].baudrate, s_uart_lcr, s_uart_pins, 0, 1);
	}
}

//static void _uart_ch_assemble(uint8_t received_byte)
//{
//	g_interrupt_counter++;	// Count the number of times isr is called (counting the characters entered)
//	int8_t mutex = 0;
//	
//	if( (char) received_byte == '=' )
//	{
//		g_num_let_base = g_interrupt_counter;	// base position from where the number will start
//		mutex = 1;
//	}
//	else 
//	{
//		mutex = 0;
//	}
//	
//	if (mutex)
//	{
//		if( g_interrupt_counter == (g_num_let_base + 1) )
//			{
//				g_rx_buffer[g_rx_buffer_index + 0] = received_byte;
//				uart_app_transmit((uint8_t *) g_rx_buffer, 1);
//			}
//		
//			if( g_interrupt_counter == (g_num_let_base + 2) )
//			{
//				g_rx_buffer[g_rx_buffer_index + 1] = received_byte;
//				uart_app_transmit((uint8_t *) &g_rx_buffer[g_num_let_base + 2], 1);
//			}
//		
//			if( g_interrupt_counter == (g_num_let_base + 3))
//			{
//				g_rx_buffer[g_rx_buffer_index + 2] = received_byte;
//				uart_app_transmit((uint8_t *) &g_rx_buffer[g_num_let_base + 3], 1);
//			}
//			if( g_interrupt_counter == (g_num_let_base + 4))
//			{
//				g_rx_buffer[g_rx_buffer_index + 3] = received_byte;
//				uart_app_transmit((uint8_t *) &g_rx_buffer[g_num_let_base + 4], 1);
//				
//				g_blinkrate = (int8_t) g_rx_buffer[0] * 1000 + (int8_t) g_rx_buffer[1] * 100 + (int8_t) g_rx_buffer[2] * 10 + (int8_t) g_rx_buffer[3];
//				
//				mutex = 0;
//			}
//		}
//}

static void _uart_isr(_S_UART_INFO * const u, uint8_t received_byte, void * upcall_arg)
{
	if ('\0' == received_byte)
	{
		xSemaphoreGiveFromISR(g_sem_handle, NULL);
	}
}
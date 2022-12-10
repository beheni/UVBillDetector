
/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Variable for storing character read from terminal */
uint8_t uart_read_value;


int main(void)
{
    cy_rslt_t result;

#if defined (CY_DEVICE_SECURE)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
#endif

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                 CY_RETARGET_IO_BAUDRATE);

    /* retarget-io init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }



    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("This is finally working!!!!! \r\n\n");
    printf("YAAAAY \r\n\n");

    cyhal_gpio_init(CYBSP_A12, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true); //led
//    cyhal_gpio_init(CYBSP_A10, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true ); // esp
    cyhal_gpio_init(CYBSP_A13, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false ); //ir

    for (;;)

    {
		printf("reading from ir \r\n\n");
		bool res = cyhal_gpio_read(CYBSP_A13);
		printf("%d\r\n\n", res);
		if (cyhal_gpio_read(CYBSP_A13) == 0)
		{
			cyhal_gpio_write(CYBSP_A12, false);
			}
		else {
			cyhal_gpio_write(CYBSP_A12, true);
		}
    }
}

/* [] END OF FILE */

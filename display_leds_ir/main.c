#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "GUI.h"
#include "mtb_st7789v.h"
#include "cy8ckit_028_tft_pins.h" /* This is part of the CY8CKIT-028-TFT shield library. */

#define GPIO_INTERRUPT_PRIORITY (7u)

/* The pins above are defined by the CY8CKIT-028-TFT library. If the display is being used on different hardware the mappings will be different. */
void paint_display(char*, GUI_COLOR);
void inform_esp();
const mtb_st7789v_pins_t tft_pins =
{
    .db08 = CY8CKIT_028_TFT_PIN_DISPLAY_DB8,
    .db09 = CY8CKIT_028_TFT_PIN_DISPLAY_DB9,
    .db10 = CY8CKIT_028_TFT_PIN_DISPLAY_DB10,
    .db11 = CY8CKIT_028_TFT_PIN_DISPLAY_DB11,
    .db12 = CY8CKIT_028_TFT_PIN_DISPLAY_DB12,
    .db13 = CY8CKIT_028_TFT_PIN_DISPLAY_DB13,
    .db14 = CY8CKIT_028_TFT_PIN_DISPLAY_DB14,
    .db15 = CY8CKIT_028_TFT_PIN_DISPLAY_DB15,
    .nrd  = CY8CKIT_028_TFT_PIN_DISPLAY_NRD,
    .nwr  = CY8CKIT_028_TFT_PIN_DISPLAY_NWR,
    .dc   = CY8CKIT_028_TFT_PIN_DISPLAY_DC,
    .rst  = CY8CKIT_028_TFT_PIN_DISPLAY_RST
};

void inform_esp() {
	cyhal_gpio_init(TX, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);
	cyhal_gpio_write(TX, true);
	cyhal_system_delay_ms(500);
	cyhal_gpio_write(TX, false);
}

void paint_display(char* message, GUI_COLOR color) {
	cy_rslt_t result;

	    /* Initialize the device and board peripherals */
	    result = cybsp_init();
	    CY_ASSERT(result == CY_RSLT_SUCCESS);

	    __enable_irq();

	    /* Initialize the display controller */
	    result = mtb_st7789v_init8(&tft_pins);
	    CY_ASSERT(result == CY_RSLT_SUCCESS);

	    GUI_Init();
	    GUI_SetFont(&GUI_Font32_1);
	    GUI_SetColor(color);
	    GUI_SetBkColor(color);
	    for(int i = 0; i < 8; i++) {
	    	if (i == 3) {
	    		GUI_DispString("_______");
	    		GUI_SetColor(GUI_BLACK);
	    		GUI_DispString(message);
	    		GUI_SetColor(color);
	    		GUI_DispString("______");

	    	}
	    	GUI_DispString("____________________________________\n");
	    }
}


uint8_t uart_read_value;

void handle_error(uint32_t status)
{
    if (status != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
}
volatile bool ir_intr_flag = false;
cyhal_gpio_callback_data_t gpio_ir_callback_data;
static void gpio_ir_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event);


int main(void)
{
    cy_rslt_t result;
//
#if defined (CY_DEVICE_SECURE)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
#endif

    /* Initialize the device and board peripherals */
//    result = cybsp_init();

    /* Board init failed. Stop program execution */
//    handle_error(result);
//
//    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(TX, RX,
                                 CY_RETARGET_IO_BAUDRATE);

    /* retarget-io init failed. Stop program execution */
    handle_error(result);

//
    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");
////
    printf("This is finally working!!!!! \r\n\n");
    printf("YAAAAY \r\n\n");
//
    cyhal_gpio_init(UV_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false); //led

//
    handle_error(result);

//    cyhal_gpio_init(TX, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false); //arduino

    handle_error(result);

//    cyhal_gpio_write(UV_LED, false);
//    cyhal_gpio_toggle(UV_LED);
//
//    cyhal_gpio_init(CYBSP_A10, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true ); // esp
//
    cyhal_gpio_init(IR, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false); //ir
////
    handle_error(result);
//
    paint_display("HELLO", GUI_ORANGE);
    cyhal_gpio_write(UV_LED, false);
    gpio_ir_callback_data.callback = gpio_ir_interrupt_handler;

    cyhal_gpio_register_callback(IR,
                                     &gpio_ir_callback_data);

	cyhal_gpio_enable_event(IR, CYHAL_GPIO_IRQ_BOTH,
                                     GPIO_INTERRUPT_PRIORITY, true);




//	int counter = 0;
    for (;;)
    {
//    	++counter;
//    	if (counter == 1) {
//    		cyhal_gpio_toggle(UV_LED);
//    	}
    	printf("reading from ir \r\n\n");
//		bool res = cyhal_gpio_read(CYBSP_A13);
		printf("%d\r\n\n", ir_intr_flag);
    	if (true == ir_intr_flag){
    		cyhal_gpio_write(UV_LED, true);
    	}
    	else {
			cyhal_gpio_write(UV_LED, false);
			inform_esp();
    	}
    }
}

static void gpio_ir_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{
	if (cyhal_gpio_read(IR) == false) {
		ir_intr_flag = false;
	}
	else {
		ir_intr_flag = true;
	}
}
/* [] END OF FILE */

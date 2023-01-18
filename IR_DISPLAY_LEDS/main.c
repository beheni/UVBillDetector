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
	cyhal_gpio_init(espRX, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);
	cyhal_gpio_write(espRX, true);
	cyhal_system_delay_ms(500);
	cyhal_gpio_write(espRX, false);
}


void paint_display(char* message, GUI_COLOR color) {
		GUI_Clear();
	    GUI_SetColor(color);
	    GUI_SetBkColor(color);
	    GUI_SetColor(GUI_BLACK);
	    GUI_DispString(message);
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

    #if defined (CY_DEVICE_SECURE)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
#endif

    __enable_irq();

    cyhal_gpio_init(UV_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false); //led

    handle_error(result);

    cyhal_gpio_init(IR, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false ); //ir

    handle_error(result);

    cyhal_gpio_init(espRX, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false ); //ir

    handle_error(result);

    cyhal_gpio_init(espRX2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, false );

    handle_error(result);

    gpio_ir_callback_data.callback = gpio_ir_interrupt_handler;

    cyhal_gpio_register_callback(IR,
                                     &gpio_ir_callback_data);
	cyhal_gpio_enable_event(IR, CYHAL_GPIO_IRQ_BOTH,
                                     GPIO_INTERRUPT_PRIORITY, false);

	/* Initialize the display controller */
	result = mtb_st7789v_init8(&tft_pins);
	CY_ASSERT(result == CY_RSLT_SUCCESS);

	GUI_Init();
	GUI_SetFont(&GUI_Font32_1);

    for (;;)
    {
//    	if (true==ir_intr_flag){
//			cyhal_gpio_write(UV_LED, true);
//		}
//		else {
//			cyhal_gpio_write(UV_LED, false);
//			inform_esp();
//		}
        bool res = cyhal_gpio_read(espRX);
    	bool res2 = cyhal_gpio_read(espRX2);
    	if (res) {
    		paint_display("", GUI_GREEN);
    	}
    	else if (res2) {
    		paint_display("", GUI_RED);
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

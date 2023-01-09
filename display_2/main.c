#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "GUI.h"
#include "mtb_st7789v.h"
#include "cy8ckit_028_tft_pins.h" /* This is part of the CY8CKIT-028-TFT shield library. */

/* The pins above are defined by the CY8CKIT-028-TFT library. If the display is being used on different hardware the mappings will be different. */
void paint_display(char*, GUI_COLOR);
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

int main(void)
{
    paint_display("HELLO", GUI_ORANGE);
    for(;;)
    {
    }
}

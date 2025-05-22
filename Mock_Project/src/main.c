#include "device_registers.h"
#include "S32K144.h"
#include "clocks_and_modes.h"
#include "ADC.h"
#include "LPUART.h"
#include <stdio.h>

#define SW2 12 // PTC12, SW2
#define SW3 13 // PTC13, SW3
#define DEBOUNCE_TIME 50 // ms
#define DOUBLE_CLICK_TIME 500 // ms
#define ADC_READ_INTERVAL 100 // ms

uint32_t last_sent_value = -1;
uint32_t last_adc_time = 0;
volatile uint32_t msTicks = 0;

// Biến theo dõi nút bấm
volatile uint32_t btn0_press_time = 0;
volatile uint32_t btn1_press_time = 0;
volatile bool btn0_pressed = false;
volatile bool btn1_pressed = false;
uint32_t btn0_last_recorded_time = 0;
uint32_t btn1_last_recorded_time = 0;
uint32_t last_btn0_time = 0;
uint32_t last_btn1_time = 0;

void WDOG_disable(void) {
    WDOG->CNT = 0xD928C520; /* Unlock watchdog */
    WDOG->TOVAL = 0xFFFF; /* Maximum timeout value */
    WDOG->CS = 0x2100; /* Disable watchdog */
}

void NVIC_Init(void) {
    S32_NVIC->ISER[PORTC_IRQn >> 5] = (1 << (PORTC_IRQn & 0x1F));
}

void LPIT0_Ch0_IRQHandler(void) {
    LPIT0->MSR |= LPIT_MSR_TIF0_MASK; /* Clear timer flag */
    msTicks++;
}

void LPIT0_init(void) {
    PCC->PCCn[PCC_LPIT_INDEX] = PCC_PCCn_PCS(6); /* Clock Src = SPLL2_DIV2_CLK */
    PCC->PCCn[PCC_LPIT_INDEX] |= PCC_PCCn_CGC_MASK; /* Enable clock to LPIT0 */
    LPIT0->MCR = LPIT_MCR_M_CEN_MASK; /* Enable module clock */
    LPIT0->TMR[0].TVAL = 40000; /* 1ms timeout at 40MHz */
    LPIT0->TMR[0].TCTRL = LPIT_TMR_TCTRL_T_EN_MASK; /* Enable timer */
    LPIT0->MIER |= LPIT_MIER_TIE0_MASK; /* Enable interrupt */
    S32_NVIC->ISER[LPIT0_Ch0_IRQn / 32] = 1 << (LPIT0_Ch0_IRQn % 32);
}

void PORT_init(void) {
    PCC->PCCn[PCC_PORTC_INDEX] |= PCC_PCCn_CGC_MASK; /* Enable clock for PORTC */
    PORTC->PCR[SW2] = PORT_PCR_MUX(1)
    				| PORT_PCR_IRQC(9)
					| PORT_PCR_PE_MASK
					| PORT_PCR_PS_MASK; /* SW2 */
    PORTC->PCR[SW3] = PORT_PCR_MUX(1)
    				| PORT_PCR_IRQC(9)
					| PORT_PCR_PE_MASK
					| PORT_PCR_PS_MASK; /* SW3 */

    PORTC->PCR[6] |= PORT_PCR_MUX(2); /* UART1 RX */
    PORTC->PCR[7] |= PORT_PCR_MUX(2); /* UART1 TX */
    PTC->PDDR &= ~((1 << SW2) | (1 << SW3)); /* Set as inputs */
}

void PORTC_IRQHandler(void) {
    if (PORTC->ISFR & (1 << SW2)) {
        PORTC->ISFR |= (1 << SW2);
        if (msTicks - btn0_last_recorded_time > DEBOUNCE_TIME) {
            btn0_last_recorded_time = msTicks;
            btn0_press_time = msTicks;
            btn0_pressed = true;
        }
    }
    if (PORTC->ISFR & (1 << SW3)) {
        PORTC->ISFR |= (1 << SW3);
        if (msTicks - btn1_last_recorded_time > DEBOUNCE_TIME) {
            btn1_last_recorded_time = msTicks;
            btn1_press_time = msTicks;
            btn1_pressed = true;
        }
    }
}

int main(void) {
    WDOG_disable();
    SOSC_init_8MHz();
    SPLL_init_160MHz();
    NormalRUNmode_80MHz();
    PORT_init();
    ADC_init();
    LPUART1_init();
    LPIT0_init();
    NVIC_Init();

    while (1) {
        uint32_t current_time = msTicks;

        /* ADC */
        if (current_time - last_adc_time >= ADC_READ_INTERVAL) {
            convertAdcChan(12);
            while (adc_complete() == 0) {}
            uint32_t adc_mv = read_adc_chx();
            uint32_t scaled = adc_mv / 50; /* 0-100 */
            if (scaled != last_sent_value) {
                char buf[10];
                sprintf(buf, "%lu\n\r", scaled);
                LPUART1_transmit_string(buf);
                last_sent_value = scaled;
            }
            last_adc_time = current_time;
        }

        /* Button 0 handling */
        if (btn0_pressed) {
            btn0_pressed = false;
            uint32_t press_time = btn0_press_time;
            if (press_time - last_btn0_time < DOUBLE_CLICK_TIME) {
                LPUART1_transmit_string("previous\n\r");
                last_btn0_time = 0;
            } else {
                last_btn0_time = press_time;
            }
        }
        if (last_btn0_time != 0 && current_time - last_btn0_time > DOUBLE_CLICK_TIME) {
            LPUART1_transmit_string("stop\n\r");
            last_btn0_time = 0;
        }

        /* Button 1 handling */
        if (btn1_pressed) {
            btn1_pressed = false;
            uint32_t press_time = btn1_press_time;
            if (press_time - last_btn1_time < DOUBLE_CLICK_TIME) {
                LPUART1_transmit_string("next\n\r");
                last_btn1_time = 0;
            } else {
                last_btn1_time = press_time;
            }
        }
        if (last_btn1_time != 0 && current_time - last_btn1_time > DOUBLE_CLICK_TIME) {
            LPUART1_transmit_string("play_pause\n\r");
            last_btn1_time = 0;
        }
    }

    return 0;
}

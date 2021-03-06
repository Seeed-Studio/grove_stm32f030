/* Generated by startup_generator */

#include <stm32f0xx.h>
#include <core_cm0.h>

extern void _estack(void);  // to force type checking
void Reset_Handler(void);
void default_handler (void) 
{
    while(1);
}

//Arduino: we must setup hardware before doing this
//void __attribute__ ((weak)) __libc_init_array (void){}

//Linker supplied pointers

extern unsigned long _sidata;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;

extern int main(void);

void Reset_Handler(void) {

    unsigned long *src, *dst;

    src = &_sidata;
    dst = &_sdata;

    // Copy data initializers

    while (dst < &_edata) {
        *(dst++) = *(src++);
	}

    // Zero bss

    dst = &_sbss;
    while (dst < &_ebss) {
        *(dst++) = 0;
    }
    main();
    while(1) {}
}




__attribute__ ((section(".isr_vector")))

const DeviceVectors exception_table = {
    _estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    0,
    0,
    0,
    0, 
	0, 
	0, 
	0,
    SVC_Handler,
    0,
    0,
    PendSV_Handler,
    SysTick_Handler,

    WWDG_IRQHandler,
    PVD_IRQHandler,
    RTC_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_1_IRQHandler,
    EXTI2_3_IRQHandler,
    EXTI4_15_IRQHandler,
    TS_IRQHandler,
    DMA1_Channel1_IRQHandler,
    DMA1_Channel2_3_IRQHandler,
    DMA1_Channel4_5_IRQHandler,
    ADC1_COMP_IRQHandler,
    TIM1_BRK_UP_TRG_COM_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM6_DAC_IRQHandler,
	0,
    TIM14_IRQHandler,
    TIM15_IRQHandler,
    TIM16_IRQHandler,
    TIM17_IRQHandler,
    I2C1_IRQHandler,
    I2C2_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
	0,
    CEC_IRQHandler,
	0,
};

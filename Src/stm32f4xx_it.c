#include "main.h"
#include "stm32f4xx_it.h"

void NMI_Handler(void)
{
  GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
  while (1)
  {
  }
}

void HardFault_Handler(void)
{
  GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
  while (1) {
  }
}

void BusFault_Handler(void)
{
  GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
  while (1) {
  }
}

void UsageFault_Handler(void)
{
  GPIOE->BSRR = (LL_GPIO_PIN_15 << 16);
  while (1) {
  }
}


void DebugMon_Handler(void)
{

}


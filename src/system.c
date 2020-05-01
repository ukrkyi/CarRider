/* (c) 2020 ukrkyi */
#include <stm32f4xx_hal.h>

#include <stm32f4xx_ll_rcc.h>

#include "system.h"

void SystemClock_Config(void)
{
	/* Can't use complex HAL functions here because they depend on tick interrupt working
	 * which is not the case since this function is called prior to RTOS Scheduler started
	 */

	/* Configure the main internal regulator output voltage */
	__HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/*------------------------------- HSE Configuration ------------------------*/
	assert_param(LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_STATUS_HSI);
	/* Turn on HSE oscillator --------------------------------------------------*/
	LL_RCC_HSE_Enable();

	/* Wait till HSE is ready */
	while(!LL_RCC_HSE_IsReady());

	/*-------------------------------- PLL Configuration -----------------------*/
	/* Disable the main PLL. */
	LL_RCC_PLL_Disable();

	/* Wait till PLL is ready */
	while(LL_RCC_PLL_IsReady());

	/* Configure the main PLL clock source, multiplication and division factors. */
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE,
				    LL_RCC_PLLM_DIV_25,
				    336,
				    LL_RCC_PLLP_DIV_4);
	/* Enable the main PLL. */
	LL_RCC_PLL_Enable();

	/* Wait till PLL is ready */
	while(!LL_RCC_PLL_IsReady());

	/* Increasing the number of wait states because of higher CPU frequency */
	if(__HAL_FLASH_GET_LATENCY() < FLASH_LATENCY_2)
	{
	  /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
	  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_2);

	  /* Check that the new number of wait states is taken into account to access the Flash
	  memory by reading the FLASH_ACR register */
	  assert_param(__HAL_FLASH_GET_LATENCY() == FLASH_LATENCY_2);
	}

	/*-------------------------- PCLK1 Configuration ---------------------------*/
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
	/*-------------------------- PCLK2 Configuration ---------------------------*/
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

	/*------------------------- SYSCLK Configuration ---------------------------*/
	assert_param(LL_RCC_PLL_IsReady());
	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

	SystemCoreClockUpdate();
	/** Enables the Clock Security System
	*/
	HAL_RCC_EnableCSS();
}

void SystemConfig() {
	__HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
	__HAL_FLASH_DATA_CACHE_ENABLE();
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();

	NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	__HAL_RCC_SYSCFG_CLK_ENABLE();

	SystemClock_Config();
}

void assert_failed(uint8_t *file, uint32_t line) {
	UNUSED(file);
	UNUSED(line);
	for( ;; );
}

//uint32_t HAL_GetTick(){
//	return xTaskGetTickCount();
//}

//// Remove HAL Tick Initialization bc it's handled by RTOS
//HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
//	UNUSED(TickPriority);
//	return HAL_OK;
//}
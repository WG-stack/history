#include "ultrasonic.h"

static void Ultrasonic_DWT_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static uint32_t Ultrasonic_Micros(void)
{
  return DWT->CYCCNT / (HAL_RCC_GetHCLKFreq() / 1000000U);
}

static uint32_t Ultrasonic_MeasureEchoUs(void)
{
  uint32_t t0 = 0;
  uint32_t start = 0;

  HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_RESET);
  HAL_Delay(2);
  HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_SET);
  for (volatile uint32_t i = 0; i < 120; i++)
  {
    __NOP();
  }
  HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_RESET);

  t0 = Ultrasonic_Micros();
  while (HAL_GPIO_ReadPin(Echo_GPIO_Port, Echo_Pin) == GPIO_PIN_RESET)
  {
    if ((uint32_t)(Ultrasonic_Micros() - t0) > 30000U)
    {
      return 0;
    }
  }

  start = Ultrasonic_Micros();
  while (HAL_GPIO_ReadPin(Echo_GPIO_Port, Echo_Pin) == GPIO_PIN_SET)
  {
    if ((uint32_t)(Ultrasonic_Micros() - start) > 30000U)
    {
      return 0;
    }
  }

  return (uint32_t)(Ultrasonic_Micros() - start);
}

void Ultrasonic_Init(void)
{
  Ultrasonic_DWT_Init();
  HAL_GPIO_WritePin(Trig_GPIO_Port, Trig_Pin, GPIO_PIN_RESET);
}

uint32_t Ultrasonic_ReadDistanceCm(void)
{
  uint32_t echo_us = Ultrasonic_MeasureEchoUs();

  if (echo_us == 0U)
  {
    return 0;
  }

  return echo_us / 58U;
}

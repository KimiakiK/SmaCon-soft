# SmaCon-soft

## STM32U575のADCを使う場合の注意点
main.cに下記のコードを書く
[参考](https://community.st.com/t5/stm32-mcu-products/i-m-using-the-stm32u575ciu-microcontroller-and-implementing-the/td-p/76079)
```
  /* USER CODE BEGIN SysInit */

  // get access to PWR
  SET_BIT(RCC->AHB3ENR, RCC_AHB3ENR_PWREN);
  // switch voltage monitor on
  SET_BIT(PWR->SVMCR, PWR_SVMCR_AVM1EN);
  // wait until ready
  while (READ_BIT(PWR->SVMSR, PWR_SVMSR_VDDA1RDY) == 0);
  // switch off VDDA isolation
  SET_BIT(PWR->SVMCR, PWR_SVMCR_ASV); 
  // we do not need access to the PWR domain
  CLEAR_BIT(RCC->AHB3ENR, RCC_AHB3ENR_PWREN);
  
  /* USER CODE END SysInit */
```

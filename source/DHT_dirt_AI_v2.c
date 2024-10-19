#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_adc.h"
#include "fsl_adc_etc.h"
#include "fsl_common.h"
#include "random_forest_model.h"
#include "fsl_device_registers.h"
#include "system_MIMXRT1062.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_ADC_BASE          ADC1
#define DEMO_ADC_USER_CHANNEL  15U
#define DEMO_ADC_CHANNEL_GROUP 0U

#define DEMO_ADC_ETC_BASE             ADC_ETC
#define DEMO_ADC_ETC_CHAIN_LENGTH     0U
#define DEMO_ADC_ETC_CHANNEL          15U
#define EXAMPLE_ADC_ETC_DONE0_Handler ADC_ETC_IRQ0_IRQHandler

#define DHT11_GPIO GPIO1
#define DHT11_GPIO_PIN 11

volatile bool g_pinSet = false;


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void ADC_Configuration(void);
/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile bool g_AdcConversionDoneFlag;
volatile uint32_t g_AdcConversionValue;
const uint32_t g_Adc_12bitFullRange = 4096U;

/*******************************************************************************
 * Code
 ******************************************************************************/
void EXAMPLE_ADC_ETC_DONE0_Handler(void)
{
    ADC_ETC_ClearInterruptStatusFlags(DEMO_ADC_ETC_BASE, kADC_ETC_Trg0TriggerSource, kADC_ETC_Done0StatusFlagMask);
    g_AdcConversionDoneFlag = true;
    g_AdcConversionValue = ADC_ETC_GetADCConversionValue(DEMO_ADC_ETC_BASE, 0U, 0U); /* Get trigger0 chain0 result. */
    SDK_ISR_EXIT_BARRIER;
}

static void delayms(int ms)
{
    volatile uint32_t i = 0;
    for (i = 0; i < ms*70000; ++i)
    {
    	__NOP(); /* delay */
    }

}

static void delayus(int us)
{
    volatile uint32_t i = 0;
    for (i = 0; i < us*70; ++i)
    {
    	__NOP(); /* delay */
    }

}

uint8_t read_data_bit(void)
{
    uint8_t bit = 0;
    while(GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 0);  // 等待信號線變高
    delayus(35);  // 等待30µs
    if(GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 1)  // 如果信號線仍為高,則為1
    {
        bit = 1;
    }
    while(GPIO_PinRead(DHT11_GPIO, DHT11_GPIO_PIN) == 1);  // 等待信號線變低
    return bit;
}
/*!
 * @brief Main function.
 */
int main(void)
{
    adc_etc_config_t adcEtcConfig;
    adc_etc_trigger_config_t adcEtcTriggerConfig;
    adc_etc_trigger_chain_config_t adcEtcTriggerChainConfig;

    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    gpio_pin_config_t USER_DHT_out_config = {
              .direction = kGPIO_DigitalOutput,
              .outputLogic = 1U,
              .interruptMode = kGPIO_NoIntmode
          };

        gpio_pin_config_t USER_DHT_in_config = {
                  .direction = kGPIO_DigitalInput,
                  .outputLogic = 0U,
                  .interruptMode = kGPIO_NoIntmode
              };


    ADC_Configuration();
    /* Initialize the ADC_ETC. */
    ADC_ETC_GetDefaultConfig(&adcEtcConfig);
    adcEtcConfig.XBARtriggerMask = 1U; /* Enable the external XBAR trigger0. */
    ADC_ETC_Init(DEMO_ADC_ETC_BASE, &adcEtcConfig);

    /* Set the external XBAR trigger0 configuration. */
    adcEtcTriggerConfig.enableSyncMode      = false;
    adcEtcTriggerConfig.enableSWTriggerMode = true;
    adcEtcTriggerConfig.triggerChainLength  = DEMO_ADC_ETC_CHAIN_LENGTH; /* Chain length is 1. */
    adcEtcTriggerConfig.triggerPriority     = 0U;
    adcEtcTriggerConfig.sampleIntervalDelay = 0U;
    adcEtcTriggerConfig.initialDelay        = 0U;
    ADC_ETC_SetTriggerConfig(DEMO_ADC_ETC_BASE, 0U, &adcEtcTriggerConfig);

    /* Set the external XBAR trigger0 chain0 configuration. */
    adcEtcTriggerChainConfig.enableB2BMode       = false;
    adcEtcTriggerChainConfig.ADCHCRegisterSelect = 1U
                                                   << DEMO_ADC_CHANNEL_GROUP; /* Select ADC_HC0 register to trigger. */
    adcEtcTriggerChainConfig.ADCChannelSelect =
        DEMO_ADC_ETC_CHANNEL; /* ADC_HC0 will be triggered to sample Corresponding channel. */
    adcEtcTriggerChainConfig.InterruptEnable = kADC_ETC_Done0InterruptEnable; /* Enable the Done0 interrupt. */
#if defined(FSL_FEATURE_ADC_ETC_HAS_TRIGm_CHAIN_a_b_IEn_EN) && FSL_FEATURE_ADC_ETC_HAS_TRIGm_CHAIN_a_b_IEn_EN
    adcEtcTriggerChainConfig.enableIrq = true; /* Enable the IRQ. */
#endif                                         /* FSL_FEATURE_ADC_ETC_HAS_TRIGm_CHAIN_a_b_IEn_EN */
    ADC_ETC_SetTriggerChainConfig(DEMO_ADC_ETC_BASE, 0U, 0U,
                                  &adcEtcTriggerChainConfig); /* Configure the trigger0 chain0. */

    /* Enable the NVIC. */
    EnableIRQ(ADC_ETC_IRQ0_IRQn);

    GPIO_PinInit(DHT11_GPIO, DHT11_GPIO_PIN, &USER_DHT_out_config);

    while (1)
    {
    	delayms(10000);

    	GPIO_PinInit(DHT11_GPIO, DHT11_GPIO_PIN, &USER_DHT_out_config);

		delayms(2000);

		GPIO_PinWrite(DHT11_GPIO,DHT11_GPIO_PIN,0);
		delayms(18);
		//SysTick_DelayTicks(18015U);
		GPIO_PinWrite(DHT11_GPIO,DHT11_GPIO_PIN,1);
		//SysTick_DelayTicks(35U);

		delayus(40);

		GPIO_PinInit(DHT11_GPIO, DHT11_GPIO_PIN, &USER_DHT_in_config);
		//SysTick_DelayTicks(160U);

		delayus(180);
		uint8_t data[5] = {0};
		for(int i = 0; i < 5; i++)
		{
			for(int j = 7; j >= 0; j--)
			{
				data[i] |= (read_data_bit() << j);
			}
		 }
		PRINTF("%d, %d\r\n",data[0],data[2]);
        g_AdcConversionDoneFlag = false;
        PRINTF("Press any key to get user channel's ADC value.\r\n");
        GETCHAR();
        ADC_ETC_DoSoftwareTrigger(DEMO_ADC_ETC_BASE, 0U); /* Do software XBAR trigger0. */
        while (!g_AdcConversionDoneFlag)
        {
        }
        float humidity = (1- g_AdcConversionValue / 4096.0f) * 100.0f;
        PRINTF("濕度百分比: %d%%\r\n", (int)humidity);

        double input_features[3] = {data[2], data[0], humidity};
        PRINTF("score: %d\r\n",(int)score(input_features));
        PRINTF("suggest: %d\r\n",suggest(input_features));

    }
}

/*!
 * @brief Configure ADC to working with ADC_ETC.
 */
void ADC_Configuration(void)
{
    adc_config_t adcConfig;
    adc_channel_config_t adcChannelConfigStruct;

    /* Initialize the ADC module. */
    ADC_GetDefaultConfig(&adcConfig);
    ADC_Init(DEMO_ADC_BASE, &adcConfig);
    ADC_EnableHardwareTrigger(DEMO_ADC_BASE, true);

    adcChannelConfigStruct.channelNumber = DEMO_ADC_USER_CHANNEL; /* External channel selection from ADC_ETC. */
    adcChannelConfigStruct.enableInterruptOnConversionCompleted = false;
    ADC_SetChannelConfig(DEMO_ADC_BASE, DEMO_ADC_CHANNEL_GROUP, &adcChannelConfigStruct);

    /* Do auto hardware calibration. */
    if (kStatus_Success == ADC_DoAutoCalibration(DEMO_ADC_BASE))
    {
        //PRINTF("ADC_DoAutoCalibration() Done.\r\n");
    }
    else
    {
        //PRINTF("ADC_DoAutoCalibration() Failed.\r\n");
    }
}

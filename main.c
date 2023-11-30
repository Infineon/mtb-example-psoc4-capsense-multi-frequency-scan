/******************************************************************************
 * File Name:   main.c
 *
 * Description: This is the source code for PSoC4: CAPSENSE with Multi
 * Frequency Scan Application for ModusToolbox.
 *
 * Related Document: See README.md
 *
 *
 *******************************************************************************
 * Copyright 2023, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 *******************************************************************************/

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "cy_capsense_structure.h"
#include "cy_syslib.h"


/*******************************************************************************
* Macros
*******************************************************************************/
#ifdef COMPONENT_PSOC4100SMAX
#define CAPSENSE_MSC0_INTR_PRIORITY      (3u)
#define CAPSENSE_MSC1_INTR_PRIORITY      (3u)
#elif COMPONENT_PSOC4000T
#define CAPSENSE_MSC0_INTR_PRIORITY      (3u)
#else /* COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S and COMPONENT_PSOC4100SP256KB */
#define CAPSENSE_INTR_PRIORITY           (3u)
#endif
#define CAPSENSE_WIDGET_INACTIVE         (0u)
#define CY_ASSERT_FAILED                 (0u)

/* EZI2C interrupt priority must be higher than CAPSENSE interrupt */
#define EZI2C_INTR_PRIORITY              (2u)

#ifdef COMPONENT_PSOC4000T
/* Boolean constants */
#define LED_ON         (1u)
#define LED_OFF        (0u)
#else /* COMPONENT_PSOC4100SMAX, COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S and COMPONENT_PSOC4100SP256KB */
/* Boolean constants */
#define LED_ON         (0u)
#define LED_OFF        (1u)
#endif

#ifdef COMPONENT_PSOC4100SP
/* Define the number of slider LEDs in the board. The number of LEDs is equal
 *  to no of slider segments in CY8CKIT-149 kit
 */
#define NUM_LEDS (CY_CAPSENSE_LINEARSLIDER0_NUM_SNS_VALUE)

/* Defining step size for LED control based on centroid position of slider */
#define POSITION_STEP_SIZE (CY_CAPSENSE_LINEARSLIDER0_X_RESOLUTION_VALUE / NUM_LEDS)

/* Slider position threshold to make the corresponding LED glow */
#define  CYBSP_LED_SLD0_SLIDER_POS_THRESHOLD  (0 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD1_SLIDER_POS_THRESHOLD  (1 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD2_SLIDER_POS_THRESHOLD  (2 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD3_SLIDER_POS_THRESHOLD  (3 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD4_SLIDER_POS_THRESHOLD  (4 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD5_SLIDER_POS_THRESHOLD  (5 * POSITION_STEP_SIZE)
#else /* COMPONENT_PSOC4000S */
/* Define the number of slider LEDs in the board. The number of LEDs is equal
 *  to no of slider segments in CY8CKIT-145-40XX kit
 */
#define NUM_LEDS (CY_CAPSENSE_LINEARSLIDER0_NUM_SNS_VALUE)

/* Defining step size for LED control based on centroid position of slider */
#define POSITION_STEP_SIZE (CY_CAPSENSE_LINEARSLIDER0_X_RESOLUTION_VALUE / NUM_LEDS)

/* Slider position threshold to make the corresponding LED glow */
#define  CYBSP_LED_SLD0_SLIDER_POS_THRESHOLD  (0 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD1_SLIDER_POS_THRESHOLD  (1 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD2_SLIDER_POS_THRESHOLD  (2 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD3_SLIDER_POS_THRESHOLD  (3 * POSITION_STEP_SIZE)
#define  CYBSP_LED_SLD4_SLIDER_POS_THRESHOLD  (4 * POSITION_STEP_SIZE)
#endif


/*******************************************************************************
* Global Definitions
*******************************************************************************/
cy_stc_scb_ezi2c_context_t ezi2c_context;


/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void initialize_capsense(void);
#ifdef COMPONENT_PSOC4100SMAX
static void capsense_msc0_isr(void);
static void capsense_msc1_isr(void);
#elif COMPONENT_PSOC4000T
static void capsense_msc0_isr(void);
#else /* COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S and COMPONENT_PSOC4100SP256KB*/
static void capsense_isr(void);
#endif
static void ezi2c_isr(void);
static void initialize_capsense_tuner(void);
static void detect_touch_and_drive_led(void);


/*******************************************************************************
 * Function Name: main
 ********************************************************************************
 * Summary:
 *  System entrance point. This function performs
 *  - initial setup of device
 *  - initialize EZI2C and CAPSENSE peripherals
 *  - initialize tuner communication
 *  - scan touch input continuously
 *
 * Return:
 *  int
 *
 *******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize EZI2C */
    initialize_capsense_tuner();

    /* Initialize MSC CAPSENSE */
    initialize_capsense();

#if defined COMPONENT_PSOC4100SMAX || defined COMPONENT_PSOC4000T
    /* Start the first scan */
    Cy_CapSense_ScanAllSlots(&cy_capsense_context);
#else /* COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S and COMPONENT_PSOC4100SP256KB*/
    /* Initiate first scan */
    Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
#endif

    for (;;)
    {
        if(CY_CAPSENSE_NOT_BUSY == Cy_CapSense_IsBusy(&cy_capsense_context))
        {
            /* Process all widgets */
            Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);

            /* Turns LED ON/OFF based on button status */
            detect_touch_and_drive_led();

            /* Establishes synchronized communication with the CAPSENSE Tuner tool */
            Cy_CapSense_RunTuner(&cy_capsense_context);

#if defined COMPONENT_PSOC4100SMAX || defined COMPONENT_PSOC4000T
            /* Start the next scan */
            Cy_CapSense_ScanAllSlots(&cy_capsense_context);
#else /* COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S and COMPONENT_PSOC4100SP256KB*/
            /* Start the next scan */
            Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
#endif
        }
    }
}


/*******************************************************************************
* Function Name: initialize_capsense
********************************************************************************
* Summary:
*  This function initializes the CAPSENSE and configures the CAPSENSE
*  interrupt.
*
*******************************************************************************/
static void initialize_capsense(void)
{
    cy_capsense_status_t status = CY_CAPSENSE_STATUS_SUCCESS;

#ifdef COMPONENT_PSOC4100SMAX
    /* CAPSENSE interrupt configuration MSC 0 */
    const cy_stc_sysint_t capsense_msc0_interrupt_config =
    {
        .intrSrc = CY_MSC0_IRQ,
        .intrPriority = CAPSENSE_MSC0_INTR_PRIORITY,
    };

    /* CAPSENSE interrupt configuration MSC 1 */
    const cy_stc_sysint_t capsense_msc1_interrupt_config =
    {
        .intrSrc = CY_MSC1_IRQ,
        .intrPriority = CAPSENSE_MSC1_INTR_PRIORITY,
    };
#elif COMPONENT_PSOC4000T
    /* CAPSENSE interrupt configuration MSCLP 0 */
    const cy_stc_sysint_t capsense_msc0_interrupt_config =
    {
        .intrSrc = CY_MSCLP0_LP_IRQ,
        .intrPriority = CAPSENSE_MSC0_INTR_PRIORITY,
    };
#else /* COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S and COMPONENT_PSOC4100SP256KB*/
    /* CAPSENSE interrupt configuration */
    const cy_stc_sysint_t CapSense_interrupt_config =
    {
        .intrSrc = CYBSP_CSD_IRQ,
        .intrPriority = CAPSENSE_INTR_PRIORITY,
    };
#endif

    /* Capture the MSC HW block and initialize it to the default state */
    status = Cy_CapSense_Init(&cy_capsense_context);

    if (CY_CAPSENSE_STATUS_SUCCESS == status)
    {
#ifdef COMPONENT_PSOC4100SMAX
        /* Initialize CAPSENSE interrupt for MSC 0 */
        Cy_SysInt_Init(&capsense_msc0_interrupt_config, capsense_msc0_isr);
        NVIC_ClearPendingIRQ(capsense_msc0_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_msc0_interrupt_config.intrSrc);

        /* Initialize CAPSENSE interrupt for MSC 1 */
        Cy_SysInt_Init(&capsense_msc1_interrupt_config, capsense_msc1_isr);
        NVIC_ClearPendingIRQ(capsense_msc1_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_msc1_interrupt_config.intrSrc);
#elif COMPONENT_PSOC4000T
        /* Initialize CAPSENSE interrupt for MSC 0 */
        Cy_SysInt_Init(&capsense_msc0_interrupt_config, capsense_msc0_isr);
        NVIC_ClearPendingIRQ(capsense_msc0_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_msc0_interrupt_config.intrSrc);

#else /* COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S and COMPONENT_PSOC4100SP256KB */
        Cy_SysInt_Init(&CapSense_interrupt_config, capsense_isr);
        NVIC_ClearPendingIRQ(CapSense_interrupt_config.intrSrc);
        NVIC_EnableIRQ(CapSense_interrupt_config.intrSrc);
#endif

        /* Initialize the CAPSENSE firmware modules */
        status = Cy_CapSense_Enable(&cy_capsense_context);
    }

    if(status != CY_CAPSENSE_STATUS_SUCCESS)
    {
        /* This status could fail before tuning the sensors correctly.
         * Ensure that this function passes after the CAPSENSE sensors are tuned
         * as per procedure given in the README.md file */
    }
}


#ifdef COMPONENT_PSOC4100SMAX
/*******************************************************************************
* Function Name: capsense_msc0_isr
********************************************************************************
* Summary:
*  Function for handling interrupts from CAPSENSE MSC0 block.
*
*******************************************************************************/
static void capsense_msc0_isr(void)
{
    Cy_CapSense_InterruptHandler(CY_MSC0_HW, &cy_capsense_context);
}


/*******************************************************************************
* Function Name: capsense_msc1_isr
********************************************************************************
* Summary:
*  Function for handling interrupts from CAPSENSE MSC1 block.
*
*******************************************************************************/
static void capsense_msc1_isr(void)
{
    Cy_CapSense_InterruptHandler(CY_MSC1_HW, &cy_capsense_context);
}
#elif COMPONENT_PSOC4000T
/*******************************************************************************
* Function Name: capsense_msc0_isr
********************************************************************************
* Summary:
*  Function for handling interrupts from CAPSENSE MSC0 block.
*
*******************************************************************************/
static void capsense_msc0_isr(void)
{
    Cy_CapSense_InterruptHandler(CY_MSCLP0_HW, &cy_capsense_context);
}

#else /* COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S and COMPONENT_PSOC4100SP256KB*/
/*******************************************************************************
 * Function Name: capsense_isr
 ********************************************************************************
 * Summary:
 *  Function for handling interrupts from CAPSENSE block.
 *
 *******************************************************************************/
static void capsense_isr(void)
{
    Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
}
#endif


/*******************************************************************************
* Function Name: initialize_capsense_tuner
********************************************************************************
* Summary:
* EZI2C module to communicate with the CAPSENSE Tuner tool.
*
*******************************************************************************/
static void initialize_capsense_tuner(void)
{
    cy_en_scb_ezi2c_status_t status = CY_SCB_EZI2C_SUCCESS;

    /* EZI2C interrupt configuration structure */
    const cy_stc_sysint_t ezi2c_intr_config =
    {
        .intrSrc = CYBSP_EZI2C_IRQ,
        .intrPriority = EZI2C_INTR_PRIORITY,
    };

    /* Initialize the EzI2C firmware module */
    status = Cy_SCB_EZI2C_Init(CYBSP_EZI2C_HW, &CYBSP_EZI2C_config, &ezi2c_context);

    if(status != CY_SCB_EZI2C_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    Cy_SysInt_Init(&ezi2c_intr_config, ezi2c_isr);
    NVIC_EnableIRQ(ezi2c_intr_config.intrSrc);

    /* Set the CAPSENSE data structure as the I2C buffer to be exposed to the
     * master on primary slave address interface. Any I2C host tools such as
     * the Tuner or the Bridge Control Panel can read this buffer but you can
     * connect only one tool at a time.
     */
    Cy_SCB_EZI2C_SetBuffer1(CYBSP_EZI2C_HW, (uint8_t *)&cy_capsense_tuner,
            sizeof(cy_capsense_tuner), sizeof(cy_capsense_tuner),
            &ezi2c_context);

    Cy_SCB_EZI2C_Enable(CYBSP_EZI2C_HW);
}


/*******************************************************************************
* Function Name: detect_touch_and_drive_led
********************************************************************************
* Summary:
* Turning LED ON/OFF based on button status
*
*******************************************************************************/
static void detect_touch_and_drive_led(void)
{
#if defined COMPONENT_PSOC4100SMAX || defined COMPONENT_PSOC4100SP256KB || defined COMPONENT_PSOC4000T
    if(CAPSENSE_WIDGET_INACTIVE != Cy_CapSense_IsWidgetActive(CY_CAPSENSE_BUTTON0_WDGT_ID, &cy_capsense_context))
    {
        Cy_GPIO_Write(CYBSP_LED_BTN0_PORT, CYBSP_LED_BTN0_NUM, LED_ON);
    }
    else
    {
        Cy_GPIO_Write(CYBSP_LED_BTN0_PORT, CYBSP_LED_BTN0_NUM, LED_OFF);
    }

#ifdef COMPONENT_PSOC4100SMAX
    if(CAPSENSE_WIDGET_INACTIVE != Cy_CapSense_IsWidgetActive(CY_CAPSENSE_BUTTON1_WDGT_ID, &cy_capsense_context))
    {
        Cy_GPIO_Write(CYBSP_LED_BTN1_PORT, CYBSP_LED_BTN1_NUM, LED_ON);
    }
    else
    {
        Cy_GPIO_Write(CYBSP_LED_BTN1_PORT, CYBSP_LED_BTN1_NUM, LED_OFF);
    }
#endif
    if(CAPSENSE_WIDGET_INACTIVE != Cy_CapSense_IsWidgetActive(CY_CAPSENSE_LINEARSLIDER0_WDGT_ID, &cy_capsense_context))
    {
        Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_NUM, LED_ON);
    }
    else
    {
        Cy_GPIO_Write(CYBSP_USER_LED_PORT, CYBSP_USER_LED_NUM, LED_OFF);
    }

#else /* COMPONENT_PSOC4100SP, COMPONENT_PSOC4000S */
    cy_stc_capsense_touch_t* slider_touch_info;
    uint16_t slider_pos;

    /* Turn ON/OFF LEDs based on the status of the corresponding CAPSENSE buttons */
    Cy_GPIO_Write(CYBSP_LED_BTN0_PORT, CYBSP_LED_BTN0_NUM, (Cy_CapSense_IsSensorActive(CY_CAPSENSE_BUTTON0_WDGT_ID, CY_CAPSENSE_BUTTON0_SNS0_ID, &cy_capsense_context) ? LED_ON : LED_OFF ));
    Cy_GPIO_Write(CYBSP_LED_BTN1_PORT, CYBSP_LED_BTN1_NUM, (Cy_CapSense_IsSensorActive(CY_CAPSENSE_BUTTON1_WDGT_ID, CY_CAPSENSE_BUTTON1_SNS0_ID, &cy_capsense_context) ? LED_ON : LED_OFF ));
    Cy_GPIO_Write(CYBSP_LED_BTN2_PORT, CYBSP_LED_BTN2_NUM, (Cy_CapSense_IsSensorActive(CY_CAPSENSE_BUTTON2_WDGT_ID, CY_CAPSENSE_BUTTON2_SNS0_ID, &cy_capsense_context) ? LED_ON : LED_OFF ));

    /* Drive slider LEDs based on finger position on slider if
     * slider is touched.
     */
    if(Cy_CapSense_IsWidgetActive(CY_CAPSENSE_LINEARSLIDER0_WDGT_ID,&cy_capsense_context))
    {
        /* Get the touch position(centroid) of CAPSENSE Linear Slider */
        slider_touch_info = Cy_CapSense_GetTouchInfo(CY_CAPSENSE_LINEARSLIDER0_WDGT_ID, &cy_capsense_context);
        slider_pos = slider_touch_info->ptrPosition->x;

        /* Turn ON LEDs based on the finger position (centroid) on the
         * CAPSENSE Linear slider
         */
        Cy_GPIO_Write(CYBSP_LED_SLD0_PORT, CYBSP_LED_SLD0_NUM, ((slider_pos > CYBSP_LED_SLD0_SLIDER_POS_THRESHOLD) ? LED_ON : LED_OFF));
        Cy_GPIO_Write(CYBSP_LED_SLD1_PORT, CYBSP_LED_SLD1_NUM, ((slider_pos > CYBSP_LED_SLD1_SLIDER_POS_THRESHOLD) ? LED_ON : LED_OFF));
        Cy_GPIO_Write(CYBSP_LED_SLD2_PORT, CYBSP_LED_SLD2_NUM, ((slider_pos > CYBSP_LED_SLD2_SLIDER_POS_THRESHOLD) ? LED_ON : LED_OFF));
        Cy_GPIO_Write(CYBSP_LED_SLD3_PORT, CYBSP_LED_SLD3_NUM, ((slider_pos > CYBSP_LED_SLD3_SLIDER_POS_THRESHOLD) ? LED_ON : LED_OFF));
        Cy_GPIO_Write(CYBSP_LED_SLD4_PORT, CYBSP_LED_SLD4_NUM, ((slider_pos > CYBSP_LED_SLD4_SLIDER_POS_THRESHOLD) ? LED_ON : LED_OFF));
#ifdef COMPONENT_PSOC4100SP
        Cy_GPIO_Write(CYBSP_LED_SLD5_PORT, CYBSP_LED_SLD5_NUM, ((slider_pos > CYBSP_LED_SLD5_SLIDER_POS_THRESHOLD) ? LED_ON : LED_OFF));
#endif
    }
    else
    {
        /* If the slider is not touched, turn OFF all slider related LEDs */
        Cy_GPIO_Write(CYBSP_LED_SLD0_PORT, CYBSP_LED_SLD0_NUM, (LED_OFF));
        Cy_GPIO_Write(CYBSP_LED_SLD1_PORT, CYBSP_LED_SLD1_NUM, (LED_OFF));
        Cy_GPIO_Write(CYBSP_LED_SLD2_PORT, CYBSP_LED_SLD2_NUM, (LED_OFF));
        Cy_GPIO_Write(CYBSP_LED_SLD3_PORT, CYBSP_LED_SLD3_NUM, (LED_OFF));
        Cy_GPIO_Write(CYBSP_LED_SLD4_PORT, CYBSP_LED_SLD4_NUM, (LED_OFF));
#ifdef COMPONENT_PSOC4100SP
        Cy_GPIO_Write(CYBSP_LED_SLD5_PORT, CYBSP_LED_SLD5_NUM, (LED_OFF));
#endif

    }
#endif
}


/*******************************************************************************
* Function Name: ezi2c_isr
********************************************************************************
* Summary:
* Function for handling interrupts from EZI2C block.
*
*******************************************************************************/
static void ezi2c_isr(void)
{
    Cy_SCB_EZI2C_Interrupt(CYBSP_EZI2C_HW, &ezi2c_context);
}


/* [] END OF FILE */

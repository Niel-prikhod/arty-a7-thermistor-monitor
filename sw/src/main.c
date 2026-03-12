#include <stdio.h>
#include <math.h>
#include "xparameters.h"
#include "xsysmon.h"
#include "sleep.h"

#define SYSMON_BASEADDR XPAR_XSYSMON_0_BASEADDR

#define VAUX4_CHANNEL (XSM_CH_AUX_MIN + 4)

XSysMon SysMonInst;

int init_xadc() {
    int Status;
    XSysMon_Config *ConfigPtr;

    // 1. Look up the hardware configuration using the Base Address
    ConfigPtr = XSysMon_LookupConfig(SYSMON_BASEADDR);
    if (ConfigPtr == NULL) {
        printf("Error: Could not find XADC configuration!\r\n");
        return XST_FAILURE;
    }

    // 2. Initialize the driver
    Status = XSysMon_CfgInitialize(&SysMonInst, ConfigPtr, ConfigPtr->BaseAddress);
    if (Status != XST_SUCCESS) {
        printf("Error: Could not initialize XADC!\r\n");
        return XST_FAILURE;
    }

    // 3. CRITICAL: Configure the XADC Sequencer
    // By default, the XADC only measures internal temperature and voltage.
    // We must explicitly tell it to monitor our external VAUX4 pin.
    
    // Stop the sequencer while we configure it
    XSysMon_SetSequencerMode(&SysMonInst, XSM_SEQ_MODE_SAFE);
    
    // Enable VAUX4 in the sequence
    Status = XSysMon_SetSeqChEnables(&SysMonInst, XSM_SEQ_CH_AUX04);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // Restart the sequencer in continuous automatic mode
    XSysMon_SetSequencerMode(&SysMonInst, XSM_SEQ_MODE_CONTINPASS);

    return XST_SUCCESS;
}

int main() {
    printf("\r\n--- Arty A7 Thermistor Monitor Started ---\r\n");

    if (init_xadc() != XST_SUCCESS) {
        printf("XADC Initialization Failed. Halting.\r\n");
        while(1); // Halt execution
    }

    printf("XADC Initialized Successfully. Reading VAUX4 (Pin A0)...\r\n\n");

    while (1) {
        // 1. Read the raw 16-bit register for VAUX4
        u16 adc_raw = XSysMon_GetAdcData(&SysMonInst, VAUX4_CHANNEL);

        // 2. The 12 bits of actual data are left-justified, so shift right by 4
        u16 adc_12bit = adc_raw >> 4;

        // 3. Convert to Voltage (XADC max range is 1.0V at a value of 4096)
        // We cast to (float) to prevent standard integer division (which would result in 0)
        float voltage = (float)adc_12bit / 4096.0f;

        // Prevent division by zero if the pin is shorted to ground
        if (voltage <= 0.01f) {
            printf("Error: Voltage too low. Check wiring!\r\n");
            sleep(1);
            continue;
        }

        // 4. Calculate Thermistor Resistance
        // Assuming: 1.0V -> Thermistor -> A0 (VAUX4) -> 10k Resistor -> GND
        float resistance = 10000.0f * ((1.0f / voltage) - 1.0f);

        // 5. Calculate Temperature using the Steinhart-Hart equation
        float steinhart;
        steinhart = resistance / 10000.0f;            // (R/Ro)
        steinhart = log(steinhart);                   // ln(R/Ro)
        steinhart /= 3950.0f;                         // 1/B * ln(R/Ro)
        steinhart += 1.0f / (25.0f + 273.15f);        // + (1/To)
        steinhart = 1.0f / steinhart;                 // Invert
        steinhart -= 273.15f;                         // Convert Kelvin to Celsius

        // 6. Print the results
        printf("Raw: %4d | Voltage: %.3f V | Res: %6.0f Ohms | Temp: %.2f C\r\n", 
               adc_12bit, voltage, resistance, steinhart);

        // Wait 1 second before the next reading
        sleep(1);
    }

    return 0;
}

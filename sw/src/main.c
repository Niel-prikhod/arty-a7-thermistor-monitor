#include <stdio.h>
#include <math.h>
#include "xparameters.h"
#include "xsysmon.h"
#include "sleep.h"

#define SYSMON_BASEADDR XPAR_XSYSMON_0_BASEADDR

#define VAUX4_CHANNEL (XSM_CH_AUX_MIN + 4)

#define V_REF		3.3f
#define RESISTOR	10000.0f
#define R107		2320
#define R140		1000
#define SH_EQ_C1	1.129148e-3
#define SH_EQ_C2	2.34125e-4
#define SH_EQ_C3	8.76741e-8
#define ABS_ZERO	273.15

typedef struct signal_s {
	float	resistance;
	float	temperature;
}	signal_t;

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

signal_t calc_physics(float v_out) {
	signal_t	signal;
	float		logR;
	float		v_sig;

	v_sig = v_out * (R107 + R140) / R140;
	signal.resistance = v_sig * RESISTOR / (V_REF - v_sig);
	// signal.resistance = ((voltage / V_REF) * RESISTOR) / (1.0 - (voltage / V_REF));
	logR = logf(signal.resistance);
	signal.temperature = (1.0 / (SH_EQ_C1 + SH_EQ_C2 * logR + SH_EQ_C3 * logR * logR * logR)) - ABS_ZERO;
	return signal;
}

int main() {
	signal_t signal;

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
        float voltage = ((float)adc_12bit / 4096.0f) * V_REF;

        // Prevent division by zero if the pin is shorted to ground
        if (voltage <= 0.01f) {
            printf("Error: Voltage too low. Check wiring!\r\n");
            sleep(1);
            continue;
        }

		signal = calc_physics(voltage);

        // 6. Print the results
        printf("%4d,%.3f,%d,%.2f\r\n", 
               adc_12bit, voltage, signal.resistance, signal.temperature);

        sleep(1);
    }

    return 0;
}

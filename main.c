#include <stdio.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"

#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"

/* Task */
#define STACKSIZE 2048
Char taskStack[STACKSIZE];
// Char commTaskStack[STACKSIZE];

//Pin configuration and variables
static PIN_Handle buttonHandle;
static PIN_State buttonState;

static PIN_Handle ledHandle;
static PIN_State ledState;


float testData[100][6] = {
    {0.01,0.02,-0.98,0.95,0.53,0.51},
    {-0.00,0.02,-0.98,1.01,0.84,-0.51},
    {-0.01,0.02,-0.99,1.03,0.03,-0.35},
    {-0.00,0.00,-1.00,0.37,0.03,0.33},
    {-0.00,0.02,-1.00,0.53,0.01,1.17},
    {0.00,0.02,-0.99,0.63,0.42,0.67},
    {0.01,0.01,-0.97,0.91,1.23,-0.02},
    {-0.00,0.02,-0.97,1.25,0.76,0.20},
    {-0.01,0.02,-0.99,1.43,0.24,0.84},
    {-0.01,0.03,-0.98,0.92,0.14,0.20},
    {0.01,0.03,-1.00,0.37,0.72,0.48},
    {-0.00,0.02,-0.99,0.93,0.82,1.21},
    {0.02,0.03,-0.96,1.76,-0.10,1.10},
    {-0.00,0.04,-0.98,1.87,-0.60,-0.22},
    {-0.00,0.03,-0.99,0.64,0.96,0.82},
    {-0.01,0.04,-1.00,1.16,0.90,1.81},
    {0.00,0.02,-0.97,1.14,0.16,0.15},
    {0.01,0.03,-1.01,0.83,0.24,1.16},
    {0.02,0.04,-0.98,0.82,0.53,0.76},
    {-0.02,0.02,-0.97,1.27,0.01,0.08},
    {-0.01,0.02,-0.97,0.27,0.24,-0.60},
    {-0.00,0.04,-0.97,1.26,0.25,0.08},
    {0.00,0.02,-0.97,0.92,0.47,0.67},
    {-0.00,0.03,-0.98,1.79,-0.12,1.05},
    {-0.00,0.02,-0.98,1.55,0.34,0.17},
    {-0.01,0.04,-0.98,0.73,0.42,-0.21},
    {-0.00,0.03,-1.00,0.62,0.45,-0.18},
    {0.00,0.03,-0.98,1.48,0.60,1.19},
    {0.00,0.02,-0.98,1.69,-0.26,-0.47},
    {0.00,0.00,-1.00,0.93,1.46,0.73},
    {0.01,0.01,-0.99,1.52,-0.06,1.30},
    {-0.00,0.01,-0.97,0.91,0.54,0.40},
    {0.01,0.03,-0.97,0.63,0.51,0.25},
    {0.01,0.03,-0.98,0.72,0.97,-0.27},
    {0.01,0.02,-1.00,0.60,0.81,0.93},
    {0.01,0.02,-0.99,1.08,-0.03,-0.50},
    {0.01,0.03,-0.98,1.08,1.07,1.02},
    {-0.01,0.03,-0.99,0.27,-0.25,0.08},
    {0.01,0.02,-0.99,1.09,0.50,0.16},
    {-0.00,0.02,-0.99,1.14,-0.63,-0.01},
    {0.01,0.02,-0.99,0.53,1.24,1.43},
    {-0.01,0.02,-0.98,1.10,0.69,0.40},
    {0.02,0.02,-0.99,0.51,0.27,0.74},
    {-0.00,0.03,-1.00,1.10,0.31,0.18},
    {0.00,0.04,-0.99,0.76,0.74,-0.18},
    {-0.01,0.03,-0.99,0.61,0.90,0.53},
    {0.01,0.02,-0.99,1.19,0.61,-0.62},
    {0.00,0.03,-0.98,1.33,-0.02,0.47},
    {0.00,0.02,-0.98,0.97,-0.48,0.58},
    {0.00,0.03,-0.97,0.40,0.59,0.60},
    {0.01,0.02,-0.99,0.91,0.66,0.74},
    {0.01,0.02,-1.00,0.56,0.53,1.12},
    {-0.00,0.02,-0.97,0.59,0.85,0.37},
    {0.02,0.02,-1.00,1.05,0.89,0.42},
    {-0.01,0.03,-1.00,1.37,-0.09,1.08},
    {-0.00,0.03,-0.99,1.00,0.21,0.28},
    {0.02,0.03,-0.97,0.14,-0.14,0.73},
    {-0.01,0.02,-0.99,-0.10,0.66,1.17},
    {-0.01,0.03,-0.99,1.25,0.82,1.44},
    {0.01,0.04,-0.97,0.92,1.17,-0.25},
    {0.01,0.02,-1.01,0.72,0.56,1.15},
    {0.00,0.03,-0.98,0.25,0.53,-0.08},
    {-0.00,0.01,-1.00,1.32,0.24,0.95},
    {0.01,0.03,-0.98,1.16,-0.50,1.08},
    {0.01,0.02,-0.98,0.05,-0.15,0.23},
    {0.02,0.02,-0.99,0.76,0.85,0.53},
    {-0.01,0.03,-0.98,1.18,0.54,0.54},
    {0.01,0.02,-0.99,0.94,0.48,0.57},
    {-0.01,0.05,-1.00,0.81,0.55,0.86},
    {0.00,0.04,-0.99,0.58,-0.20,-0.21},
    {-0.01,0.04,-0.99,0.05,0.70,1.56},
    {-0.00,0.03,-0.99,1.56,0.92,0.71},
    {0.00,0.01,-0.97,0.73,-0.17,0.92},
    {-0.00,0.03,-1.00,0.43,1.07,0.90},
    {-0.01,0.04,-0.98,0.66,0.33,1.35},
    {-0.00,0.02,-0.99,0.60,1.08,0.50},
    {-0.01,0.02,-1.00,0.71,1.81,1.53},
    {0.01,0.03,-0.99,0.86,-0.05,0.61},
    {-0.01,0.03,-0.96,0.83,0.63,1.24},
    {-0.00,0.01,-0.97,0.31,0.43,0.29},
    {0.00,0.02,-0.97,1.33,1.02,1.43},
    {-0.00,0.01,-0.97,0.37,0.59,0.64},
    {-0.01,0.04,-0.98,0.55,0.26,0.32},
    {0.01,0.02,-0.98,0.32,0.18,0.27},
    {0.02,0.03,-1.00,1.07,0.61,0.56},
    {-0.01,0.01,-0.98,1.21,0.65,0.64},
    {0.00,0.02,-0.99,1.35,0.76,0.84},
    {-0.01,0.01,-0.98,1.08,0.24,0.08},
    {-0.00,0.04,-0.98,0.53,0.17,-0.13},
    {0.02,0.01,-0.98,0.34,-0.14,0.86},
    {0.01,0.03,-1.00,0.47,0.06,0.25},
    {0.01,0.02,-0.99,1.38,0.38,0.76},
    {0.01,0.03,-0.98,0.43,-0.14,0.89},
    {0.01,0.03,-0.98,0.02,-0.68,-0.22},
    {-0.00,0.04,-1.01,0.45,0.37,0.53},
    {0.00,0.03,-0.98,0.92,1.91,0.25},
    {0.00,0.03,-0.98,0.60,0.79,0.24},
    {-0.00,0.02,-0.96,1.30,0.45,0.82},
    {-0.00,0.03,-0.98,0.82,-0.01,0.66},
    {0.03,0.03,-1.00,1.39,0.08,0.40}};

PIN_Config buttonConfig[] = {
Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
PIN_TERMINATE };

PIN_Config ledConfig[] = {
Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
PIN_TERMINATE };

//motion sensor
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
Board_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX, PIN_TERMINATE };

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = { .pinSDA = Board_I2C0_SDA1, .pinSCL = Board_I2C0_SCL1 };

void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
	PIN_setOutputValue(ledHandle, Board_LED1, !PIN_getOutputValue( Board_LED1));
}

/* Task Functions */

/*Data collection task*/
Void sensorFxn(UArg arg0, UArg arg1) {
	float ax, ay, az, gx, gy, gz;
/*
  	I2C_Handle i2c; // INTERFACE FOR OTHER SENSORS
	I2C_Params i2cParams;
	I2C_Params_init(&i2cParams);
	i2cParams.bitRate = I2C_400kHz;
*/

	I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams;
	I2C_Params_init(&i2cMPUParams);
	i2cMPUParams.bitRate = I2C_400kHz;
	i2cMPUParams.custom = (uintptr_t) &i2cMPUCfg;

	//Open ic2 connection for motion sensor
	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	if (i2cMPU == NULL) {
		System_abort("Error Initializing I2CMPU\n");
	}

	//Power on sensor
	PIN_setOutputValue(hMpuPin, Board_MPU_POWER, Board_MPU_POWER_ON);

	// WAIT 100MS FOR THE SENSOR TO POWER UP
	Task_sleep(100000 / Clock_tickPeriod);
	System_printf("MPU9250: Power ON\n");
	System_flush();

	// Motion sensor SETUP
	System_printf("MPU9250: Setup and calibration...\n");
	System_flush();
	mpu9250_setup(&i2cMPU);
	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();
	I2C_close(i2cMPU);

	int i = 0;

	while (1) {
		char str[128];
		//Collect data from motion sensor
		i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
		if (i2cMPU == NULL) {
			System_abort("Error Initializing I2CMPU\n");
		}
		if(i < 100){
			ax=testData[i][0];
			ay=testData[i][1];
			az=testData[i][2];
			gx=testData[i][3];
			gy=testData[i][4];
			gz=testData[i][5];
			i++;
		}
		//mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
		I2C_close(i2cMPU);
		sprintf(str,"ax: %f   ay: %f   az: %f   gx: %f   gy: %f   gz: %f\n", ax, ay, az, gx, gy, gz);
		System_printf(str);
		System_flush();
		Task_sleep(100000 / Clock_tickPeriod);
	}
}

/* Communication Task */
/*
 Void commTaskFxn(UArg arg0, UArg arg1) {

 // Radio to receive mode
 int32_t result = StartReceive6LoWPAN();
 if(result != true) {
 System_abort("Wireless receive mode failed");
 }

 while (1) {

 // If true, we have a message
 if (GetRXFlag() == true) {

 // Handle the received message..
 }

 // Absolutely NO Task_sleep in this task!!
 }
 }
 */

Int main(void) {

	Task_Handle task;
	Task_Params taskParams;

	Board_initGeneral();
	Board_initI2C();

	hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
	if (hMpuPin == NULL) {
		System_abort("Pin open failed!");
	}

	Task_Params_init(&taskParams);
	taskParams.stackSize = STACKSIZE;
	taskParams.stack = &taskStack;
	task = Task_create((Task_FuncPtr) sensorFxn, &taskParams, NULL);
	if (task == NULL) {
		System_abort("Task create failed!");
	}

	/* Communication Task */
	/*
	 Init6LoWPAN(); // This function call before use!

	 Task_Params_init(&commTaskParams);
	 commTaskParams.stackSize = STACKSIZE;
	 commTaskParams.stack = &commTaskStack;
	 commTaskParams.priority=1;

	 commTask = Task_create(commTaskFxn, &commTaskParams, NULL);
	 if (commTask == NULL) {
	 System_abort("Task create failed!");
	 }
	 */

	/* Sanity check */
	System_printf("Hello world!\n");
	System_flush();

	/* Start BIOS */
	BIOS_start();

	return (0);
}

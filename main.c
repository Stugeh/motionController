
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
#include "sensors/mpu9250.h"

/* Task */
#define STACKSIZE 2048
Char taskStack[STACKSIZE];
FILE * fptr;
// Char commTaskStack[STACKSIZE];

//motion sensor
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
		Board_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX, PIN_TERMINATE };

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = { .pinSDA = Board_I2C0_SDA1, .pinSCL = Board_I2C0_SCL1 };


/* Task Functions */

//data struct for MPU data
struct motionData{
	float ax, ay, az, gx, gy, gz;
	UInt32 time;
};

//Stores last 5 seconds of motion data from MPU9250.
struct motionData MPUData[40] = {NULL};
struct motionData xData[100] = {NULL};


int forPrinting = 0;
//calculates moving average to clean up data
void movavg(int i){
	//how many datapoints to get the average from
	int window=9;
		float axTot = 0, ayTot = 0, azTot = 0, gxTot = 0, gyTot = 0, gzTot = 0;
		int j;
		for(j=i; j > i - window; j--){
			axTot = axTot + MPUData[j].ax;
			ayTot = ayTot + MPUData[j].ay;
			azTot = azTot + MPUData[j].az;
			gxTot = gxTot + MPUData[j].gx;
			gyTot = gyTot + MPUData[j].gy;
			gzTot = gzTot + MPUData[j].gz;
		}
		MPUData[i].ax = axTot / window;
		MPUData[i].ay = ayTot / window;
		MPUData[i].az = azTot / window;
		MPUData[i].gx = gxTot / window;
		MPUData[i].gy = gyTot / window;
		MPUData[i].gz = gzTot / window;

		xData[forPrinting] = MPUData[i];
}
/*Data collection task*/
Void sensorFxn(UArg arg0, UArg arg1) {
	float ax, ay, az, gx, gy, gz;
/*
 	//opening data file to monitor sensor via python script
	fptr = fopen("data.csv", "w");
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
	System_printf("MPU9250: Setup and calibration OK\nStarting data collection\n");
	System_flush();
	I2C_close(i2cMPU);


	int i = 0;
	UInt32 timestamp;
	while (1) {
		struct motionData dataPoint;
		//Collect data from motion sensor
		i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
		if (i2cMPU == NULL) {
			System_abort("Error Initializing I2CMPU\n");
		}
		mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
		timestamp = Clock_getTicks()/Clock_tickPeriod;
		I2C_close(i2cMPU);

		//write data to a datapoint and add to array
		dataPoint.ax = ax;
		dataPoint.ay = ay;
		dataPoint.az = az;
		dataPoint.gx = gx;
		dataPoint.gy = gy;
		dataPoint.gz = gz;
		dataPoint.time = timestamp;
		MPUData[i] = dataPoint;

		//movavg(i);
		xData[forPrinting] = MPUData[i];
		//write datapoint to a csv for monitoring via python script
		//fprintf(fptr,"%d,%f,%f,%f,%f,%f,%f\n",timestamp, ax, ay, az, gx, gy, gz);


		i++;
		forPrinting++;
		if(forPrinting==100){
			System_printf("time,ax,ay,az,gx,gy,gz\n");
			System_flush();
			int n;
			for(n=0;n<forPrinting; n++){
				System_printf("%d,%f,%f,%f,%f,%f,%f\n",xData[n].time, xData[n].ax, xData[n].ay, xData[n].az,xData[n].gx, xData[n].gy, xData[n].gz);
				System_flush();
			}

		}
		if(i==40){
			i=0;
		}
		Task_sleep(100000/Clock_tickPeriod);
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

	/* Start BIOS */
	BIOS_start();

	return (0);
}

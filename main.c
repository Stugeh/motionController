
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
//#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"

#include "wireless/comm_lib.h"
#include "sensors/mpu9250.h"

//Image bitmaps for game
#include "images.h"

/* Task */
#define STACKSIZE 2048
Char dataTaskStack[STACKSIZE];
Char displayTaskStack[STACKSIZE];

// Char commTaskStack[STACKSIZE];

//motion sensor
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
	Board_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX, PIN_TERMINATE};

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {.pinSDA = Board_I2C0_SDA1, .pinSCL = Board_I2C0_SCL1};

//Button config
static PIN_Handle buttonHandle;
static PIN_State buttonState;
PIN_Config buttonConfig[] = {
	Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
	Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
	PIN_TERMINATE};

//data struct for MPU data
struct motionData
{
	float ax, ay, az, gx, gy, gz;
	UInt32 time;
};

/***GLOBAL VARIABLES***/

//Stores last 4 seconds of motion data from MPU9250.
struct motionData MPUData[40] = {NULL};

//test array for storing 10 seconds of motion data before printing.
//struct motionData testData[100] = {NULL};
//test array iterator
//uint8_t forPrinting = 0;

//game state
enum state
{
	MENU = 1,
	GAME,
	HELP,
};

enum move
{
	STILL = 1,
	UP,
	DOWN,
	LEFT,
	RIGHT
};

enum state currState = MENU;
enum state prevState = MENU;
enum move currMove = STILL;
//current selection when in menu
uint8_t select = 0;

//file pointer for writing test data to file.
//FILE *fptr;

/* Task Functions */

//calculates moving average to clean up data
void movavg(uint8_t i)
{
	//how many datapoints to get the average from
	uint8_t window = 2;
	//New values for every sensor axis
	float axTot = 0, ayTot = 0, azTot = 0, gxTot = 0, gyTot = 0, gzTot = 0;
	int8_t j;
	for (j = i; j > i - window; j--)
	{
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
	//testData[forPrinting] = MPUData[i];
}

/*detects moves*/
void moveDetection(uint8_t i)
{
	uint8_t gThreshold = 20;
	uint32_t sleep = 150000;
	/*Using gyroscope to determine direction*/
	if (MPUData[i].gx > gThreshold)
	{
		System_printf("U\n");
		System_flush();
		currMove = UP;
		Task_sleep(sleep / Clock_tickPeriod);
	}
	if (MPUData[i].gx < -gThreshold)
	{
		System_printf("D\n");
		System_flush();
		currMove = DOWN;
		Task_sleep(sleep / Clock_tickPeriod);
	}
	if (MPUData[i].gy > gThreshold)
	{
		System_printf("R\n");
		System_flush();
		currMove = RIGHT;
		Task_sleep(sleep / Clock_tickPeriod);
	}
	if (MPUData[i].gy < -gThreshold)
	{
		System_printf("L\n");
		System_flush();
		currMove = LEFT;
		Task_sleep(sleep / Clock_tickPeriod);
	}
	//currMove = STILL;
}

/*Data collection task*/
Void sensorFxn(UArg arg0, UArg arg1)
{
	float ax, ay, az, gx, gy, gz;
	/*
 	//opening data file to monitor sensor via python script
	fptr = fopen("data.csv", "w");
	 */

	I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams;
	I2C_Params_init(&i2cMPUParams);
	i2cMPUParams.bitRate = I2C_400kHz;
	i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

	//Open ic2 connection for motion sensor
	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	if (i2cMPU == NULL)
	{
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
	while (1)
	{
		if (currState == GAME)
		{
			struct motionData dataPoint;
			//Collect data from motion sensor
			i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
			if (i2cMPU == NULL)
			{
				System_abort("Error Initializing I2CMPU\n");
			}
			mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
			timestamp = Clock_getTicks() / Clock_tickPeriod;
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

			movavg(i);
			//testData[forPrinting] = MPUData[i];
			moveDetection(i);
			i++;
			if (i == 40)
			{
				i = 0;
			}

			/*
			forPrinting++;
			if (forPrinting == 100)
			{
				System_printf("time,ax,ay,az,gx,gy,gz\n");
				System_flush();
				int n;
				for (n = 0; n < forPrinting; n++)
				{
					System_printf("%d,%f,%f,%f,%f,%f,%f\n", testData[n].time, testData[n].ax, testData[n].ay, testData[n].az, testData[n].gx, testData[n].gy, testData[n].gz);
					System_flush();
				}
			}
			*/
		}
		Task_sleep(100000 / Clock_tickPeriod);
	}
}

/*Button handler*/
void buttonFxn(PIN_Handle handle, PIN_Id pinId)
{
	uint8_t numOfSelections = 2;
	if (currState != GAME)
	{
		switch (pinId)
		{

		case Board_BUTTON0:
			if (select == 0)
			{
				currState = GAME;
			}
			else if (select == 1)
			{
				currState = HELP;
			}
			break;
		case Board_BUTTON1:
			if (select == numOfSelections - 1)
			{
				select = 0;
			}
			else
			{
				select++;
			}
			break;
		default:
			break;
		}
	}
}

/*Draws graphics on the integrated display*/
void displayFxn(UArg arg0, UArg arg1)
{
	// uint8_t UP_MAP[8] = {0x18, 0x3c, 0x7e, 0xff, 0xff, 0x3c, 0x3c, 0x3c};
	// uint8_t DOWN_MAP[8] = {0x3c, 0x3c, 0x3c, 0xff, 0xff, 0x7e, 0x3c, 0x18};
	// uint8_t LEFT_MAP[8] = {0x18, 0x38, 0x7f, 0xff, 0xff, 0x7f, 0x38, 0x18};
	// uint8_t RIGHT_MAP[8] = {0x18, 0x1c, 0x7f, 0xff, 0xff, 0x7f, 0x1c, 0x18};

	uint32_t imgPalette[] = {0, 0xFFFFFF};

	const tImage up_img = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 64,
		.YSize = 64,
		.pPalette = imgPalette,
		.pPixel = UP_MAP};

	const tImage down_img = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 64,
		.YSize = 64,
		.pPalette = imgPalette,
		.pPixel = DOWN_MAP};

	const tImage left_img = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 60,
		.YSize = 64,
		.pPalette = imgPalette,
		.pPixel = LEFT_MAP};

	const tImage right_img = {
		.BPP = IMAGE_FMT_1BPP_UNCOMP,
		.NumColors = 2,
		.XSize = 64,
		.YSize = 64,
		.pPalette = imgPalette,
		.pPixel = RIGHT_MAP};

	System_printf("Display init.\n");
	System_flush();

	Display_Params params;
	Display_Params_init(&params);
	params.lineClearMode = DISPLAY_CLEAR_BOTH;

	System_printf("DISPLAY: Opening display.\n");
	System_flush();

	Display_Handle displayHandle = Display_open(Display_Type_LCD, &params);

	if (displayHandle == NULL)
	{
		System_abort("Display open failed!");
	}

	Task_sleep(1000 * (1000 / Clock_tickPeriod));

	System_printf("DISPLAY: Done initializing display.\n");
	System_flush();

	uint8_t select_mult = 7;

	while (1)
	{
		tContext *pContext = DisplayExt_getGrlibContext(displayHandle);
		//when switching from menu or help to game clear the screen.
		if (currState == GAME && prevState != GAME)
		{
			Display_clear(displayHandle);
		}
		prevState = currState;

		if (currState == MENU)
		{
			Display_print0(displayHandle, 0, 9, "SELECT");
			Display_print0(displayHandle, 1, 2, "MENU");
			Display_print0(displayHandle, 3, 2, "Start game");
			Display_print0(displayHandle, 4, 2, "Help");
			Display_print0(displayHandle, 11, 10, "DOWN");
			if (pContext)
			{
				GrLineDraw(pContext, 10, 18, 74, 18);
				GrLineDraw(pContext, 10, 32 + select_mult * select, 80, 32 + select_mult * select);
				GrFlush(pContext);
			}
		}
		if (currState == HELP)
		{
			Display_print0(displayHandle, 6, 2, "Tilt device");
			Display_print0(displayHandle, 7, 0, "to move character");
			Display_print0(displayHandle, 8, 2, "on screen");
		}
		if (currState == GAME)
		{
			switch (currMove)
			{
			case STILL:
				Display_print0(displayHandle, 5, 2, "Tilt device");
				break;

			case UP:
				Display_clear(displayHandle);
				GrImageDraw(pContext, &up_img, 12, 12);
				GrFlush(pContext);
				break;

			case DOWN:
				Display_clear(displayHandle);
				GrImageDraw(pContext, &down_img, 12, 12);
				GrFlush(pContext);
				break;

			case LEFT:
				Display_clear(displayHandle);
				GrImageDraw(pContext, &left_img, 12, 12);
				GrFlush(pContext);
				break;

			case RIGHT:
				Display_clear(displayHandle);
				GrImageDraw(pContext, &right_img, 12, 12);
				GrFlush(pContext);
				break;

			default:
				break;
			}
		}
		Task_sleep(1000000 / Clock_tickPeriod);
	}
}

/* Communication Task */

Void commTaskFxn(UArg arg0, UArg arg1)
{
	// Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if (result != true)
	{
		System_abort("Wireless receive mode failed");
	}
	while (1)
	{
		// If true, we have a message
		if (GetRXFlag() == true)
		{
			// Handle the received message..
		}
		// Absolutely NO Task_sleep in this task!!
	}
}

Int main(void)
{
	Task_Handle dataTask;
	Task_Params dataTaskParams;

	Task_Handle displayTask;
	Task_Params displayTaskParams;

	Board_initGeneral();
	Board_initI2C();

	//open MPU pin
	hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
	if (hMpuPin == NULL)
	{
		System_abort("Pin open failed!");
	}

	//open button pins
	buttonHandle = PIN_open(&buttonState, buttonConfig);
	if (!buttonHandle)
	{
		System_abort("Error initializing button pins\n");
	}

	//set button interrupt handler
	if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0)
	{
		System_abort("Error registering button callback function");
	}

	/*DISPLAY TASK PARAMS*/
	Task_Params_init(&displayTaskParams);
	displayTaskParams.stackSize = STACKSIZE;
	displayTaskParams.stack = &displayTaskStack;
	displayTaskParams.priority = 2;

	/*DATA TASK PARAMS*/
	Task_Params_init(&dataTaskParams);
	dataTaskParams.stackSize = STACKSIZE;
	dataTaskParams.stack = dataTaskStack;
	dataTaskParams.priority = 2;

	/*adding tasks to be executed*/
	displayTask = Task_create((Task_FuncPtr)displayFxn, &displayTaskParams, NULL);
	dataTask = Task_create((Task_FuncPtr)sensorFxn, &dataTaskParams, NULL);

	if (dataTask == NULL || displayTask == NULL)
	{
		System_abort("Task create failed!");
	}

	System_printf("Tasks created\n");
	System_flush();

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

	System_printf("BIOS started\n");
	System_flush();

	return (0);
}

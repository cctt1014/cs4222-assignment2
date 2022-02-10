#include <stdio.h>

#include "contiki.h"
#include "sys/rtimer.h"
#include "sys/etimer.h"

#include "board-peripherals.h"
#include "buzzer.h"

#include <stdint.h>
#include <math.h>


static struct rtimer timer_rtimer;

PROCESS(process_IDLE, "IDLE");
PROCESS(process_ACTIVE, "ACTIVE");
AUTOSTART_PROCESSES(&process_IDLE, &process_ACTIVE);


/* --- For process active --- */

#define ACC_THRESHOLD 0
#define GYRO_THRESHOLD 20000

static rtimer_clock_t timeout_IMU = RTIMER_SECOND / 20;  // 20Hz

static void
run_IMU(struct rtimer *timer, void *ptr)  {
	int ax, ay, az, gx, gy, gz;

	ax = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
	ay = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
	az = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);

	gx = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
	gy = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
	gz = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);

	int a = (int) (sqrt((double)((ax^2) + (ay^2) + (az^2))));

	if (a > ACC_THRESHOLD && (abs(gx) > GYRO_THRESHOLD || abs(gy) > GYRO_THRESHOLD|| abs(gz) > GYRO_THRESHOLD)) {
		printf("Significant motion detected!\n");
		process_post(&process_ACTIVE, PROCESS_EVENT_CONTINUE, NULL);
	} else {
		rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_IMU, 0, run_IMU, NULL);
	}

}

PROCESS_THREAD(process_IDLE, ev, data) {

	PROCESS_BEGIN();
	
	//init_mpu_reading
	mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL); 

	while(1) {
		printf("Mode: IDLE\n");
		rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_IMU, 0,  run_IMU, NULL);
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
	}

	PROCESS_END();
}


/* --- For process active --- */

#define LIGHT_THRESHOLD 300

static rtimer_clock_t timeout_light = RTIMER_SECOND / 4;  // 4Hz

static int prev_lux = CC26XX_SENSOR_READING_ERROR;
static bool toIDLE = false;

static void 
init_opt_reading(void) {
	SENSORS_ACTIVATE(opt_3001_sensor);
}

void 
run_light_sensor(struct rtimer *timer, void *ptr) {

	int curr_lux = opt_3001_sensor.value(0) / 100;
	init_opt_reading();

	printf("curr light: %d lux | prev light: %d lux\n", curr_lux, prev_lux);

	if(curr_lux != CC26XX_SENSOR_READING_ERROR && prev_lux != CC26XX_SENSOR_READING_ERROR && abs(curr_lux-prev_lux) > LIGHT_THRESHOLD) {
		printf("Significant change of light detected!\n");
		toIDLE = true;
	} else {
		prev_lux = curr_lux;
		rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_light, 0, run_light_sensor, NULL);
	}
	

}

PROCESS_THREAD(process_ACTIVE, ev, data) {
	static struct etimer timer_etimer;

	PROCESS_BEGIN();	

	buzzer_init();
	init_opt_reading();

	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
	printf("ACTIVE state start at the first time!\n");

	while (1) {
		rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_light, 0, run_light_sensor, NULL);
		
		buzzer_start(2093);
		printf("State: BUZZ!\n");
		etimer_set(&timer_etimer, CLOCK_SECOND*3); 
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

		if (toIDLE) {
			buzzer_stop();

			prev_lux = CC26XX_SENSOR_READING_ERROR;
			process_post(&process_IDLE, PROCESS_EVENT_CONTINUE, NULL);
			PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
			
			// Resume ACTIVE Mode
			printf("Mode: ACTIVE\n");
			toIDLE = false;
			continue;
		}

		buzzer_stop();
		printf("State: WAIT\n");
		etimer_set(&timer_etimer, CLOCK_SECOND*3); 
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

		if (toIDLE) {
			prev_lux = CC26XX_SENSOR_READING_ERROR;
			process_post(&process_IDLE, PROCESS_EVENT_CONTINUE, NULL);
			PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
			printf("Mode: ACTIVE\n");
			toIDLE = false;
		}

	}

	PROCESS_END();

}



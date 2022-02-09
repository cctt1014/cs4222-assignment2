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
AUTOSTART_PROCESSES(&process_IDLE);


/* --- For process active --- */

static rtimer_clock_t timeout_IMU = RTIMER_SECOND / 20;  // 20Hz

void
run_IMU(struct rtimer *timer, void *ptr)  {
	int ax, ay, az, gx, gy, gz;

	ax = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
	ay = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
	az = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);

	gx = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
	gy = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
	gz = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);

	int a = (int) (sqrt((double)((ax^2) + (ay^2) + (az^2))));

	if (a > 0 && (abs(gx) > 20000 || abs(gy) > 20000|| abs(gz) > 20000)) {
		printf("Transit to ACTIVE!\n");
		process_start(&process_ACTIVE, NULL);		
	} else {
		rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_IMU, 0, run_IMU, NULL);
	}

}

PROCESS_THREAD(process_IDLE, ev, data) {

	PROCESS_BEGIN();
	
	//init_mpu_reading
	mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL); 

	while(1) {
		rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_IMU, 0,  run_IMU, NULL);
		PROCESS_YIELD();
	}

	PROCESS_END();
}


/* --- For process active --- */

static rtimer_clock_t timeout_light = RTIMER_SECOND / 4;  // 4Hz

static int prev_lux;

static void 
init_opt_reading(void) {
	SENSORS_ACTIVATE(opt_3001_sensor);
	do {
		prev_lux = opt_3001_sensor.value(0);
	} while (prev_lux == CC26XX_SENSOR_READING_ERROR);
}

void 
run_light_sensor(struct rtimer *timer, void *ptr) {

	rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_light, 0, run_light_sensor, NULL);
	
	int curr_lux;
	do {
		curr_lux = opt_3001_sensor.value(0);
	} while (curr_lux == CC26XX_SENSOR_READING_ERROR);

	printf("Curr lux")

	if (abs(curr_lux-prev_lux) >= 300) {
		buzzer_stop(); 
	} else {
		prev_lux = curr_lux;
	}

}

PROCESS_THREAD(process_ACTIVE, ev, data) {
	
	PROCESS_BEGIN();
	buzzer_init();
	init_opt_reading();

	while(1) {
		rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_light, 0, run_light_sensor, NULL);
		
		printf("BACK TO IDLE\n");
		process_post_synch(&process_IDLE, PROCESS_EVENT_CONTINUE, NULL);
		break;
	}
	

	PROCESS_END();

}



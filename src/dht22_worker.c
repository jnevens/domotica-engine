/*
 * dht22_worker.c
 *
 *  Created on: Feb 15, 2020
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <eu/log.h>
#include <eu/list.h>

#include "device.h"
#include "utils_time.h"
#include "utils_sched.h"
#include "dht22_private.h"
#include "dht22.h"
#include "dht22_worker.h"

#define MMIO_SUCCESS 0
#define MMIO_ERROR_DEVMEM -1
#define MMIO_ERROR_MMAP -2
#define MMIO_ERROR_OFFSET -3
#define GPIO_BASE_OFFSET 0x200000
#define GPIO_LENGTH 4096

// Define errors and return values.
typedef enum {
	DHT_SUCCESS = 0,
	DHT_ERROR_SENSOR_NOT_RESPONDING,
	DHT_ERROR_TIMEOUT,
	DHT_ERROR_CHECKSUM,
	DHT_ERROR_ARGUMENT,
	DHT_ERROR_GPIO,
} dht22_stateus_e;


// This is the only processor specific magic value, the maximum amount of time to
// spin in a loop before bailing out and considering the read a timeout.  This should
// be a high value, but if you're running on a much faster platform than a Raspberry
// Pi or Beaglebone Black then it might need to be increased.
#define DHT_MAXCOUNT 3200000

// Number of bit pulses to expect from the DHT.  Note that this is 41 because
// the first pulse is a constant 50 microsecond pulse, with 40 pulses to represent
// the data afterwards.
#define DHT_PULSES 41

static volatile uint32_t* pi_2_mmio_gpio = NULL;
static pthread_t worker_thread = 0;
static eu_list_t *dht22_sensors = NULL;

static inline void pi_2_mmio_set_input(const int gpio_number)
{
	// Set GPIO register to 000 for specified GPIO number.
	*(pi_2_mmio_gpio + ((gpio_number) / 10)) &= ~(7 << (((gpio_number) % 10) * 3));
}

static inline void pi_2_mmio_set_output(const int gpio_number)
{
	// First set to 000 using input function.
	pi_2_mmio_set_input(gpio_number);
	// Next set bit 0 to 1 to set output.
	*(pi_2_mmio_gpio + ((gpio_number) / 10)) |= (1 << (((gpio_number) % 10) * 3));
}

static inline void pi_2_mmio_set_high(const int gpio_number)
{
	*(pi_2_mmio_gpio + 7) = 1 << gpio_number;
}

static inline void pi_2_mmio_set_low(const int gpio_number)
{
	*(pi_2_mmio_gpio + 10) = 1 << gpio_number;
}

static inline uint32_t pi_2_mmio_input(const int gpio_number)
{
	return *(pi_2_mmio_gpio + 13) & (1 << gpio_number);
}

static int pi_2_mmio_init(void)
{
	if (pi_2_mmio_gpio == NULL) {
		// Check for GPIO and peripheral addresses from device tree.
		// Adapted from code in the RPi.GPIO library at:
		//   http://sourceforge.net/p/raspberry-gpio-python/
		FILE *fp = fopen("/proc/device-tree/soc/ranges", "rb");
		if (fp == NULL) {
			return MMIO_ERROR_OFFSET;
		}
		fseek(fp, 4, SEEK_SET);
		unsigned char buf[4];
		if (fread(buf, 1, sizeof(buf), fp) != sizeof(buf)) {
			fclose(fp);
			return MMIO_ERROR_OFFSET;
		}
		uint32_t peri_base = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0;
		uint32_t gpio_base = peri_base + GPIO_BASE_OFFSET;
		fclose(fp);

		int fd = open("/dev/mem", O_RDWR | O_SYNC);
		if (fd == -1) {
			// Error opening /dev/mem.  Probably not running as root.
			return MMIO_ERROR_DEVMEM;
		}
		// Map GPIO memory to location in process space.
		pi_2_mmio_gpio = (uint32_t*) mmap(NULL, GPIO_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpio_base);
		close(fd);
		if (pi_2_mmio_gpio == MAP_FAILED) {
			// Don't save the result if the memory mapping failed.
			pi_2_mmio_gpio = NULL;
			return MMIO_ERROR_MMAP;
		}
	}
	return MMIO_SUCCESS;
}

static int pi_2_dht_read(int pin, float* humidity, float* temperature)
{
	// Validate humidity and temperature arguments and set them to zero.
	if (humidity == NULL || temperature == NULL) {
		return DHT_ERROR_ARGUMENT;
	}
	*temperature = 0.0f;
	*humidity = 0.0f;

	// Store the count that each DHT bit pulse is low and high.
	// Make sure array is initialized to start at zero.
	int pulseCounts[DHT_PULSES * 2] = { 0 };

	// Set pin to output.
	pi_2_mmio_set_output(pin);

	// Set pin high for ~500 milliseconds.
	pi_2_mmio_set_high(pin);
	sleep_milliseconds(500);

	// The next calls are timing critical and care should be taken
	// to ensure no unnecssary work is done below.
	sched_set_max_priority();

	// Set pin low for ~20 milliseconds.
	pi_2_mmio_set_low(pin);
	busy_wait_milliseconds(20);

	// pull high 20-40us
	pi_2_mmio_set_high(pin);
	busy_wait_microseconds(30);

	// Set pin at input.
	pi_2_mmio_set_input(pin);

	// Wait for DHT to pull pin low.
	uint32_t count = 0;
	while (pi_2_mmio_input(pin)) {
		if (++count >= DHT_MAXCOUNT) {
			// Timeout waiting for response.
			return DHT_ERROR_SENSOR_NOT_RESPONDING;
		}
	}

	// Record pulse widths for the expected result bits.
	for (int i = 0; i < DHT_PULSES * 2; i += 2) {
		// Count how long pin is low and store in pulseCounts[i]
		while (!pi_2_mmio_input(pin)) {
			if (++pulseCounts[i] >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				return DHT_ERROR_TIMEOUT;
			}
		}
		// Count how long pin is high and store in pulseCounts[i+1]
		while (pi_2_mmio_input(pin)) {
			if (++pulseCounts[i + 1] >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				return DHT_ERROR_TIMEOUT;
			}
		}
	}

	// Drop back to normal priority.
	sched_set_default_priority();

	// Compute the average low pulse width to use as a 50 microsecond reference threshold.
	// Ignore the first two readings because they are a constant 80 microsecond pulse.
	uint32_t threshold = 0;
	for (int i = 2; i < DHT_PULSES * 2; i += 2) {
		threshold += pulseCounts[i];
	}
	threshold /= DHT_PULSES - 1;

	// Interpret each high pulse as a 0 or 1 by comparing it to the 50us reference.
	// If the count is less than 50us it must be a ~28us 0 pulse, and if it's higher
	// then it must be a ~70us 1 pulse.
	uint8_t data[5] = { 0 };
	for (int i = 3; i < DHT_PULSES * 2; i += 2) {
		int index = (i - 3) / 16;
		data[index] <<= 1;
		if (pulseCounts[i] >= threshold) {
			// One bit for long pulse.
			data[index] |= 1;
		}
		// Else zero bit for short pulse.
	}

	// Useful debug info:
	//printf("Data: 0x%x 0x%x 0x%x 0x%x 0x%x\n", data[0], data[1], data[2], data[3], data[4]);

	// Verify checksum of received data.
	if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
		// Calculate humidity and temp for DHT22 sensor.
		*humidity = (data[0] * 256 + data[1]) / 10.0f;
		*temperature = ((data[2] & 0x7F) * 256 + data[3]) / 10.0f;
		if (data[2] & 0x80) {
			*temperature *= -1.0f;
		}
		return DHT_SUCCESS;
	} else {
		return DHT_ERROR_CHECKSUM;
	}
}


static bool dht22_worker_measure_device_do(device_t *device, float *temperature, float *humidity)
{
	dht22_t *dht22 = device_get_userdata(device);
	float temp = 0.0, hum = 0.0;
	int count = 0;

	do {
		int rv = pi_2_dht_read(dht22->gpio, &hum, &temp);
		if (rv == DHT_ERROR_SENSOR_NOT_RESPONDING) {
			eu_log_debug("DHT22: %s: Sensor not responding!", device_get_name(device));
			count++;
		} else if (rv == DHT_ERROR_TIMEOUT) {
			eu_log_debug("DHT22: %s: Sensor timeout!", device_get_name(device));
		} else if (rv == DHT_ERROR_CHECKSUM) {
			eu_log_debug("DHT22: %s: Invalid checksum!", device_get_name(device));
		} else if (rv == DHT_SUCCESS) {
			*temperature = temp;
			*humidity = hum;
			return true;
		}
		sleep(1);
	} while(count < 10);

	eu_log_err("DHT22: %s: TIMEOUT!", device_get_name(device));

	return false;
}

static void dht22_worker_measure_device(device_t *device)
{
	dht22_t *dht22 = device_get_userdata(device);
	int count = 0;

	do {
		float temp1 = 0.0, hum1 = 0.0;
		float temp2 = 0.0, hum2 = 0.0;

		if (!dht22_worker_measure_device_do(device, &temp1, &hum1))
			break;
		if (!dht22_worker_measure_device_do(device, &temp2, &hum2))
			break;

		if ((fabs(temp1 - temp2) < 0.15) && (fabs(hum1 - hum2) < .15)) {
			eu_log_info("DHT22: %s: temp:%f, hum:%f", device_get_name(device), temp2, hum2);
			dht22->temperature = temp2;
			dht22->humidity = hum2;
			return;
		}
	} while (count < 3);

	eu_log_err("DHT22: %s: TIMEOUT!", device_get_name(device));
}

static void *perform_work(void *arguments)
{
	// Initialize GPIO library.
	if (pi_2_mmio_init() < 0) {
		return NULL;
	}

	for (;;) {
		eu_list_for_each_declare(node, dht22_sensors) {
			device_t *device = eu_list_node_data(node);

			dht22_worker_measure_device(device);
			sleep(1);
		}
		sleep(10);
	}

	return NULL;
}

void dht22_worker_init(eu_list_t *dht22_devices)
{
	dht22_sensors = dht22_devices;
	int result_code = pthread_create(&worker_thread, NULL, perform_work, NULL);
	if (result_code) {
		eu_log_err("Failed to create DHT22 worker thread! %m");
	}
}

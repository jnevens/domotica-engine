/*
 * dht22.c
 *
 *  Created on: Feb 8, 2017
 *      Author: jnevens
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <eu/log.h>
#include <eu/event.h>
#include <eu/timer.h>

#include "utils_sched.h"
#include "utils_time.h"
#include "device.h"
#include "event.h"
#include "action.h"
#include "dht22.h"

typedef struct {
	int gpio;
	eu_event_timer_t *timer;
	time_t period;
	float temperature;
	float humidity;
} dht22_t;

// Define errors and return values.
#define DHT_ERROR_TIMEOUT -1
#define DHT_ERROR_CHECKSUM -2
#define DHT_ERROR_ARGUMENT -3
#define DHT_ERROR_GPIO -4
#define DHT_SUCCESS 0
#define DHT11 11
#define DHT22 22
#define AM2302 22
#define MMIO_SUCCESS 0
#define MMIO_ERROR_DEVMEM -1
#define MMIO_ERROR_MMAP -2
#define MMIO_ERROR_OFFSET -3
#define GPIO_BASE_OFFSET 0x200000
#define GPIO_LENGTH 4096

// This is the only processor specific magic value, the maximum amount of time to
// spin in a loop before bailing out and considering the read a timeout.  This should
// be a high value, but if you're running on a much faster platform than a Raspberry
// Pi or Beaglebone Black then it might need to be increased.
#define DHT_MAXCOUNT 32000

// Number of bit pulses to expect from the DHT.  Note that this is 41 because
// the first pulse is a constant 50 microsecond pulse, with 40 pulses to represent
// the data afterwards.
#define DHT_PULSES 41

volatile uint32_t* pi_2_mmio_gpio = NULL;

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

static bool dht22_read_part2(void *arg)
{
	device_t *device = arg;
	dht22_t *dht22 = device_get_userdata(device);
	int pin = dht22->gpio;

	// Store the count that each DHT bit pulse is low and high.
	// Make sure array is initialized to start at zero.
	int pulseCounts[DHT_PULSES * 2] = { 0 };

	// The next calls are timing critical and care should be taken
	// to ensure no unnecssary work is done below.
	sched_set_max_priority();

	// Set pin low for ~20 milliseconds.
	pi_2_mmio_set_low(pin);
	busy_wait_milliseconds(20);

	// Set pin at input.
	pi_2_mmio_set_input(pin);
	// Need a very short delay before reading pins or else value is sometimes still low.
	for (volatile int i = 0; i < 50; ++i) {
	}

	// Wait for DHT to pull pin low.
	uint32_t count = 0;
	while (pi_2_mmio_input(pin)) {
		if (++count >= DHT_MAXCOUNT) {
			// Timeout waiting for response.
			sched_set_default_priority();
			eu_log_err("DHT22 timeout!");
			return false;
		}
	}

	// Record pulse widths for the expected result bits.
	for (int i = 0; i < DHT_PULSES * 2; i += 2) {
		// Count how long pin is low and store in pulseCounts[i]
		while (!pi_2_mmio_input(pin)) {
			if (++pulseCounts[i] >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				sched_set_default_priority();
				eu_log_err("DHT22 timeout!");
				return false;
			}
		}
		// Count how long pin is high and store in pulseCounts[i+1]
		while (pi_2_mmio_input(pin)) {
			if (++pulseCounts[i + 1] >= DHT_MAXCOUNT) {
				// Timeout waiting for response.
				sched_set_default_priority();
				eu_log_err("DHT22 timeout!");
				return false;
			}
		}
	}

	// Done with timing critical code, now interpret the results.

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
	eu_log_debug("DHT22 Data: 0x%x 0x%x 0x%x 0x%x 0x%x", data[0], data[1], data[2], data[3], data[4]);

	// Verify checksum of received data.
	if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
		// Calculate humidity and temp for DHT22 sensor.
		float humidity = (data[0] * 256 + data[1]) / 10.0f;
		float temperature = ((data[2] & 0x7F) * 256 + data[3]) / 10.0f;
		if (data[2] & 0x80) {
			//temperature *= -1.0f;
			temperature = -1.0f;
		}

		eu_log_info("DHT22 %s: Temp: %.2f Hum: %.2f", device_get_name(device), temperature, humidity);

		dht22->temperature = temperature;
		dht22->humidity = humidity;

		device_trigger_event(device, EVENT_TEMPERATURE);
		device_trigger_event(device, EVENT_HUMIDITY);
	} else {
		eu_log_err("DHT22 checksum error!");
	}

	return false;
}

static int dht22_read(device_t *device)
{
	dht22_t *dht22 = device_get_userdata(device);

	// Initialize GPIO library.
	if (pi_2_mmio_init() < 0) {
		return DHT_ERROR_GPIO;
	}

	// Set pin to output.
	pi_2_mmio_set_output(dht22->gpio);

	// Set pin high for ~500 milliseconds.
	pi_2_mmio_set_high(dht22->gpio);

	eu_event_timer_create(500, dht22_read_part2, device);
}

static void dht22_read_schedule(device_t *device);

static bool dht22_read_schedule_callback(void *arg)
{
	device_t *device = (device_t *)arg;

	dht22_read(device);
	dht22_read_schedule(device);

	return false;
}

static void dht22_read_schedule(device_t *device)
{
	dht22_t *dht22 = device_get_userdata(device);
	time_t ct = time(NULL);
	time_t delay = dht22->period - (ct % dht22->period);

	eu_log_debug("schedule DHT22 read in: %ds", delay);

	eu_event_timer_create(delay * 1000, dht22_read_schedule_callback, device);
}

static bool dht22_parser(device_t *device, char *options[])
{
	bool rv = false;
	int gpio = atoi(options[1]);
	eu_log_debug("gpio: %d", gpio);

	dht22_t *dht22 = calloc(1, sizeof(dht22_t));
	dht22->gpio = gpio;
	dht22->period = 900;
	device_set_userdata(device, dht22);

	if (gpio >= 0 && gpio <= 1024)
		rv = true;

	dht22_read_schedule(device);

	return rv;
}

static bool dht22_device_state(device_t *device, eu_variant_map_t *state)
{
	dht22_t *dht22 = device_get_userdata(device);

	eu_variant_map_set_float(state, "temperature", dht22->temperature);
	eu_variant_map_set_float(state, "humidity", dht22->humidity);

	return true;
}

static device_type_info_t dht22_info = {
	.name = "DHT22",
	.events = EVENT_TEMPERATURE | EVENT_HUMIDITY,
	.actions = 0,
	.conditions = 0,
	.check_cb = NULL,
	.parse_cb = dht22_parser,
	.exec_cb = NULL,
	.state_cb = dht22_device_state,
};

bool dht22_technology_init()
{
	device_type_register(&dht22_info);

	eu_log_info("Succesfully initialized: Sunriset!");
	return true;
}

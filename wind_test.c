#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define ADS1015_ADDR 0x48 // I2C address of the ADS1015
#define WIND_CHANNEL 0 // Analog input channel for wind sensor
#define TEMP_CHANNEL 1 // Analog input channel for temperature sensor

int i2c_fd;

int i2c_init(const char *device) {
    i2c_fd = open(device, O_RDWR);
    if (i2c_fd < 0) {
        perror("Failed to open I2C device");
        return -1;
    }
    return 0;
}

int i2c_read_adc(int channel) {
    unsigned char buf[2];
    int result;

    // Configure the ADC for single-ended input, 1.024V range
    unsigned char config[3] = {
        0x01, // Config register (address)
        0xC1, // MSB: 0xC1 (single-ended, AIN0/GND), LSB: 0x83 (1.024V range)
        0x83  // Config register (value)
    };

    // Set config register
    if (write(i2c_fd, config, 3) != 3) {
        perror("Failed to write to the I2C bus");
        return -1;
    }

    // Select the appropriate channel
    buf[0] = 0x00; // Pointer register (address)
    buf[1] = (channel << 4) | 0x03; // MSB: AINx (where x is the channel), LSB: 0x03 (start single-conversion)
    if (write(i2c_fd, buf, 2) != 2) {
        perror("Failed to write to the I2C bus");
        return -1;
    }

    usleep(1000); // Wait for conversion to complete

    // Read the conversion result
    buf[0] = 0x00; // Pointer register (address)
    if (write(i2c_fd, &buf[0], 1) != 1) {
        perror("Failed to write to the I2C bus");
        return -1;
    }
    if (read(i2c_fd, &buf[0], 2) != 2) {
        perror("Failed to read from the I2C bus");
        return -1;
    }

    // Convert the result to an integer value
    result = (buf[0] << 8) | buf[1];

    return result;
}

int main() {
    if (i2c_init("/dev/i2c-2") != 0) // Use the appropriate I2C device file
        return -1;

    while (1) {
        // Read wind
        int wind_raw = i2c_read_adc(WIND_CHANNEL);
        float wind_mph = pow((((float)wind_raw - 2048.0) / 2048.0), 3.36814); // Adjust scale as needed
        printf("%.2f MPH\t", wind_mph);

        // Read temperature
        int temp_raw = i2c_read_adc(TEMP_CHANNEL);
        float temp_c = (temp_raw / 2047.0) * 125.0; // Adjust scale as needed
        printf("%.2f C\n", temp_c);

        sleep(1); // Adjust delay as needed
    }

    close(i2c_fd);
    return 0;
}

/*
	The ICM20948 includes two chips: one contains the accelerometer and gyroscope,
	the other contains the magnetometer (AK09916 but hereby referred to as MAG). 
	The ICM20948 is an I2C slave to the ESP32 but a master to the MAG. 
	THE ICM20948 includes registers to talk to MAG via I2C, but the registers
	are set and read by the ESP32.

	(MASTER)	<-->	(SLAVE)

	  ESP32            ICM 20948

	  				    (MASTER)   <-->   (SLAVE)

										  AK09916
	
	The ICM20948 acts as sort of a middle man between the ESP32 and the MAG.
	I'm pretty sure I could connect the auxillary bus to directly connect
	to MAG directly via it's own I2C address but I'd like the program to 
	only use the ICM20948 registers.

*/

/* 
	This driver uses the old I2C API ESP-IDF v5.1.5
	This is due to the new driver not yet supporting writing custom I2C packets.
	I tried to implement new API but ran into issues due to the i2c_master_transmit/receive
	only supporting a single byte of data being sent/recieved before its stop command. 
	This is an issue because the ICM20948 expects its address, then register, then data, 
	which the new API doesn't currently support.
*/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_log.h"
#include "driver/i2c.h"

#include "icm20948.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)                                                                                           \
	(byte & 0x80 ? '1' : '0'), (byte & 0x40 ? '1' : '0'), (byte & 0x20 ? '1' : '0'), (byte & 0x10 ? '1' : '0'),        \
	    (byte & 0x08 ? '1' : '0'), (byte & 0x04 ? '1' : '0'), (byte & 0x02 ? '1' : '0'), (byte & 0x01 ? '1' : '0')

/*  
	ICM20948 registers given via datasheet
	ICM20948 includes Banks 0 through 3 to reuse register addresses
	Not all used but all included for completionist sake
*/

// Bank 0
#define ICM20948_WHO_AM_I       0x00
#define ICM20948_USER_CTRL      0x03
#define ICM20948_LP_CONFIG      0x05

#define ICM20948_PWR_MGMT_1     0x06
#define ICM20948_PWR_MGMT_2     0x07

#define ICM20948_INT_PIN_CFG    0x0F
#define ICM20948_INT_ENABLE     0x10
#define ICM20948_INT_ENABLE_1   0x11
#define ICM20948_INT_ENABLE_2   0x12
#define ICM20948_INT_ENABLE_3   0x13

#define ICM20948_I2C_MST_STATUS 0x17
#define ICM20948_INT_STATUS     0x19
#define ICM20948_INT_STATUS_1   0x1A
#define ICM20948_INT_STATUS_2   0x1B
#define ICM20948_INT_STATUS_3   0x1C

#define ICM20948_DELAY_TIMEH    0x28
#define ICM20948_DELAY_TIMEL    0x29

#define ICM20948_ACCEL_XOUT_H   0x2D
#define ICM20948_ACCEL_XOUT_L   0x2E
#define ICM20948_ACCEL_YOUT_H   0x2F
#define ICM20948_ACCEL_YOUT_L   0x30
#define ICM20948_ACCEL_ZOUT_H   0x31
#define ICM20948_ACCEL_ZOUT_L   0x32

#define ICM20948_GYRO_XOUT_H    0x33
#define ICM20948_GYRO_XOUT_L    0x34
#define ICM20948_GYRO_YOUT_H    0x35
#define ICM20948_GYRO_YOUT_L    0x36
#define ICM20948_GYRO_ZOUT_H    0x37
#define ICM20948_GYRO_ZOUT_L    0x38

#define ICM20948_TEMP_OUT_H     0x39
#define ICM20948_TEMP_OUT_L     0x3A

/*  
	These registers store the data read from the MAG
	Goes to ...DATA_23 at address 0x52, not including all
*/
#define ICM20948_EXT_SLV_SENS_DATA_00	0x3B
#define ICM20948_EXT_SLV_SENS_DATA_01   0x3C
#define ICM20948_EXT_SLV_SENS_DATA_02   0x3D
#define ICM20948_EXT_SLV_SENS_DATA_03   0x3E
#define ICM20948_EXT_SLV_SENS_DATA_04   0x3F
#define ICM20948_EXT_SLV_SENS_DATA_05   0x40
#define ICM20948_EXT_SLV_SENS_DATA_06   0x41
#define ICM20948_EXT_SLV_SENS_DATA_07   0x42
#define ICM20948_EXT_SLV_SENS_DATA_08   0x43

#define ICM20948_FIFO_EN_1      		0x66
#define ICM20948_FIFO_EN_2      		0x67
#define ICM20948_FIFO_RST               0x68
#define ICM20948_FIFO_MODE              0x69
#define ICM20948_FIFO_COUNTH            0x70
#define ICM20948_FIFO_COUNTL    		0x71
#define ICM20948_FIFO_R_W       		0x72
#define ICM20948_DATA_RDY_STATUS        0x74
#define ICM20948_FIFO_CFG       		0x76

/* Selects register bank, same address on all banks */
#define ICM20948_REG_BANK_SEL   		0x7F	

// Bank 1 
#define ICM20948_SELF_TEST_X_GYRO       0x02
#define ICM20948_SELF_TEST_Y_GYRO       0x03
#define ICM20948_SELF_TEST_Z_GYRO       0x04
#define ICM20948_SELF_TEST_X_ACCEL      0x0E
#define ICM20948_SELF_TEST_Y_ACCEL      0x0F
#define ICM20948_SELF_TEST_Z_ACCEL      0x10

#define ICM20948_XA_OFFS_H     			0x14
#define ICM20948_XA_OFFS_L     			0x14
#define ICM20948_YA_OFFS_H       		0x17
#define ICM20948_YA_OFFS_L     			0x18
#define ICM20948_ZA_OFFS_H     			0x1A
#define ICM20948_ZA_OFFS_L     			0x1B

#define ICM20948_TIMEBASE_CORRECTION_PLL 0x01

// Bank 2
#define ICM20948_GYRO_SMPLRT_DIV   		0x00
#define ICM20948_GYRO_CONFIG_1   		0x01
#define ICM20948_GYRO_CONFIG_2   		0x02

#define ICM20948_XG_OFFS_USRH   		0x03
#define ICM20948_XG_OFFS_USRL   		0x04
#define ICM20948_YG_OFFS_USRH   		0x05
#define ICM20948_YG_OFFS_USRL   		0x06
#define ICM20948_ZG_OFFS_USRH   		0x07
#define ICM20948_ZG_OFFS_USRL   		0x08

#define ICM20948_ODR_ALIGN_EN   		0x09

#define ICM20948_ACCEL_SMPLRT_DIV_1   	0x10
#define ICM20948_ACCEL_SMPLRT_DIV_2  	0x11
#define ICM20948_ACCEL_INTEL_CTRL   	0x12
#define ICM20948_ACCEL_WOM_THR      	0x13
#define ICM20948_ACCEL_CONFIG        	0x14
#define ICM20948_ACCEL_CONFIG_2        	0x15

#define ICM20948_FSYNC_CONFIG        	0x52
#define ICM20948_TEMP_CONFIG        	0x53
#define ICM20948_MOD_CTRL_USR        	0x54

// Bank 3
#define ICM20948_I2C_MST_ODR_CONFIG     0x00
#define ICM20948_I2C_MST_CTRL           0x01
#define ICM20948_I2C_MST_DELAY_CTRL     0x02

/*
	These are the registers that set the I2C data
	to communicate with MAG. There are 3 other groups
	SLV1 to SLV4 which exist on registers 0x07 to 0x17
	but are not used so will not be listed
*/
#define ICM20948_I2C_SLV0_ADDR        	0x03
#define ICM20948_I2C_SLV0_REG        	0x04
#define ICM20948_I2C_SLV0_CTRL			0x05
#define ICM20948_I2C_SLV0_D0        	0x06

/*  
	Magnetometer registers (0x0C I2C address)
	From the AK09916 data sheet. These are used with registers
	directly above to write and read from the magnetometer
*/
#define M_REG_WIA2             0x01
#define M_REG_ST1              0x10
#define M_REG_HXL              0x11
#define M_REG_HXH              0x12
#define M_REG_HYL              0x13
#define M_REG_HYH              0x14
#define M_REG_HZL              0x15
#define M_REG_HZH              0x16
#define M_REG_ST2              0x18
#define M_REG_CNTL2            0x31
#define M_REG_CNTL3            0x32
/* DO NOT use TS1 or TS2 EVER !!! */ 
#define M_REG_TS1              0x33
#define M_REG_TS2              0x34

typedef struct {
	i2c_port_t bus;
	gpio_num_t int_pin;
	uint16_t dev_addr;
	uint32_t counter;
	float dt; /*!< delay time between two measurements, dt should be small (ms level) */
	struct timeval *timer;
} icm20948_dev_t;

static esp_err_t
icm20948_write(icm20948_handle_t sensor, const uint8_t reg_start_addr, const uint8_t *const data_buf, const uint8_t data_len)
{
	icm20948_dev_t *sens = (icm20948_dev_t *)sensor;
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	ret = i2c_master_start(cmd);
	assert(ESP_OK == ret);
	ret = i2c_master_write_byte(cmd, sens->dev_addr | I2C_MASTER_WRITE, true);
	assert(ESP_OK == ret);
	ret = i2c_master_write_byte(cmd, reg_start_addr, true);
	assert(ESP_OK == ret);
	ret = i2c_master_write(cmd, data_buf, data_len, true);
	assert(ESP_OK == ret);
	ret = i2c_master_stop(cmd);
	assert(ESP_OK == ret);
	ret = i2c_master_cmd_begin(sens->bus, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	return ret;
}

esp_err_t
icm20948_write_MAG(icm20948_handle_t sensor, const uint8_t reg_start_addr, const uint8_t *const data_buf)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 3);
	if (ret != ESP_OK)
		return ret;

	tmp = ICM20948_I2C_MAG_ADDRESS;
	ret = icm20948_write(sensor, ICM20948_I2C_SLV0_ADDR, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_write(sensor, ICM20948_I2C_SLV0_REG, &reg_start_addr, 1);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_write(sensor, ICM20948_I2C_SLV0_D0, data_buf, 1);
	if (ret != ESP_OK)
		return ret;
	
	tmp = 0x80|0x01;
	ret = icm20948_write(sensor, ICM20948_I2C_SLV0_CTRL, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	return ret;
}

static esp_err_t
icm20948_read(icm20948_handle_t sensor, const uint8_t reg_start_addr, uint8_t *const data_buf, const uint8_t data_len)
{
	icm20948_dev_t *sens = (icm20948_dev_t *)sensor;
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	ret = i2c_master_start(cmd);
	assert(ESP_OK == ret);
	ret = i2c_master_write_byte(cmd, sens->dev_addr | I2C_MASTER_WRITE, true);
	assert(ESP_OK == ret);
	ret = i2c_master_write_byte(cmd, reg_start_addr, true);
	assert(ESP_OK == ret);
	ret = i2c_master_start(cmd);
	assert(ESP_OK == ret);
	ret = i2c_master_write_byte(cmd, sens->dev_addr | I2C_MASTER_READ, true);
	assert(ESP_OK == ret);
	ret = i2c_master_read(cmd, data_buf, data_len, I2C_MASTER_LAST_NACK);
	assert(ESP_OK == ret);
	ret = i2c_master_stop(cmd);
	assert(ESP_OK == ret);
	ret = i2c_master_cmd_begin(sens->bus, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	return ret;
}

esp_err_t
icm20948_read_MAG(icm20948_handle_t sensor, const uint8_t onset_reg, const uint8_t data_len)
{
	// icm20948_handle_t sensor, const uint8_t reg_start_addr, const uint8_t *const data_buf, const uint8_t data_len
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 3);
	if (ret != ESP_OK)
		return ret;

	tmp = 0x80 | ICM20948_I2C_MAG_ADDRESS;
	ret = icm20948_write(sensor, ICM20948_I2C_SLV0_ADDR, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_write(sensor, ICM20948_I2C_SLV0_REG, &onset_reg, 1);
	if (ret != ESP_OK)
		return ret;

	tmp = 0x80|data_len;
	ret = icm20948_write(sensor, ICM20948_I2C_SLV0_CTRL, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	return ret;
}

icm20948_handle_t
icm20948_create(i2c_port_t port, const uint16_t dev_addr)
{
	icm20948_dev_t *sensor = (icm20948_dev_t *)calloc(1, sizeof(icm20948_dev_t));
	sensor->bus = port;
	sensor->dev_addr = dev_addr << 1;
	sensor->counter = 0;
	sensor->dt = 0;
	sensor->timer = (struct timeval *)calloc(1, sizeof(struct timeval));
	return (icm20948_handle_t)sensor;
}

void
icm20948_delete(icm20948_handle_t sensor)
{
	icm20948_dev_t *sens = (icm20948_dev_t *)sensor;
	free(sens);
}

esp_err_t
icm20948_get_deviceid(icm20948_handle_t sensor, uint8_t *const deviceid)
{
		return icm20948_read(sensor, ICM20948_WHO_AM_I, deviceid, 1);
}

esp_err_t
icm20948_wake_up(icm20948_handle_t sensor)
{
	esp_err_t ret;
	uint8_t tmp;
	ret = icm20948_read(sensor, ICM20948_PWR_MGMT_1, &tmp, 1);
	if (ESP_OK != ret) {
		return ret;
	}
	tmp &= (~BIT6);
	ret = icm20948_write(sensor, ICM20948_PWR_MGMT_1, &tmp, 1);
	return ret;
}

esp_err_t
icm20948_sleep(icm20948_handle_t sensor)
{
	esp_err_t ret;
	uint8_t tmp;
	ret = icm20948_read(sensor, ICM20948_PWR_MGMT_1, &tmp, 1);
	if (ESP_OK != ret) {
		return ret;
	}
	tmp |= BIT6;
	ret = icm20948_write(sensor, ICM20948_PWR_MGMT_1, &tmp, 1);
	return ret;
}

esp_err_t
icm20948_reset(icm20948_handle_t sensor)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_read(sensor, ICM20948_PWR_MGMT_1, &tmp, 1);
	if (ret != ESP_OK)
		return ret;
	tmp |= 0x80;
	ret = icm20948_write(sensor, ICM20948_PWR_MGMT_1, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	return ret;
}

esp_err_t
icm20948_set_bank(icm20948_handle_t sensor, uint8_t bank)
{
	esp_err_t ret;
	if (bank > 3)
		return ESP_FAIL;
	bank = (bank << 4) & 0x30;
	ret = icm20948_write(sensor, ICM20948_REG_BANK_SEL, &bank, 1);
	return ret;
}

esp_err_t
icm20948_set_gyro_fs(icm20948_handle_t sensor, icm20948_gyro_fs_t gyro_fs)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 2);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_read(sensor, ICM20948_GYRO_CONFIG_1, &tmp, 1);

#if CONFIG_LOG_DEFAULT_LEVEL == 4
	printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

	if (ret != ESP_OK)
		return ret;
	tmp &= 0x09;
	tmp |= (gyro_fs << 1);

#if CONFIG_LOG_DEFAULT_LEVEL == 4
	printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

	ret = icm20948_write(sensor, ICM20948_GYRO_CONFIG_1, &tmp, 1);
	return ret;
}

esp_err_t
icm20948_get_gyro_fs(icm20948_handle_t sensor, icm20948_gyro_fs_t *gyro_fs)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 2);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_read(sensor, ICM20948_GYRO_CONFIG_1, &tmp, 1);

#if CONFIG_LOG_DEFAULT_LEVEL == 4
	printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

	tmp &= 0x06;
	tmp >>= 1;
	*gyro_fs = tmp;
	return ret;
}

esp_err_t
icm20948_get_gyro_sensitivity(icm20948_handle_t sensor, float *const gyro_sensitivity)
{
	esp_err_t ret;
	icm20948_gyro_fs_t gyro_fs;
	ret = icm20948_get_gyro_fs(sensor, &gyro_fs);
	if (ret != ESP_OK)
		return ret;

	switch (gyro_fs) {
	case GYRO_FS_250DPS:
		*gyro_sensitivity = 131;
		break;

	case GYRO_FS_500DPS:
		*gyro_sensitivity = 65.5;
		break;

	case GYRO_FS_1000DPS:
		*gyro_sensitivity = 32.8;
		break;

	case GYRO_FS_2000DPS:
		*gyro_sensitivity = 16.4;
		break;

	default:
		break;
	}
	return ret;
}

esp_err_t
icm20948_get_raw_gyro(icm20948_handle_t sensor, icm20948_raw_sensor_data_t *const raw_gyro_value)
{
	uint8_t data_rd[6];
	esp_err_t ret = icm20948_read(sensor, ICM20948_GYRO_XOUT_H, data_rd, sizeof(data_rd));

	raw_gyro_value->raw_gyro_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
	raw_gyro_value->raw_gyro_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
	raw_gyro_value->raw_gyro_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));

	return ret;
}

esp_err_t
icm20948_get_gyro(icm20948_handle_t sensor, icm20948_sensor_data_t *const gyro_value)
{
	esp_err_t ret;
	float gyro_sensitivity;
	icm20948_raw_sensor_data_t raw_gyro;

	ret = icm20948_get_gyro_sensitivity(sensor, &gyro_sensitivity);
	if (ret != ESP_OK) {
		return ret;
	}

	ret = icm20948_set_bank(sensor, 0);
	if (ret != ESP_OK) {
		return ret;
	}

	ret = icm20948_get_raw_gyro(sensor, &raw_gyro);
	if (ret != ESP_OK) {
		return ret;
	}

	gyro_value->gyro_x = raw_gyro.raw_gyro_x / gyro_sensitivity;
	gyro_value->gyro_y = raw_gyro.raw_gyro_y / gyro_sensitivity;
	gyro_value->gyro_z = raw_gyro.raw_gyro_z / gyro_sensitivity;
	return ESP_OK;
}

esp_err_t
icm20948_set_acce_fs(icm20948_handle_t sensor, icm20948_acce_fs_t acce_fs)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 2);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_read(sensor, ICM20948_ACCEL_CONFIG, &tmp, 1);

#if CONFIG_LOG_DEFAULT_LEVEL == 4
	printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

	if (ret != ESP_OK)
		return ret;
	tmp &= 0x09;
	tmp |= (acce_fs << 1);

#if CONFIG_LOG_DEFAULT_LEVEL == 4
	printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

	ret = icm20948_write(sensor, ICM20948_ACCEL_CONFIG, &tmp, 1);
	return ret;
}

esp_err_t
icm20948_get_acce_fs(icm20948_handle_t sensor, icm20948_acce_fs_t *acce_fs)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 2);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_read(sensor, ICM20948_ACCEL_CONFIG, &tmp, 1);

#if CONFIG_LOG_DEFAULT_LEVEL == 4
	printf(BYTE_TO_BINARY_PATTERN "\n", BYTE_TO_BINARY(tmp));
#endif

	tmp &= 0x06;
	tmp >>= 1;
	*acce_fs = tmp;
	return ret;
}

esp_err_t
icm20948_get_raw_acce(icm20948_handle_t sensor, icm20948_raw_sensor_data_t *const raw_acce_value)
{
	uint8_t data_rd[6];
	esp_err_t ret = icm20948_read(sensor, ICM20948_ACCEL_XOUT_H, data_rd, sizeof(data_rd));

	raw_acce_value->raw_acce_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
	raw_acce_value->raw_acce_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
	raw_acce_value->raw_acce_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));
	return ret;
}

esp_err_t
icm20948_get_acce_sensitivity(icm20948_handle_t sensor, float *const acce_sensitivity)
{
	esp_err_t ret;
	icm20948_acce_fs_t acce_fs;
	ret = icm20948_get_acce_fs(sensor, &acce_fs);
	switch (acce_fs) {
	case ACCE_FS_2G:
		*acce_sensitivity = 16384;
		break;

	case ACCE_FS_4G:
		*acce_sensitivity = 8192;
		break;

	case ACCE_FS_8G:
		*acce_sensitivity = 4096;
		break;

	case ACCE_FS_16G:
		*acce_sensitivity = 2048;
		break;

	default:
		break;
	}
	return ret;
}

esp_err_t
icm20948_get_acce(icm20948_handle_t sensor, icm20948_sensor_data_t *const acce_value)
{
	esp_err_t ret;
	float acce_sensitivity;
	icm20948_raw_sensor_data_t raw_acce;

	ret = icm20948_get_acce_sensitivity(sensor, &acce_sensitivity);
	if (ret != ESP_OK) {
		return ret;
	}

	ret = icm20948_set_bank(sensor, 0);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_get_raw_acce(sensor, &raw_acce);
	if (ret != ESP_OK) {
		return ret;
	}

	acce_value->acce_x = raw_acce.raw_acce_x / acce_sensitivity;
	acce_value->acce_y = raw_acce.raw_acce_y / acce_sensitivity;
	acce_value->acce_z = raw_acce.raw_acce_z / acce_sensitivity;
	return ESP_OK;
}

esp_err_t
icm20948_set_acce_dlpf(icm20948_handle_t sensor, icm20948_dlpf_t dlpf_acce)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = icm20948_read(sensor, ICM20948_ACCEL_CONFIG, &tmp, 1);
	if (ret != ESP_OK)
		return ESP_FAIL;

	tmp &= 0xC7;
	tmp |= dlpf_acce << 3;

	ret = icm20948_write(sensor, ICM20948_ACCEL_CONFIG, &tmp, 1);
	if (ret != ESP_OK)
		return ESP_FAIL;

	return ret;
}

esp_err_t
icm20948_set_gyro_dlpf(icm20948_handle_t sensor, icm20948_dlpf_t dlpf_gyro)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = icm20948_read(sensor, ICM20948_GYRO_CONFIG_1, &tmp, 1);
	if (ret != ESP_OK)
		return ESP_FAIL;

	tmp &= 0xC7;
	tmp |= dlpf_gyro << 3;

	ret = icm20948_write(sensor, ICM20948_GYRO_CONFIG_1, &tmp, 1);
	if (ret != ESP_OK)
		return ESP_FAIL;

	return ret;
}

esp_err_t
icm20948_enable_dlpf(icm20948_handle_t sensor, bool enable)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = icm20948_read(sensor, ICM20948_ACCEL_CONFIG, &tmp, 1);
	if (ret != ESP_OK)
		return ESP_FAIL;

	if (enable)
		tmp |= 0x01;
	else
		tmp &= 0xFE;

	ret = icm20948_write(sensor, ICM20948_ACCEL_CONFIG, &tmp, 1);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = icm20948_read(sensor, ICM20948_GYRO_CONFIG_1, &tmp, 1);
	if (ret != ESP_OK)
		return ESP_FAIL;

	if (enable)
		tmp |= 0x01;
	else
		tmp &= 0xFE;

	ret = icm20948_write(sensor, ICM20948_GYRO_CONFIG_1, &tmp, 1);
	if (ret != ESP_OK)
		return ESP_FAIL;

	return ret;
}

esp_err_t
icm20948_mag_init(icm20948_handle_t sensor)
{
	esp_err_t ret;
	uint8_t tmp;

	ret = icm20948_set_bank(sensor, 0);
	if (ret != ESP_OK)
		return ret;

	// Reset I2C Master
	ret = icm20948_read(sensor, ICM20948_USER_CTRL, &tmp, 1);
	if (ret != ESP_OK)
		return ret;
	tmp|= 0x02;
	ret = icm20948_write(sensor, ICM20948_USER_CTRL, &tmp, 1);
	if (ret != ESP_OK)
		return ret;
	
	vTaskDelay(10 / portTICK_PERIOD_MS);

	// Enable I2C_Master
	ret = icm20948_read(sensor, ICM20948_USER_CTRL, &tmp, 1);
	if (ret != ESP_OK)
		return ret;
	tmp|= 0x20;
	ret = icm20948_write(sensor, ICM20948_USER_CTRL, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	// I2C Master Clock 400 kHz
	ret = icm20948_set_bank(sensor, 3);
	if (ret != ESP_OK)
		return ret;

	tmp = 0x07;
	ret = icm20948_write(sensor, ICM20948_I2C_MST_CTRL, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	// I2C Duty Cycle Mode
	ret = icm20948_set_bank(sensor, 0);
	if (ret != ESP_OK)
		return ret;

	tmp = 0x40;
	ret = icm20948_write(sensor, ICM20948_LP_CONFIG, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	// I2C ODR 136 Hz
	ret = icm20948_set_bank(sensor, 3);
	if (ret != ESP_OK)
		return ret;

	tmp = 0x03;
	ret = icm20948_write(sensor, ICM20948_I2C_MST_ODR_CONFIG, &tmp, 1);
	if (ret != ESP_OK)
		return ret;

	// Reset magnetometer
	tmp = 0x01;
	ret = icm20948_write_MAG(sensor, (uint8_t) M_REG_CNTL3, &tmp);
	if (ret != ESP_OK)
		return ret;

	vTaskDelay(100 / portTICK_PERIOD_MS);

	// Continuous mode 4 (100 Hz measurements)
	tmp = 0x08;
	ret = icm20948_write_MAG(sensor, (uint8_t) M_REG_CNTL2, &tmp);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_read_MAG(sensor, M_REG_HXL, 8);
	if(ret != ESP_OK){
		return ret;
	}

	return ret;

}

esp_err_t
icm20948_get_raw_mag(icm20948_handle_t sensor, icm20948_raw_sensor_data_t *const raw_mag_value)
{
	uint8_t data_rd[8] = {0,0,0,0,0,0,0,0};
	esp_err_t ret;

	ret = icm20948_set_bank(sensor, 0);
	if (ret != ESP_OK)
		return ret;
		
	ret = icm20948_read(sensor, ICM20948_EXT_SLV_SENS_DATA_00, data_rd, sizeof(data_rd));
	if (ret != ESP_OK)
		return ret;

	raw_mag_value->raw_mag_x = (int16_t)((data_rd[1] << 8) + (data_rd[0]));
	raw_mag_value->raw_mag_y = (int16_t)((data_rd[3] << 8) + (data_rd[2]));
	raw_mag_value->raw_mag_z = (int16_t)((data_rd[5] << 8) + (data_rd[4]));
	return ret;
}
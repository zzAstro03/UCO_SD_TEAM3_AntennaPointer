#include <stdio.h>
#include <math.h>

#include "esp_log.h"
#include "driver/i2c.h"
#include "icm20948.h"

#define I2C_MASTER_SCL_IO  	38				    /*!< gpio number for I2C master clock YELLOW WIRE*/
#define I2C_MASTER_SDA_IO  	37  				/*!< gpio number for I2C master data  BLUE WIRE*/
#define I2C_MASTER_NUM     	I2C_NUM_0 			/*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 	400000    			/*!< I2C master clock frequency */
#define PI 3.14159265358979323846

static const char *TAG = "icm test";
static icm20948_handle_t icm20948 = NULL; // Accel and gyro object

static esp_err_t
i2c_bus_init(void)
{
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
	conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

	esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
	if (ret != ESP_OK)
		return ret;

	return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

static esp_err_t
icm20948_configure(icm20948_acce_fs_t acce_fs, icm20948_gyro_fs_t gyro_fs)
{
	esp_err_t ret;

	icm20948 = icm20948_create(I2C_MASTER_NUM, ICM20948_I2C_ADDRESS);
	if (icm20948 == NULL) {
		ESP_LOGE(TAG, "ICM20948 create returned NULL!");
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "ICM20948 creation successfull!");
	
	ret = icm20948_reset(icm20948);
	if (ret != ESP_OK)
		return ret;

	vTaskDelay(10 / portTICK_PERIOD_MS);

	ret = icm20948_wake_up(icm20948);
	if (ret != ESP_OK)
		return ret;

	ret = icm20948_set_bank(icm20948, 0);
	if (ret != ESP_OK)
		return ret;

	uint8_t device_id;
	ret = icm20948_get_deviceid(icm20948, &device_id);
	if (ret != ESP_OK)
		return ret;
	ESP_LOGI(TAG, "0x%02X", device_id);
	if (device_id != ICM20948_WHO_AM_I_VAL)
		return ESP_FAIL;

	ret = icm20948_set_gyro_fs(icm20948, gyro_fs);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = icm20948_set_acce_fs(icm20948, acce_fs);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = icm20948_mag_init(icm20948);
	if (ret != ESP_OK)
		return ESP_FAIL;

	return ret;
}

void
icm_read_task(void *args)
{
	esp_err_t ret = icm20948_configure(ACCE_FS_16G, GYRO_FS_2000DPS);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "ICM configuration failure");
		vTaskDelete(NULL);
	}
	ESP_LOGI(TAG, "ICM20948 configuration successfull!");

	icm20948_sensor_data_t sensorData;
	icm20948_raw_sensor_data_t rawSensorData;
	int count = 0;
        while(1){
			
            ret = icm20948_get_acce(icm20948, &sensorData);
            float elevation = -1*atan(sensorData.acce_z/sqrt(pow(sensorData.acce_x, 2) + pow(sensorData.acce_y,2)))*180/PI+90;
            /*if(ret == ESP_OK){
                //ESP_LOGI(TAG, "ax: %lf ay: %lf az: %lf", sensorData.acce_x, sensorData.acce_y, sensorData.acce_z);
                ESP_LOGI(TAG, "Elevation: %lf degrees", elevation);
            }*/

			/*
			ret = icm20948_get_gyro(icm20948, &sensorData);
			if (ret == ESP_OK)
				ESP_LOGI(TAG, "gx: %lf gy: %lf gz: %lf", sensorData.gyro_x, sensorData.gyro_y, sensorData.gyro_z);
			*/

/*
	Need to eventually incorporate this into the icm20948.c file as a 
	function. Currently need to calibrate data, then apply offsets,
	then combine to get device orientation


*/ 
			//ret = icm20948_get_acce(icm20948, &sensorData);
			ret = icm20948_get_raw_mag(icm20948, &rawSensorData);
			float Gx = sensorData.acce_x;
			float Gy = sensorData.acce_y;
			float Gz = sensorData.acce_z;
			float Hx = (float) rawSensorData.raw_mag_x*4192/32752;
			float Hy = (float) rawSensorData.raw_mag_y*4192/32752;
			float Hz = (float) rawSensorData.raw_mag_z*4192/32752;
			float dataStorage[10] = {0,0,0,0,0,0,0,0,0,0};
			float averageAzimuth;

			float magA = sqrt( pow((-1*Gy*Hz + Gz*Hy),2) + pow((Gz*Hx + Gx*Hz),2) + pow((-1*Gx*Hy - Gy*Hx),2));
			float magBx = sqrt(Gy*Gy + Gz*Gz);
			float magBy = sqrt(Gx*Gx + Gz*Gz);
			float magBz = sqrt(Gx*Gx + Gy*Gy);

			float AdotBx = Gx*(Gy*Hy + Gz*Hz) + Hx*(Gy*Gy + Gz*Gz);
			float AdotBy = Gy*(Gz*Hz - Gx*Hx) - Hy*(Gz*Gz + Gx*Gx);
			float AdotBz = Gz*(Gy*Hy - Gx*Hx) - Hz*(Gx*Gx + Gy*Gy);

			// These azimuth will change based on how the sensor is orientated
			float azimuthX = acos(AdotBx /  (magA * magBx))*180/PI;
			float azimuthY = acos(AdotBy /  (magA * magBy))*180/PI;
			float azimuthZ = acos(AdotBz /  (magA * magBz))*180/PI;

			if (ret == ESP_OK){
				//ESP_LOGI(TAG, "mx: %lf my: %lf mz: %lf uT", Hx, Hy, Hz);
				
				//ESP_LOGI(TAG, "AzimuthX: %lf AzimuthY: %lf AzimuthZ: %lf", azimuthX, azimuthY, azimuthZ);
				//ESP_LOGI(TAG, "AzimuthX: %lf degrees", azimuthX);
				//ESP_LOGI(TAG, "AzimuthY: %lf degrees", azimuthY);
				//ESP_LOGI(TAG, "AzimuthZ: %lf degrees", azimuthZ);
				
				if(count == 500){
					ESP_LOGI(TAG, "Azimuth: %lf degrees", azimuthY);
					ESP_LOGI(TAG, "Elevation: %lf degrees\n", elevation);
					count = 0;
				}
				
			}

			count = count + 1;

            //vTaskDelay(1 / portTICK_PERIOD_MS);
        }

	vTaskDelete(NULL);
}

void
app_main(void)
{
	ESP_LOGI(TAG, "Starting ICM test");
	esp_err_t ret = i2c_bus_init();
	ESP_LOGI(TAG, "I2C bus initialization: %s", esp_err_to_name(ret));

    xTaskCreate(icm_read_task, "icm read task", 1024 * 10, NULL, 15, NULL);
	
}
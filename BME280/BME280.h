#ifndef BME280_H
#define BME280_H

#include<iostream>
#include<pigpiod_if2.h>
#include<unistd.h>
#include<expected>










class BME280{

	public:
		//Datas Required for the BME280.
		typedef struct{

			int32_t tempreture;
			int32_t pressure;
			int32_t humidity;

		} Datas;

		//Nubers for Over_Sample command.
		//If OVER_SAMPLE_X, Over sampling X times. 
		typedef enum{

			OVER_SAMPLE_SKIP = 0x00,
			OVER_SAMPLE_1 =	0x01,
			OVER_SAMPLE_2 =	0x02,
			OVER_SAMPLE_4 =	0x03,
			OVER_SAMPLE_8 =	0x04,
			OVER_SAMPLE_16 = 0x05
		}Over_Sample;

		//BME280 modes.
		typedef enum{

			SLEEP_MODE = 0x00,
			FORCE_MODE_FROM_NORMAL = 0x01, //If Normal mode, data is automatically updated.
			FORCE_MODE = 0x02,
			NORMAL_MODE = 0x03 //If Force mode, data is update manually and enter sleep mode.

		}Mode;
	
		//Numbers for Sampling_time command.
		//If SAMPLE_T_X,Sampling time is X[ms].	
		typedef enum{

			SAMPLE_T_05 = 0x00,
			SAMPLE_T_625 = 0x01,
			SAMPLE_T_1250 = 0x02,
			SAMPLE_T_2500 = 0x03,
			SAMPLE_T_5000 = 0x04,
			SAMPLE_T_10000 = 0x05,
			SAMPLE_T_100 = 0x06,
			SAMPLE_T_200 = 0x07

		}Sampling_Time;

		//Numbers for IIR_Coefficient.
		//If FILTER_X,IIR coefficient is X.
		typedef enum{

			FILTER_OFF = 0x00, //Unusing IIR filter in BME280.  
			FILTER_2 = 0x01,
			FILTER_4 = 0x02,
			FILTER_8 = 0x03,
			FILTER_16 = 0x04

		}IIR_Coefficient;

		//Decide SPI3 state.
		typedef enum{

			SPI3_ENABLE = 0x01, //Using SPI3 mode.
			SPI3_DISABLE = 0x00 //Unusing SPI3 mode.

		}SPI3_Enable;

		//Address and commands for BME280. 
		typedef enum{

			ID_ADDRESS = 0xD0, //Address for read BME280 ID.
			RESET_ADDRESS = 0xE0, //Address for software reset BME280.
			RESET_COMMAND = 0xB6, //Command for software reset BME280. After use RESET_ADDRESS.
			HUMIDITY_CTRL_ADDRESS = 0xF2, //Address for controll humidity oversampling.
			STATUS_ADDRESS = 0xF3, //Address for read data status updating flag or measuring flag.
			MEAS_ADDRESS = 0xF4, //Address for controll press and temperature oversampling and for changing the BME280's operating mode.
			CONFIG_ADDRESS = 0xF5, //Address for Set sampling time,IIR Cofficient and SPI3 En/Dis. 
			DATA_ADDRESS = 0xF7, //Start address of the data.
			CORRECTION_ADDRESS = 0x88, //Start address of the data's coefficient(0x88~0xA1).
			CORRECTION_ADDRESS1 = 0xE1 //Start Address of the data's coefficient(0xE1~0xE6).

		}Address;

		//Device ID and I2C address for BME280.
		typedef enum{

			ID = 0x60, //Device ID for BME280.
			I2C_ADDRESS_0 = 0x76, //I2C address.
			I2C_ADDRESS_1 = 0x77 //I2C address.

		}Device;

		//Mask for status byte.
		typedef enum{

			MEASURING = 0x08,
			UPDATE = 0x01

		}Mask;

		//Flags and data for BME280 configration.
		typedef struct{

			Over_Sample humidity_over_sample,tempreture_over_sample,press_over_sample;
			Mode mode;
			Sampling_Time sample_time;
			IIR_Coefficient iir_coefficient;
			SPI3_Enable spi_en;
			Device device_ID,I2C_ID;

		}Device_Info;


		Datas data;
		double now_height;
		

		int handle,pi_bme280; // BME280's I2C handle and raspberry pi handle.

		BME280(int pi,int i2c_num);//Create BME280's constructor.[pi] is pigpiod handle ID.[i2c_num] is I2C channnel number.
		[[nodiscard]]std::expected<void,std::string>Setting(Device_Info set_info);
		[[nodiscard]]std::expected<void,std::string>Read_Data(bool pressure,bool humidity);
		~BME280(); //BME280's dectructor.


	protected:		

		Datas raw;

		Device_Info device_info;
		
		char raw_data[8];
		char raw_correction[26];
		char raw_correction1[7];

		uint16_t t1,p1;
		int16_t t2,t3,p2,p3,p4,p5,p6,p7,p8,p9,h2,h4,h5,h6;
		uint8_t h1,h3;


		/* BME280 communicate command. */
		[[nodiscard]]bool Check(); //Communicate BME280.
		[[nodiscard]]bool Read_ID(); //Read BME280 ID.
		void Reset(); //Reset BME280

		/* BME280 Setting command. */
		[[nodiscard]]bool Ctrl_Humidity(uint8_t command); //Control humidity sampling.
		[[nodiscard]]bool Config(uint8_t command); //Configrate BME280.
		[[nodiscard]]bool Ctrl_Pressure_Tempreture(uint8_t command); //Control pressure and humidity sampling.
		
		/* BME280 status command. */
		[[nodiscard]]bool Status(); //See BME280's status.


		/* BME280 read data command. */
		[[nodiscard]]std::expected<void,std::string> Catch_Data(bool pressure,bool humidity); //Read BME280 data.

		/* Get coefficients command. */
		[[nodiscard]]bool Coefficients(); //Read BME280 coefficients.
		
		/* Get BME280 data command. */
		[[nodiscard]]int64_t Tempreture_Read();
		[[nodiscard]]bool Pressure_Read(int64_t t_fine);
		void Humidity_Read(int64_t t_fine);



};

#endif
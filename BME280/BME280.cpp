#include"BME280.h"
#include<cassert>
#include<expected>
#include<chrono>










/*
 * There are the public functions of the BME280. 
 */






//It's BME280 constructor. [pi] is pigpiod handle ID. [i2c_num] is I2C channnel number.
BME280::BME280(int pi,int i2c_num){

	handle=i2c_open(pi,i2c_num,Device::I2C_ADDRESS_0,0);
	assert(handle >= 0);
	pi_bme280=pi;

	return;

}


//It's BME280 configration function. [set_info] is BME280 configration struct.The required information is oversample,mode,sampling time,IIR Coefficient,and SPI Enable/Disable.
[[nodiscard]]std::expected<void,std::string>BME280::Setting(Device_Info set_info){

	Reset();
	if(!Check()){ return std::unexpected("Error!BME280 is not found."); }
	if(!Read_ID()){ return std::unexpected("Error! RaspberryPi can't communicate with BME280."); }
	
	uint8_t ctrl_hum_command,ctrl_meas_command,config_command;

	ctrl_hum_command = (uint8_t)set_info.humidity_over_sample;
	ctrl_meas_command =(uint8_t)(set_info.tempreture_over_sample << 5) | (set_info.press_over_sample << 2) | (set_info.mode);	
	config_command =(uint8_t)(set_info.sample_time << 5) | (set_info.iir_coefficient << 2) | (set_info.spi_en);

	if(!Ctrl_Humidity(ctrl_hum_command) ){ return std::unexpected("Error!Can't \"write ctrl_hum\" command."); }
	if(!Ctrl_Pressure_Tempreture(ctrl_meas_command) ){ return std::unexpected("Error!Can't write \"ctrl_meas\" command."); }
	if(!Config(config_command) ){ return std::unexpected("Error!Can't write \"config\" command."); }

	device_info=set_info;

	if(!Coefficients()){ return std::unexpected("Error!Can't read coefficient."); }

	return {};

}


//It's read data from BME280. [pressure] is update pressure data flag. And [Humidity] is update humidity flag.
[[nodiscard]]std::expected<void,std::string>BME280::Read_Data(bool pressure=false,bool humidity=false){
	
	if(!Check()){ return std::unexpected("Error!BME280 is not found."); }
	if(!Read_ID()){ return std::unexpected("Error! RaspberryPi can't communicate with BME280."); }

	uint8_t force_mode_command = (device_info.tempreture_over_sample << 5) | (device_info.press_over_sample << 2) | Mode::FORCE_MODE;


	switch(device_info.mode){

		case Mode::SLEEP_MODE:
			return std::unexpected("Sorry.This device is sleep mode.Pleese select mode \"Normal\" or \"Force\".");

		case Mode::FORCE_MODE_FROM_NORMAL:
			device_info.mode = Mode::FORCE_MODE;
			[[fallthrough]];

		case Mode::FORCE_MODE:
			if(!Ctrl_Pressure_Tempreture(force_mode_command)){ return std::unexpected("Error,BME280 is sleep and can't communication now."); }
			[[fallthrough]];

		case NORMAL_MODE:{
			auto start = std::chrono::system_clock::now();
			while(!Status()){
				auto now = std::chrono::system_clock::now();
				if( (now-start) >= std::chrono::milliseconds(500) ){ return std::unexpected("Error!Device is timeout."); }

				usleep(1000);
			}

			auto no_error=Catch_Data(pressure,humidity);
			if(!no_error){ return std::unexpected(no_error.error()); }
			return {};

		}	


		default:
			break;

	}


	return std::unexpected("Error.Please set value.");	


}


//BME280 destructor.
BME280::~BME280(){

	i2c_close(pi_bme280,handle);

}








/*
 * There are the private functions of the BME280.
 */









/* BME280 reset and suvival confirmation command*/







//BME280 check function.If communicate BME280,return true.
[[nodiscard]]bool BME280::Check(){
	
	uint8_t error_check=0;
	while(error_check<5){
		if(i2c_write_quick(pi_bme280,handle,0)==0){ return true; } 
		error_check++;
	}
	return false;
	
}


//Read ID command.If read BME280 ID,return true.
[[nodiscard]]bool BME280::Read_ID(){

	uint8_t error_check=0;
	while(error_check<5){
		if(i2c_read_byte_data(pi_bme280,handle,Address::ID_ADDRESS) == Device::ID){ return true; }
		error_check++;
	}
	return false;

}


//Reset BME280.
void BME280::Reset(){

	i2c_write_byte_data(pi_bme280,handle,Address::RESET_ADDRESS,Address::RESET_COMMAND);
	usleep(1000);
	return;

}







/* BME280 configrate command. */





//Configrate humidity over sample. If change the setting, return true.
[[nodiscard]]bool BME280::Ctrl_Humidity(uint8_t command){

	i2c_write_byte_data(pi_bme280,handle,Address::HUMIDITY_CTRL_ADDRESS,command);
	usleep(100);
	if( (i2c_read_byte_data(pi_bme280,handle,Address::HUMIDITY_CTRL_ADDRESS) & 0x07) != command ){ return false; }
	else{ return true; }

}


//BME280 Configration command. If change the setting, return true.
[[nodiscard]]bool BME280::Config(uint8_t command){
	

	i2c_write_byte_data(pi_bme280,handle,Address::CONFIG_ADDRESS,command);
	usleep(100);
	if(i2c_read_byte_data(pi_bme280,handle,Address::CONFIG_ADDRESS) != command ){ return false; } 
	else{ return true; }

}


//Configrate pressure and tempreture over sample and change mode. If change the seting, return true.
[[nodiscard]]bool BME280::Ctrl_Pressure_Tempreture(uint8_t command){

	i2c_write_byte_data(pi_bme280,handle,Address::MEAS_ADDRESS,command);
	usleep(100);
	if(i2c_read_byte_data(pi_bme280,handle,Address::MEAS_ADDRESS) != command ){ return false; }
	else{ return true; }

}





/* BME280 status command. */



//Read BME280 status,If good timing, return true. 
[[nodiscard]]bool BME280::Status(){

	if( (i2c_read_byte_data(pi_bme280,handle,Address::STATUS_ADDRESS) & (Mask::MEASURING | Mask::UPDATE) ) == 0){ return true; }
	else{ return false; }
	
}













/* BME280 culcurate and read command. */






//Read and calculate BME280 data.
[[nodiscard]]std::expected<void,std::string> BME280::Catch_Data(bool pressure,bool humidity){

	if(i2c_read_i2c_block_data(pi_bme280,handle,Address::DATA_ADDRESS,raw_data,8) < 0){ return std::unexpected("Error! Can't read data."); };
	raw.tempreture=(int32_t) ( (raw_data[3]<<12) | (raw_data[4]<<4) | (raw_data[5]>>4) );
	int32_t t_fine=Tempreture_Read();
	
	if(pressure){ 

		raw.pressure=(int32_t) ( (raw_data[0]<<12) | (raw_data[1]<<4) | (raw_data[2]>>4) );
	       	if( !Pressure_Read(t_fine) ){ return std::unexpected("Error.Pressure is funny."); }

       	}

	if(humidity){  

		raw.humidity=(int32_t) ( (raw_data[6]<<8) | (raw_data[7]) );
	       	Humidity_Read(t_fine); 

	}

	return {};

}








/* BME280 coefficient command. */





//Read the coeffients required for the calucutration.
[[nodiscard]]bool BME280::Coefficients(){

	if(i2c_read_i2c_block_data(pi_bme280,handle,Address::CORRECTION_ADDRESS,raw_correction,25) < 0){ return false; }
	if(i2c_read_i2c_block_data(pi_bme280,handle,Address::CORRECTION_ADDRESS1,raw_correction1,7) < 0){ return false; } //Burst reading

	t1=(uint16_t)( (raw_correction[1]<<8) | raw_correction[0] );
	t2=(int16_t)( (raw_correction[3]<<8) | raw_correction[2] );
	t3=(int16_t)( (raw_correction[5]<<8) | raw_correction[4] );

	p1=(uint16_t)( (raw_correction[7]<<8) | raw_correction[6] );
	p2=(int16_t)( (raw_correction[9]<<8)| raw_correction[8] );
	p3=(int16_t)( (raw_correction[11]<<8) | raw_correction[10] );
	p4=(int16_t)( (raw_correction[13]<<8) | raw_correction[12] );
	p5=(int16_t)( (raw_correction[15]<<8) | raw_correction[14] );
	p6=(int16_t)( (raw_correction[17]<<8) | raw_correction[16] );
	p7=(int16_t)( (raw_correction[19]<<8) | raw_correction[18] );
	p8=(int16_t)( (raw_correction[21]<<8) | raw_correction[20] );
	p9=(int16_t)( (raw_correction[23]<<8) | raw_correction[22] );

	h1=(uint8_t)raw_correction[24]; 
	h2=(int16_t)( (raw_correction1[1]<<8) | raw_correction1[0] );
	h3=(uint8_t)raw_correction1[2];
	h4=(int16_t)( (raw_correction1[3]<<4) | (raw_correction1[4] >> 4) );
	h5=(int16_t)( ( (raw_correction1[4] & 0x0f) << 4) | raw_correction1[5] >> 4 );
	h6=(int8_t) (raw_correction1[6]);

	return true;

}



/* BME280 calcurrate command. */




//Calculate BME280 tempreture data.
[[nodiscard]]int64_t BME280::Tempreture_Read(){

	int64_t var1,var2,t_fine;

	var1=( ( (raw.tempreture >> 3)-(t1<<1) )*t2 ) >> 11;
	var2=( ( ( ( (raw.tempreture >> 4)-t1)*( (raw.tempreture >> 4)-t1) ) >> 12 )*t3) >>14;
	t_fine=var1+var2;
	data.tempreture=( (t_fine*5+128) >> 8);

	return t_fine;

}


//Calcurate BME280 pressure data.
[[nodiscard]]bool BME280::Pressure_Read(int64_t t_fine){

	int64_t var1,var2;
	int64_t p;

	var1=t_fine-128000;
	var2=var1*var1*(int64_t)p6;
	var2=var2+( (var1*(int64_t)p5) << 17 );
	var2=var2+( (int64_t)p4 << 35);
	var1=( (var1*var1*(int64_t)p3) >> 8)+( (var1*(int64_t)p2) << 12);
	var1=( ( ( (int64_t)1 << 47)+var1)*(int64_t)p1) >>33;
	
	if(var1==0){ return false; }

	p=1048576-raw.pressure;
	p=(int64_t) ( ( ( (p << 31)-var2)*3125)/var1);
	var1=( (int64_t)p9*(p >> 13)*(p >> 13) ) >> 25;
	var2=( (int64_t)p8*p) >> 19;
	p=( (p+var1+var2) >> 8)+( (int64_t)p7 << 4);
	data.pressure=p;

	return true;

}


//Calcurate BME280 humidity data.
void BME280::Humidity_Read(int64_t t_fine){

	int64_t var1;
	var1=t_fine-76800;
	var1=( ( ( ( (raw.humidity << 14)-(h4 << 20)-( (int64_t)h5*var1) )+16384) >> 15)*( ( ( ( ( ( (var1*(int64_t)h6) >> 10)*( ( (var1*(int64_t)h3) >> 11)+32768) ) >> 10)+2097152)*h2+8192) >> 14) );
	var1=var1-( ( ( (var1 >>15)*(var1 >> 15) >> 7)*h1) >> 4);
	var1=(var1<0) ? 0:var1;
	var1=(var1>419430400) ? 419430400:var1;
	data.humidity=var1>>12;
	return;

}
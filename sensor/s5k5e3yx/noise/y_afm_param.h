/*Y_AFM*/		/*y_afm_param.h*/

/*
struct sensor_y_afm_level {
	uint32_t iir_bypass;
	uint8_t skip_num;
	uint8_t afm_format;	
	uint8_t afm_position_sel;	
	uint8_t shift;
	uint16_t win[25][4];
	uint16_t coef[11];		
	uint16_t reserved1;
};
*/
                              
 
{0x01,0x00,0x07,0x00,0x00,
{{660,700,760,800},{625,1050,775,1250},{440,300,600,470},{300,500,400,700},{550,576,680,680},
{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
{660,700,760,800},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{18,43,-9,64,-128,64,57,-34,64,-128,64,},0x00,}
},


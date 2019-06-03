/*param0.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x01,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0031,0x00D5,0x0233,0x03FF,0x03FF,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x003B,0x001C,0x0010,0x000B,0x000B,0x000B,0x000B,0x000B/*0-7*/
		},
		/*intercept_b*/
		{
			0x0008,0x000E,0x0018,0x0023,0x0023,0x0023,0x0023,0x0023/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x001E,0x001E,0x0078,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x001E,0x001E,0x0078,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x07,0x02,0x09/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0190,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0005,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x02,
		/*lowoffset*/
		0x02,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x0028,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param1.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0031,0x00D5,0x0233,0x03FF,0x03FF,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x003B,0x001C,0x0010,0x000B,0x000B,0x000B,0x000B,0x000B/*0-7*/
		},
		/*intercept_b*/
		{
			0x0008,0x000E,0x0018,0x0023,0x0023,0x0023,0x0023,0x0023/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x07,0x02,0x09/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0190,
		/*texture_th*/
		0x0096,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0005,
			/*max*/
			0x0005,

		},
		/*lowcoeff*/
		0x02,
		/*lowoffset*/
		0x02,
		/*highcoeff*/
		0x00,
		/*highoffset*/
		0x00,
		/*hv_ratio*/
		0x0028,
		/*rd_ration*/
		0x002D,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param2.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0031,0x00D5,0x0233,0x03FF,0x03FF,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x003B,0x001C,0x0010,0x000B,0x000B,0x000B,0x000B,0x000B/*0-7*/
		},
		/*intercept_b*/
		{
			0x0008,0x000E,0x0018,0x0023,0x0023,0x0023,0x0023,0x0023/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x07,0x02,0x09/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0190,
		/*texture_th*/
		0x0096,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0005,
			/*max*/
			0x0005,

		},
		/*lowcoeff*/
		0x02,
		/*lowoffset*/
		0x02,
		/*highcoeff*/
		0x00,
		/*highoffset*/
		0x00,
		/*hv_ratio*/
		0x0028,
		/*rd_ration*/
		0x002D,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param3.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0031,0x00D5,0x0233,0x03FF,0x03FF,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x003B,0x001C,0x0010,0x000B,0x000B,0x000B,0x000B,0x000B/*0-7*/
		},
		/*intercept_b*/
		{
			0x0008,0x000E,0x0018,0x0023,0x0023,0x0023,0x0023,0x0023/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x07,0x02,0x09/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0190,
		/*texture_th*/
		0x0096,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0005,
			/*max*/
			0x0005,

		},
		/*lowcoeff*/
		0x02,
		/*lowoffset*/
		0x02,
		/*highcoeff*/
		0x00,
		/*highoffset*/
		0x00,
		/*hv_ratio*/
		0x0028,
		/*rd_ration*/
		0x002D,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param4.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param5.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param6.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param7.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param8.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param9.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param10.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param11.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param12.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param13.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param14.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param15.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param16.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param17.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param18.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param19.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param20.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param21.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param22.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param23.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,
/*param24.*/
/*sharkl5_sensor_bpc_level*/
{
	/*bpc_comm*/
	{
		/*bpc_mode*/
		0x00,
		/*hv_mode*/
		0x00,
		/*rd_mode*/
		0x00,
		/*isMonoSensor*/
		0x00,
		/*double_bypass*/
		0x00,
		/*three_bypass*/
		0x00,
		/*four_bypass*/
		0x00,
		/*reserved*/
		0x00,
		/*lut_level*/
		{
			0x0002,0x001B,0x0088,0x0155,0x02AB,0x03FF,0x03FF,0x03FF/*0-7*/
		},
		/*slope_k*/
		{
			0x019C,0x0059,0x0022,0x0013,0x000D,0x000A,0x000A,0x000A/*0-7*/
		},
		/*intercept_b*/
		{
			0x0000,0x0003,0x0009,0x0011,0x0019,0x0021,0x0021,0x0021/*0-7*/
		},
		/*dtol*/
		0.20,

	},
	/*bpc_pos*/
	{
		/*continuous_mode*/
		0x00,
		/*skip_num*/
		0x00,
		/*reserved*/
		{
			0x00,0x00/*0-1*/
		}
	},
	/*bpc_thr*/
	{
		/*double_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*three_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*four_th*/
		{
			0x0000,0x001E,0x003C,0x0078/*0-3*/
		},
		/*shift*/
		{
			0x01,0x02,0x03/*0-2*/
		},
		/*reserved*/
		0x00,
		/*flat_th*/
		0x0000,
		/*texture_th*/
		0x0000,

	},
	/*bpc_rules*/
	{
		/*k_val*/
		{
			/*min*/
			0x0012,
			/*max*/
			0x0012,

		},
		/*lowcoeff*/
		0x05,
		/*lowoffset*/
		0xFF,
		/*highcoeff*/
		0x05,
		/*highoffset*/
		0xFF,
		/*hv_ratio*/
		0x003C,
		/*rd_ration*/
		0x003C,

	},
	/*ppi_block*/
	{
		/*ppi_en*/
		0x00000001,
		/*block_start_col*/
		0x0000,
		/*block_start_row*/
		0x0000,
		/*block_end_col*/
		0x0000,
		/*block_end_row*/
		0x0000,

	},
	/*bpc_result_en*/
	0x0000,
	/*bypass*/
	0x0000,
}
,

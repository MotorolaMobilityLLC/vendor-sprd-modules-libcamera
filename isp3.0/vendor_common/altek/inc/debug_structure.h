/******************************************************************************
 *  File name: debug_structure.h
 *  Latest Update Date:2016/04/14
 *
 *  Comment:
 *  Describe structure definition of debug information
 *****************************************************************************/
#include "mtype.h"

#define DEBUG_STRUCT_VERSION               (501)

#define MAX_AEFE_DEBUG_SIZE_STRUCT1        (669)
#define MAX_AEFE_DEBUG_SIZE_STRUCT2        (5120)
#define MAX_FLICKER_DEBUG_SIZE_STRUCT1     (8)
#define MAX_FLICKER_DEBUG_SIZE_STRUCT2     (600)
#define MAX_AWB_DEBUG_SIZE_STRUCT1         (348)
#define MAX_AWB_DEBUG_SIZE_STRUCT2         (10240)
#define MAX_AF_DEBUG_SIZE_STRUCT1          (385)
#define MAX_AF_DEBUG_SIZE_STRUCT2          (7168)
#define MAX_BIN1_DEBUG_SIZE_STRUCT2        (30000)
#define MAX_BIN2_DEBUG_SIZE_STRUCT2        (1137144)
#define START_STR_SIZE                     (32)
#define END_STR_SIZE                       (24)
#define PROJECT_NAME_SIZE                  (16)
#define OTP_RESERVED1_SIZE                 (8)
#define OTP_RESERVED2_SIZE                 (8)
#define OTP_RESERVED3_SIZE                 (6)
#define OTP_LSC_DATA_SIZE                  (1658)
#define SHADING_DEBUG_VERSION_NUMBER       (12)
#define IRP_TUNING_DEBUG_INFO              (6)
#define IRP_TUNING_DEBUG_CCM               (9)
#define IRP_TUNING_DEBUG_PARA_ADDR         (56)
#define IRP_TUNING_DEBUG_RESERVED          (5)
#define MAX_IRP_GAMMA_TONE_SIZE            (1027)
#define ISP_SW_DEBUG_RESERVED_SIZE         (59)
#define OTHER_DEBUG_RESERVED_SIZE          (11)

enum e_scinfo_color_order {
	E_SCINFO_COLOR_ORDER_RG = 0,
	E_SCINFO_COLOR_ORDER_GR,
	E_SCINFO_COLOR_ORDER_GB,
	E_SCINFO_COLOR_ORDER_BG
};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct otp_report_debug1 {
	uint8 current_module_calistatus;  /* 00:Fail, 01:Success */
	uint8 current_module_year;  /* eg: 2015 0x0F */
	uint8 current_module_month;  /* eg: Jan 0x01 */
	uint8 current_module_day;  /* eg: 1st 0x01 */
	uint8 current_module_mid;  /* 0x02(Truly) */
	uint8 current_module_lens_id;
	uint8 current_module_vcm_id;
	uint8 current_module_driver_ic;
	uint8 reserved1[OTP_RESERVED1_SIZE];  /* Set 0x00 */
	uint16 current_module_iso;  /* MinISO */
	uint16 current_module_r_gain;  /* ISO_Gain_R */
	uint16 current_module_g_gain;  /* ISO_Gain_G */
	uint16 current_module_b_gain;  /* ISO_Gain_B */
	uint8 reserved2[OTP_RESERVED2_SIZE];  /* Set 0x00 */
	uint8 reserved3[OTP_RESERVED3_SIZE];  /* Set 0x00 */
	uint8 current_module_af_flag;  /* 00:Fail, 01:Success */
	uint16 current_module_infinity;
	uint16 current_module_macro;
	uint8 total_check_sum;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct otp_report_debug2 {
	uint8 current_module_calistatus;  /* 00:Fail, 01:Success */
	uint8 current_module_year;  /* eg: 2015 0x0F */
	uint8 current_module_month;  /* eg: Jan 0x01 */
	uint8 current_module_day;  /* eg: 1st 0x01 */
	uint8 current_module_mid;  /* 0x02(Truly) */
	uint8 current_module_lens_id;
	uint8 current_module_vcm_id;
	uint8 current_module_driver_ic;
	uint8 reserved1[OTP_RESERVED1_SIZE];  /* Set 0x00 */
	uint16 current_module_iso;  /* MinISO */
	uint16 current_module_r_gain;  /* ISO_Gain_R */
	uint16 current_module_g_gain;  /* ISO_Gain_G */
	uint16 current_module_b_gain;  /* ISO_Gain_B */
	uint8 reserved2[OTP_RESERVED2_SIZE];  /* Set 0x00 */
	uint8 current_module_lsc[OTP_LSC_DATA_SIZE];  /* LSC Data */
	uint8 reserved3[OTP_RESERVED3_SIZE];  /* Set 0x00 */
	uint8 current_module_af_flag;  /* 00:Fail, 01:Success */
	uint16 current_module_infinity;
	uint16 current_module_macro;
	uint8 total_check_sum;
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct raw_img_debug {
	uint16 raw_width;  /* CFA Width */
	uint16 raw_height;  /* CFA Height */
	uint8 raw_format;  /* altek RAW10, android RAW10, packed10, unpacted10 */
	uint8 raw_bit_number;  /* 8bit or 10 bit */
	uint8 mirror_flip;  /* no, mirror, flip or mirror+flip */
	enum e_scinfo_color_order raw_color_order;  /* BGGR */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct shading_debug {
	uint16 wb_r_gain_balanced;  /* (10-bit), 1x: 256, balanced WB gain for shading */
	uint16 wb_g_gain_balanced;  /* (10-bit), 1x: 256, balanced WB gain for shading */
	uint16 wb_b_gain_balanced;  /* (10-bit), 1x: 256, balanced WB gain for shading */
	uint8 input_data;  /* Rear sensor */
	uint8 applied_sensor_id;  /* Rear sensor */
	uint8 otp_info;  /* Rear sensor */
	uint8 otp_version;  /* Rear sensor */
	uint16 shading_width;  /* Rear sensor */
	uint16 shading_height;  /* Rear sensor */
	uint16 hr_gain;  /* Rear sensor */
	uint16 mr_gain;  /* Rear sensor */
	uint16 lr_gain;  /* Rear sensor */
	uint16 current_r_gain;  /* Rear sensor */
	uint16 current_b_gain;  /* Rear sensor */
	uint16 current_proj_x;  /* Rear sensor */
	uint16 current_proj_y;  /* Rear sensor */
	uint8 rp_run;  /* Rear sensor */
	uint8 bp_run;  /* Rear sensor */
	uint8 ext_input_data;  /* Front sensor */
	uint8 ext_applied_sensor_id;  /* Front sensor */
	uint8 ext_otp_info;  /* Front sensor */
	uint16 ext_hr_gain;  /* Front sensor */
	uint16 ext_mr_gain;  /* Front sensor */
	uint16 ext_lr_gain;  /* Front sensor */
	uint16 ext_current_proj_x;  /* Front sensor */
	uint16 ext_current_proj_y;  /* Front sensor */
	uint8 ext_rp_run;  /* Front sensor */
	uint8 ext_bp_run;  /* Front sensor */
	uint8 orientation;
	uint8 version_number[SHADING_DEBUG_VERSION_NUMBER];  /* The version of ShadingInfo_XXX.bin (created date+time. ex: 201601122130) */
	uint16 tuning_version;  /* The version of shading fine-tune data (user define) */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct irp_tuning_debug {
	uint32 qmerge_ver;
	uint32 tool_ver;
	uint32 tuning_ver;
	uint8 sensor_id;	/* sensor: 0 (rear 0)
				 * sensor: 1 (rear1)
				 * lite: 2 (front, no YCC NRE)
				 */
	uint16 sensor_type;  /* User definition by sensor vendor/type serial number */
	uint8 sensor_mode;  /* for FR, video/preview size, depend on projects; default (FR + preview) */
	uint8 tuning_id;  /* Normal mode;Saturation mode;contrast mode... */
	uint8 level_type;	/* +2 ;+1.5;+1.0...;-1.5;-2.0
				 * Negative ; Grayscale ; Sepia ...
				 */
	uint8 quality_path;  /* Normal quality ;high quality */
	uint16 bayer_scalar_w;  /* Bayer Scalar width */
	uint16 bayer_scalar_h;  /* Bayer Scalar height */
	uint8 DebugFlag[IRP_TUNING_DEBUG_INFO];  /* verify flag;HDR flag;RDN flag;NRE flag;Asharp flag;GEG flag */
	uint16 wb_r_gain;  /* (10-bit), 1x: 256, WB gain for final result */
	uint16 wb_g_gain;  /* (10-bit), 1x: 256, WB gain for final result */
	uint16 wb_b_gain;  /* (10-bit), 1x: 256, WB gain for final result */
	uint16 r_black_offset;  /* (10-bit) */
	uint16 g_black_offset;  /* (10-bit) */
	uint16 b_black_offset;  /* (10-bit) */
	uint32 color_temperature;
	int32 ccm[IRP_TUNING_DEBUG_CCM];  /* ccm, 1x: 256 */
	uint32 parameter_addr[IRP_TUNING_DEBUG_PARA_ADDR];  /* every parameter ID address */
	uint32 reserved[IRP_TUNING_DEBUG_RESERVED];  /* Reserved */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct irp_gamma_tone {
	uint16 gamma_tone[MAX_IRP_GAMMA_TONE_SIZE];  /* 1. Generate / Tuning CCM to fit the target image color reproduction
						  * 2. Advanced tuning to achieve target camera style.
						  */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct isp_sw_debug {
	uint8 ucSharpnessLevel;  /* UI: Sharpness, ie. -2 ~0(default)~ 2, step 0.5 */
	uint8 ucSaturationLevel;  /* UI: Saturation, ie. -2 ~0(default)~ 2, step 0.5 */
	uint8 ucContrastLevel;  /* UI: Contrast, ie. -2 ~0(default)~ 2, step 0.5 */
	uint8 ucBrightnessValue;  /* UI: Brightness, ie. -2 ~0(default)~ 2, step 0.5 */
	uint8 ucSpecialEffect;	/* UI: Special effect, ie. 0:Off (default),Blue point,
				 * Red-yellow point,Green point,Solarise,Posterise,Washed out,
				 * Warm vintage,Cold vintage,Black & white,Negative
				 */
	uint8 aucReserver[ISP_SW_DEBUG_RESERVED_SIZE];  /* Reserved */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct other_debug {
	uint32 valid_ad_gain;  /* (scale 100) Valid exposure information */
	uint32 valid_exposure_time;  /* (Unit: us) Valid exposure information */
	uint32 valid_exposure_line;  /* Valid exposure information */
	uint32 fn_value;  /* Lens F-number(scale 1000), Fno 2.0 = 2000 */
	uint32 focal_length;  /* The focal length of lens, (scale 1000) 3097 => 3.097 mm */
	uint8 flash_flag;  /* 0: off, 1: preflash on, 2: mainflash on */
	uint8 reserved[OTHER_DEBUG_RESERVED_SIZE];  /* Reserved */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct debug_info1 {
	int8 string1[START_STR_SIZE];  /* ie. exif_str_g2v1 */
	int32 struct_version;
	int8 project_name[PROJECT_NAME_SIZE];
	uint16 main_ver_major;  /* Reserved*/
	uint16 main_ver_minor;  /* Reserved*/
	uint16 isp_ver_major;  /* Reserved*/
	uint16 isp_ver_minor;  /* Reserved*/
	uint32 structure_size1;
	uint8 ae_fe_debug_info1[MAX_AEFE_DEBUG_SIZE_STRUCT1];	/* Update @ each report update
								 * ae_output_data_t -> rpt_3a_update -> ae_update-> ae_commonexif_data
								 * Real size:  ae_output_data_t->rpt_3a_update->ae_update->ae_commonexif_valid_size
								 */
	uint8 flicker_debug_info1[MAX_FLICKER_DEBUG_SIZE_STRUCT1];	/* Update @ each report update
									* flicker_output_data_t -> rpt_update -> flicker_update-> flicker_debug_info1(Exif data)
									* Real size:  flicker_output_data_t -> rpt_update -> flicker_update->flicker_debug_info1_valid_size
									*/
	uint8 awb_debug_info1[MAX_AWB_DEBUG_SIZE_STRUCT1];	/* Update @ each report update
								 * AWB debug_info(Exif data)  is "awb_debug_data_array[]" in the structure "alAWBLib_output_data_t" from alAWBRuntimeObj_t-> estimation
								 */
	uint8 af_debug_info1[MAX_AF_DEBUG_SIZE_STRUCT1];	/* Collect @ finish focus
								 * alAFLib_output_report_t->alAF_EXIF_data[]
								 * alAFLib_output_report_t->alAF_EXIF_data_size
								 */
	struct otp_report_debug1 otp_report_debug_info1;
	struct raw_img_debug raw_debug_info1;	/* Sensor/ISP, included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor,
						 * u8 a_ucOutputImage, u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
						 */
	struct shading_debug shading_debug_info1;	/* Included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor, u8 a_ucOutputImage,
							 * u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
							 */
	struct irp_tuning_debug irp_tuning_para_debug_info1;	/* Included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor, u8 a_ucOutputImage,
								 * u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
								 */
	struct isp_sw_debug sw_debug1;	/* Included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor,
					 * u8 a_ucOutputImage, u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
					 */
	struct other_debug other_debug_info1;
	int8 end_string[END_STR_SIZE];  /* ie. end_exif_str_g2v1 */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct debug_info2 {
	int8 string2[START_STR_SIZE];  /* ie. jpeg_str_g2v1 */
	uint32 structure_size2;
	uint8 ae_fe_debug_info2[MAX_AEFE_DEBUG_SIZE_STRUCT2];	/* Update @ each report update
								 * ae_output_data_t->rpt_3a_update->ae_update->ae_debug_data
								 * Real size: ae_output_data_t->rpt_3a_update->ae_update->ae_debug_valid_size
								 */
	uint8 flicker_debug_info2[MAX_FLICKER_DEBUG_SIZE_STRUCT2];	/* Update @ each report update
									* flicker_output_data_t -> rpt_update -> flicker_update-> flicker_debug_info2
									* Real size:  flicker_output_data_t -> rpt_update -> flicker_update->flicker_debug_info2_valid_size
									*/
	uint8 awb_debug_info2[MAX_AWB_DEBUG_SIZE_STRUCT2];	/* Update @ each report update
								 * AWB debug_info (Structure 2)  is "*awb_debug_data_full" in the structure "alAWBLib_output_data_t" from alAWBRuntimeObj_t-> estimation
								 */
	uint8 af_debug_info2[MAX_AF_DEBUG_SIZE_STRUCT2];	/* Collect @ finish focus
								 * alAFLib_output_report_t->alaf_debug_data[]
								 * alAFLib_output_report_t->alAF_debug_data_size
								 */
	struct otp_report_debug2 otp_report_debug_info2;
	struct irp_gamma_tone irp_tuning_para_debug_info2;	/* Included into S_ISP_DEBUG_INFO of ISP API: u32 ISPDrv_GetOutputBuffer(u8 a_ucSensor,
								 * u8 a_ucOutputImage, u32 a_pudOutputBuffer, S_SCENARIO_SIZE a_ptCropSize, S_ISP_DEBUG_INFO * a_ptIspDebugInfo)
								 */
	uint8 bin_file1[MAX_BIN1_DEBUG_SIZE_STRUCT2];	/* This is merged table data from tuning tool
							 * Include  AE Bin + AF Bin + AWB Bin File.
							 * Also refer to porting document
							 */

	uint8 bin_file2[MAX_BIN2_DEBUG_SIZE_STRUCT2];	/* This is merged table data from tuning tool
							 * Include  Shading + IRP Bin File.
							 * Also refer to porting document
							 */

	int8 end_string[END_STR_SIZE];  /* ie. end_jpeg_str_g2v1 */
};
#pragma pack(pop)  /* restore old alignment setting from stack  */

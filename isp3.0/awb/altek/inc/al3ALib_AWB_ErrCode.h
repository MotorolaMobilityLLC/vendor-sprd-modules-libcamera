#ifndef  _ALTEK_AWB_LIB_ERRORCODE_
#define _ALTEK_AWB_LIB_ERRORCODE_

typedef enum {
        ALTEK_AWB_SUCCESS               = (0x0000),

        ALTEK_AWB_ERROR_INVALID_INPUT   = (0x0100),
        ALTEK_AWB_ERROR_INVALID_STABLE_TYPE,
        ALTEK_AWB_ERROR_INVALID_STABLE_SIZE,
        ALTEK_AWB_ERROR_INVALID_MWB,
        ALTEK_AWB_ERROR_INVALID_TABLE_SIZE,
        ALTEK_AWB_ERROR_INVALID_CALIB_OTP,
        ALTEK_AWB_ERROR_INVALID_FLASH_PARA,

        ALTEK_AWB_ERROR_INVALID_PARAM   = (0x0200),
        ALTEK_AWB_ERROR_INVALID_TUNING_FILE,

        ALTEK_AWB_ERROR_INIT            = (0x0300),
        ALTEK_AWB_ERROR_INIT_NULL_OBJ,
        ALTEK_AWB_ERROR_INIT_BUFFER_NULL,
        ALTEK_AWB_ERROR_INIT_PRIVATE_MEM_NULL,
        ALTEK_AWB_ERROR_INIT_HANDLER_OVER_SIZE,
        ALTEK_AWB_ERROR_INIT_PRIVATE_MEM_NOTMATCH,
        ALTEK_AWB_ERROR_INIT_MEM_OVERLAP,
        ALTEK_AWB_ERROR_INIT_MEM_VERSION_WRONG,
        ALTEK_AWB_ERROR_INIT_MEM_FORMAT_WRONG,

        ALTEK_AWB_ERROR_FLOW            = (0x0400),
        ALTEK_AWB_ERROR_FLOW_RUNNING_LOCK,
        ALTEK_AWB_ERROR_FLOW_HANDLER_WRONG,

        ALTEK_AWB_ERROR_ESTIMATION      = (0x0500),
        ALTEK_AWB_ERROR_ESTIMATION_MEM_NULL,
        ALTEK_AWB_ERROR_ESTIMATION_TUNING_FILE_NULL,
        ALTEK_AWB_ERROR_ESTIMATION_STATS_FORMAT,
        ALTEK_AWB_ERROR_ESTIMATION_STATS_SIZE,

        ALTEK_AWB_ERROR_SET_PARAM       = (0x0600),
        ALTEK_AWB_ERROR_SET_PARAM_INPUT_NULL,
        ALTEK_AWB_ERROR_SET_PARAM_INPUT_TYPE,

        ALTEK_AWB_ERROR_GET_PARAM       = (0x0700),
        ALTEK_AWB_ERROR_GET_PARAM_INPUT_NULL,
        ALTEK_AWB_ERROR_GET_PARAM_INPUT_TYPE,

        ALTEK_AWB_ERROR_DEINIT          = (0x0800),
        ALTEK_AWB_ERROR_DEINIT_NULL_OBJ,
}   ALTEK_AWB_ERROR_CODE;

#endif

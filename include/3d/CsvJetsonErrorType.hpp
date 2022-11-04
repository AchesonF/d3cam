#ifndef CSV_JETSON_ERROR_TYPE_HPP__
#define CSV_JETSON_ERROR_TYPE_HPP__

#define CSV_OK							0
#define CSV_WARNING						-1
#define CSV_ERR_UNKNOWN					-2
#define CSV_ERR_IS_RUNNING				-3
#define CSV_ERR_READ_LUT_FILE			-6     // bad LUT	 file
#define CSV_ERR_LOAD_MODEL				-8    // load XML error
#define CSV_ERR_MODEL_NOT_READY			-10   // not load XML
#define CSV_ERR_NEW_MEMORY				-12    // failed malloc memory
#define CSV_ERR_LOAD_IMAGE				-14   // load image error
// gpu error
#define CSV_ERR_CHECK_GPU				-16   // gpu crash
#define CSV_ERR_RELOAD_GPU				-18   // reload gpu

#endif //CSV_JETSON_ERROR_TYPE_HPP__
#ifndef INCLUDE_PB_ERROR_H_
#define INCLUDE_PB_ERROR_H_

#include <stdint.h>

enum pb_results
{
    PB_RESULT_OK,
    PB_RESULT_ERROR,
    PB_RESULT_AUTHENTICATION_FAILED,
    PB_RESULT_NOT_AUTHENTICATED,
    PB_RESULT_NOT_SUPPORTED,
    PB_RESULT_INVALID_ARGUMENT,
    PB_RESULT_INVALID_COMMAND,
    PB_RESULT_PART_VERIFY_FAILED,
    PB_RESULT_PART_NOT_BOOTABLE,
    PB_RESULT_NO_MEMORY,
    PB_RESULT_TRANSFER_ERROR,
    PB_RESULT_NOT_FOUND,
    PB_RESULT_STREAM_NOT_INITIALIZED,
    PB_RESULT_END,              /* Sentinel */
};

const char *pb_error_string(enum pb_results result);

#endif  // INCLUDE_PB_ERROR_H_
/**
 * Punch BOOT
 *
 * Copyright (C) 2020 Jonas Blixt <jonpe960@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <string.h>
#include <pb-tools/wire.h>
#include <pb-tools/error.h>

int pb_wire_init_result(struct pb_result *result,
                                enum pb_results result_code)
{
    memset(result, 0, sizeof(*result));
    result->magic = PB_WIRE_MAGIC;
    result->result_code = (uint8_t) result_code;

    return PB_RESULT_OK;
}

int pb_wire_init_result2(struct pb_result *result,
                             enum pb_results result_code,
                             void *data,
                             size_t size)
{
    pb_wire_init_result(result, result_code);

    if (size > PB_RESULT_RESPONSE_MAX_SIZE)
        return -PB_RESULT_NO_MEMORY;

    memcpy(result->response, data, size);

    return PB_RESULT_OK;
}

const char *pb_wire_command_string(enum pb_commands cmd)
{
    return "";
}

const char *pb_wire_slc_string(enum pb_slc slc)
{
    return "";
}

bool pb_wire_requires_auth(struct pb_command *command)
{
    bool result = true;

    /* Commands that do not require authentication */
    switch (command->command)
    {
        case PB_CMD_DEVICE_IDENTIFIER_READ:
            result = false;
        break;
        case PB_CMD_AUTHENTICATE:
            result = false;
        break;
        case PB_CMD_AUTH_SET_OTP_PASSWORD:
            result = false;
        break;
        default:
        break;
    }

    return result;
}


bool pb_wire_valid_command(struct pb_command *command)
{
    if (command->magic != PB_WIRE_MAGIC)
        return false;

    bool valid_command = ((command->command > PB_CMD_INVALID) &&
            (command->command < PB_CMD_END));

    return valid_command;
}

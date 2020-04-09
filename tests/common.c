/**
 * Punch BOOT
 *
 * Copyright (C) 2018 Jonas Blixt <jonpe960@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <pb/board.h>
#include <pb/plat.h>
#include <plat/qemu/pl061.h>
#include <plat/qemu/gcov.h>
#include <plat/qemu/semihosting.h>
#include <pb/assert.h>
#include "test.h"

void __assert(const char *file, unsigned int line,
              const char *assertion)
{
    printf("Assert failed %s:%i (%s)\n\r", file, line, assertion);

    gcov_final();
    semihosting_sys_exit(1);

    while (1) {}
}


void pb_main(void)
{
    //test_uart_init();
    gcov_init();
    test_main();
    gcov_final();
    semihosting_sys_exit(0);
}

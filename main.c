/**
 * Punch BOOT
 *
 * Copyright (C) 2018 Jonas Blixt <jonpe960@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <pb/pb.h>
#include <pb/plat.h>
#include <pb/timing_report.h>
#include <pb/command.h>
#include <pb/storage.h>
#include <pb/transport.h>
#include <pb/console.h>
#include <pb/crypto.h>
#include <pb/board.h>
#include <pb/boot.h>

static struct pb_console console;
static struct pb_storage storage;
static struct pb_transport transport;
static struct pb_crypto crypto;
static struct pb_command cmd;
static struct pb_command_context command_ctx;
static struct pb_boot_context boot_ctx;
static struct pb_board board;
extern struct bpak_keystore keystore_pb;
static uint8_t device_uuid[16] __no_bss;

static int pb_early_init(void)
{
    int rc;

    tr_init();

    rc = pb_console_init(&console);

    if (rc != PB_OK)
        return rc;

    rc = pb_storage_init(&storage);

    if (rc != PB_OK)
        return rc;

    rc = pb_transport_init(&transport, device_uuid);

    if (rc != PB_OK)
        return rc;

    rc = pb_crypto_init(&crypto);

    if (rc != PB_OK)
        return rc;

    rc = pb_command_init(&command_ctx, &transport, &storage, &crypto, &board,
                            &keystore_pb, &boot_ctx, device_uuid);

    if (rc != PB_OK)
        return rc;

    rc = plat_early_init(&storage, &transport, &console, &crypto, &command_ctx,
                         &boot_ctx, &keystore_pb, &board);

    if (rc != PB_OK)
        return rc;

    rc = pb_console_start(&console);

    if (rc != PB_OK)
        return rc;

    return PB_OK;
}

int putchar(int c)
{
    if (console.driver->ready)
    {
        console.driver->write(console.driver, (char *) &c, 1);
    }

    return c;
}

void pb_main(void)
{
    int rc;

    rc = pb_early_init();

    if (rc != PB_OK)
        plat_reset();

    tr_stamp_begin(TR_BLINIT);
    tr_stamp_begin(TR_TOTAL);

#if LOGLEVEL > 0
    printf("\n\r\n\rPB " PB_VERSION " starting\n\r");
#endif

    rc = pb_crypto_start(&crypto);

    if (rc != PB_OK)
    {
        LOG_ERR("Could not initialize crypto");
        goto run_command_mode;
    }

    rc = plat_get_uuid(&crypto, (char *) device_uuid);

    if (rc != PB_OK)
    {
        LOG_ERR("Could not read device UUID");
        goto run_command_mode;
    }

    rc = pb_storage_start(&storage);

    if (rc != PB_OK)
    {
        LOG_ERR("Could not initialize storage");
        goto run_command_mode;
    }

    rc = pb_boot_load_state(&boot_ctx);

    if (rc != PB_OK)
    {
        LOG_ERR("Could not load state");
        goto run_command_mode;
    }

    if (board.pre_boot)
    {
        rc = board.pre_boot(&board);

        if (rc != PB_OK)
        {
            LOG_ERR("Board pre_boot failed");
            goto run_command_mode;
        }
    }

    if (board.force_command_mode)
    {
        LOG_INFO("Forced command mode");
        goto run_command_mode;
    }

    rc = pb_boot_load_fs(&boot_ctx, NULL);

    if (rc != PB_OK)
    {
        LOG_ERR("Could not boot");
        goto run_command_mode;
    }

    pb_boot(&boot_ctx, device_uuid, false);

    LOG_ERR("Boot failed");

run_command_mode:
    LOG_DBG("Starting transport");

    rc = pb_transport_start(&transport, 10);

    if (rc != PB_OK)
    {
        LOG_ERR("Transport init err");
        plat_reset();
    }

    LOG_DBG("Transport init done");

    while (true)
    {
        rc = pb_transport_read(&transport, &cmd, sizeof(cmd));

        if (rc != PB_OK)
        {
            LOG_ERR("Read error %i", rc);
            goto run_command_mode;
        }

        rc = pb_command_parse(&command_ctx, &cmd);

        if (rc != PB_OK)
        {
            LOG_ERR("Command error %i", rc);
        }
    }
}
/**
 * Punch BOOT
 *
 * Copyright (C) 2020 Jonas Blixt <jonpe960@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <string.h>
#include <pb/board.h>
#include <pb/io.h>
#include <pb/plat.h>
#include <pb/crypto.h>
#include <uuid/uuid.h>
#include <pb/fuse.h>
#include <pb/board.h>
#include <pb/boot.h>
#include <plat/qemu/plat.h>
#include <plat/qemu/virtio.h>
#include <plat/qemu/virtio_block.h>
#include <plat/qemu/virtio_serial.h>
#include <plat/qemu/fuse.h>
#include <plat/qemu/gcov.h>
#include <plat/qemu/uart.h>
#include <bearssl/bearssl_hash.h>
#include <plat/qemu/semihosting.h>
#include <bpak/bpak.h>

static struct qemu_uart_device console_uart;
extern const struct fuse fuses[];
extern const uint32_t rom_key_map[];

static struct fuse rom_key_revoke_fuse =
        TEST_FUSE_BANK_WORD(8, "Revoke");

static struct fuse security_fuse =
        TEST_FUSE_BANK_WORD(9, "Security fuse");

static struct pb_result_slc_key_status key_status;

static const char *platform_namespace_uuid =
    "\x3f\xaf\xc6\xd3\xc3\x42\x4e\xdf\xa5\xa6\x0e\xb1\x39\xa7\x83\xb5";

static const char *device_unique_id =
    "\xbe\x4e\xfc\xb4\x32\x58\xcd\x63";


void *plat_get_private(void)
{
    return NULL;
}

int plat_get_uuid(char *out)
{
    int rc;
    char device_uu_str[37];

    rc = uuid_gen_uuid3(platform_namespace_uuid,
                          (const char *) device_unique_id, 8, out);

    uuid_unparse((const unsigned char *) out, device_uu_str);
    LOG_DBG("Get UUID %s", device_uu_str);

    return rc;
}

int plat_fuse_read(struct fuse *f)
{
    uint32_t tmp_val = 0;
    int err;

    if ( (f->status & FUSE_VALID) != FUSE_VALID)
        return -PB_ERR;

    err = qemu_fuse_read(f->bank, &tmp_val);

    if (err != PB_OK)
        LOG_ERR("Could not read fuse");

    f->value = tmp_val;

    return err;
}

int plat_fuse_write(struct fuse *f)
{
    int err;

    if ( (f->status & FUSE_VALID) != FUSE_VALID)
        return -PB_ERR;
    LOG_DBG("Writing fuse %s", f->description);

    err = qemu_fuse_write(f->bank, f->value);

    if (err != PB_OK)
        LOG_ERR("Could not write fuse");
    return err;
}

int plat_fuse_to_string(struct fuse *f, char *s, uint32_t n)
{
    if ( (f->status & FUSE_VALID) != FUSE_VALID)
        return -PB_ERR;

    return snprintf(s, n, "FUSE <%u> %s = 0x%08x\n", f->bank,
                f->description, f->value);
}

/* QEMU SLC Interface */
int plat_slc_set_configuration(void)
{
    int err;

    /* Read fuses */
    foreach_fuse(f, (struct fuse *) fuses)
    {
        err = plat_fuse_read(f);

        LOG_DBG("Fuse %s: 0x%08x", f->description, f->value);
        if (err != PB_OK)
        {
            LOG_ERR("Could not access fuse '%s'", f->description);
            return err;
        }
    }

    /* Perform the actual fuse programming */

    LOG_INFO("Writing fuses");

    foreach_fuse(f, fuses)
    {
        f->value = f->default_value;
        err = plat_fuse_write(f);

        if (err != PB_OK)
            return err;
    }

#ifdef CONFIG_CALL_BOARD_SLC_SET_CONFIGURATION
    return board_slc_set_configuration(&private);
#else
    return PB_OK;
#endif
}

int plat_slc_set_configuration_lock(void)
{
    int rc;

    rc = plat_fuse_read(&security_fuse);

    if (rc != PB_OK)
        return rc;

    if (security_fuse.value & (1 << 0))
    {
        LOG_ERR("Already locked");
        return -PB_ERR;
    }

    security_fuse.value |= (1 << 0);

    rc = plat_fuse_write(&security_fuse);

    return rc;
}

int plat_slc_set_end_of_life(void)
{

    int rc;

    rc = plat_fuse_read(&security_fuse);

    if (rc != PB_OK)
        return rc;

    security_fuse.value |= (1 << 1);

    rc = plat_fuse_write(&security_fuse);

    return rc;
}

int plat_slc_read(enum pb_slc *slc)
{

    int rc;

    rc = plat_fuse_read(&security_fuse);

    if (rc != PB_OK)
        return rc;

    if (security_fuse.value & (1 << 0))
    {
        LOG_INFO("SLC: Configuration locked");
        *slc = PB_SLC_CONFIGURATION_LOCKED;
    }
    else if (security_fuse.value & (1 << 1))
    {
        LOG_INFO("SLC: End of life");
        *slc = PB_SLC_EOL;
    }
    else
    {
        LOG_INFO("SLC: Configuration");
        *slc = PB_SLC_CONFIGURATION;
    }

    return PB_OK;
}

int plat_slc_key_active(uint32_t id, bool *active)
{
    int rc;
    *active = false;
    rc = plat_slc_get_key_status(NULL);

    if (rc != PB_OK)
        return rc;

    for (int i = 0; i < 16; i++)
    {
        if (!key_status.active[i])
            continue;
        if (key_status.active[i] == id)
        {
            *active = true;
            break;
        }
    }

    return PB_OK;
}

int plat_slc_revoke_key(uint32_t id)
{
    int rc;
    LOG_INFO("Revoking key 0x%x", id);

    rc = plat_slc_get_key_status(NULL);

    if (rc != PB_OK)
        return rc;

    rc = plat_fuse_read(&rom_key_revoke_fuse);

    if (rc != PB_OK)
        return rc;

    for (int i = 0; i < 16; i++)
    {
        if (!rom_key_map[i])
            break;
        if (rom_key_map[i] != id)
            continue;

        if (!(rom_key_revoke_fuse.value & (1 << i)))
        {
            rom_key_revoke_fuse.value |= (1 << i);
            return plat_fuse_write(&rom_key_revoke_fuse);
        }
    }
    return PB_OK;
}

int plat_slc_init(void)
{
    int rc;
    rc = qemu_fuse_init();
    return rc;
}

int plat_slc_get_key_status(struct pb_result_slc_key_status **status)
{
    int rc;

    if (status)
        (*status) = &key_status;

    rc = plat_fuse_read(&rom_key_revoke_fuse);

    if (rc != PB_OK)
        return rc;

    for (int i = 0; i < 16; i++)
    {
        if (!rom_key_map[i])
            break;

        if (rom_key_revoke_fuse.value & (1 << i))
        {
            key_status.active[i] = 0;
            key_status.revoked[i] = rom_key_map[i];
        }
        else
        {
            key_status.revoked[i] = 0;
            key_status.active[i] = rom_key_map[i];
        }
    }

    return PB_OK;
}

int plat_early_init(void)
{
    int rc;
    memset(&key_status, 0, sizeof(key_status));
    rc = board_early_init(NULL);

#ifdef CONFIG_QEMU_ENABLE_TEST_COVERAGE
    gcov_init();
#endif

    return rc;
}

/* Console API */

int plat_console_init(void)
{
    return pb_qemu_console_init(&console_uart);
}

int plat_console_putchar(char c)
{
    qemu_uart_write(&console_uart, (char *) &c, 1);
    return PB_OK;
}

bool plat_force_command_mode(void)
{
    return board_force_command_mode(NULL);
}

int plat_boot_override(uint8_t *uuid)
{
#ifdef CONFIG_OVERRIDE_ARCH_JUMP
    return board_boot_override(NULL, uuid);
#else
    return PB_OK;
#endif
}

int plat_status(void *response_bfr,
                    size_t *response_size)
{
    return board_status(NULL, response_bfr, response_size);
}

int plat_command(uint32_t command,
                     void *bfr,
                     size_t size,
                     void *response_bfr,
                     size_t *response_size)
{
    return board_command(NULL, command, bfr, size,
                            response_bfr, response_size);
}

#ifdef CONFIG_CALL_EARLY_PLAT_BOOT
int plat_early_boot(void)
{
    return board_early_boot(NULL);
}

int plat_late_boot(bool *abort_boot, bool manual)
{
    return board_late_boot(NULL, abort_boot, manual);
}
#endif

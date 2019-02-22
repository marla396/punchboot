#include <stdio.h>
#include <pb.h>
#include <uuid.h>

typedef union 
{
    uint8_t raw[16];
    struct
    {
        uint32_t time_low;
        uint16_t time_mid;
        uint16_t time_hi_and_version;
        uint8_t clock_seq_hi_and_res;
        uint8_t clock_seq_low;
        uint8_t node[6];
    } uuid __packed;
} uuid_t;


uint32_t uuid_to_string(uint8_t *uuid, char *out)
{
    uuid_t *u = (uuid_t *) uuid;

    snprintf(out,37, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        u->uuid.time_low,
        u->uuid.time_mid,
        u->uuid.time_hi_and_version,
        u->uuid.clock_seq_hi_and_res,
        u->uuid.clock_seq_low,
        u->uuid.node[0],
        u->uuid.node[1],
        u->uuid.node[2],
        u->uuid.node[3],
        u->uuid.node[4],
        u->uuid.node[5]);

    

    return PB_OK;
}


uint32_t uuid3_to_string(uint8_t *uuid, char *out)
{
    uint32_t *u0 = (uint32_t *) &uuid[0];
    uint16_t *u1 = (uint16_t *) &uuid[4];
    uint16_t *u2 = (uint16_t *) &uuid[6];
    uint16_t *u3 = (uint16_t *) &uuid[8];
    uint16_t *u4 = (uint16_t *) &uuid[10];
    uint32_t *u5 = (uint32_t *) &uuid[12];

    *u0 = ((*u0 >> 24) & 0xff) |
          ((*u0 << 8)  & 0xff0000) |
          ((*u0 >> 8)  & 0xff00) |
          ((*u0 << 24) & 0xff000000);


    *u1 = (*u1 >> 8) | (*u1 << 8);
    *u2 = (*u2 >> 8) | (*u2 << 8);
    *u3 = (*u3 >> 8) | (*u3 << 8);
    *u4 = (*u4 >> 8) | (*u4 << 8);

    *u5 = ((*u5 >> 24) & 0xff) |
          ((*u5 << 8)  & 0xff0000) |
          ((*u5 >> 8)  & 0xff00) |
          ((*u5 << 24) & 0xff000000);

    snprintf(out,37, "%08x-%04x-%04x-%04x-%04x%08x",
                *u0,*u1,*u2,*u3,*u4,*u5);

    

    return PB_OK;
}

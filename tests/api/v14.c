/* API v14: the firmware image fields appended to cogmaster_media, with the three version numbers.
   The rest of the layout does not move: every other struct size is pinned again here. Every name
   the version adds is used here. */
#include <stddef.h>
#include <stdint.h>
#include "cogmaster.h"
#include "../check.h"

int main(void)
{
    CHECK_EQ_U64(COGMASTER_API_VERSION, 14u);
    CHECK_EQ_U64(COGMASTER_ABI_VERSION, 1u);
    CHECK(COGMASTER_BEHAVIOUR_REVISION >= 16u);
    CHECK_EQ_U64(COGMASTER_STATE_FORMAT_VERSION, 17u);

    cogmaster_versions v = {0};
    v.size = (uint32_t)sizeof v;
    CHECK(cogmaster_get_versions(&v) == COGMASTER_STATUS_OK);
    CHECK_EQ_U64(v.api, 14u);
    CHECK(v.behaviour >= 16u);
    CHECK_EQ_U64(v.state, 17u);

    const size_t p = sizeof(void *);
    CHECK_EQ_U64(offsetof(cogmaster_media, video_mode), p == 8u ? 40u : 24u);
    CHECK_EQ_U64(offsetof(cogmaster_media, fm), p == 8u ? 48u : 28u);
    CHECK_EQ_U64(offsetof(cogmaster_media, bios), p == 8u ? 56u : 32u);
    CHECK_EQ_U64(offsetof(cogmaster_media, bios_size), p == 8u ? 64u : 36u);
    CHECK_EQ_U64(sizeof(cogmaster_media), p == 8u ? 72u : 40u);
#if UINTPTR_MAX > UINT32_MAX
    cogmaster_media pad = {0};
    CHECK_EQ_U64(offsetof(cogmaster_media, reserved3), 52u);
    CHECK_EQ_U64(pad.reserved3, 0u);
#endif

    CHECK_EQ_U64(sizeof(cogmaster_input), 48u);
    CHECK_EQ_U64(sizeof(cogmaster_run_result), 24u);
    CHECK_EQ_U64(sizeof(cogmaster_info), 76u);
    CHECK_EQ_U64(sizeof(cogmaster_cart_info), 536u);
    CHECK_DONE();
}

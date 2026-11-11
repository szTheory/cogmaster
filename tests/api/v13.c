/* API v13: the light phaser device constant and the three version numbers. The layout does not
   move: every struct size and axis offset is pinned again here. Every name the version adds is
   used here. */
#include <stddef.h>
#include "cogmaster.h"
#include "../check.h"

int main(void)
{
    CHECK_EQ_U64(COGMASTER_API_VERSION, 13u);
    CHECK_EQ_U64(COGMASTER_ABI_VERSION, 1u);
    CHECK_EQ_U64(COGMASTER_BEHAVIOUR_REVISION, 15u);
    CHECK_EQ_U64(COGMASTER_STATE_FORMAT_VERSION, 15u);

    cogmaster_versions v = {0};
    v.size = (uint32_t)sizeof v;
    CHECK(cogmaster_get_versions(&v) == COGMASTER_STATUS_OK);
    CHECK_EQ_U64(v.api, 13u);
    CHECK_EQ_U64(v.behaviour, 15u);
    CHECK_EQ_U64(v.state, 15u);

    CHECK_EQ_U64(COGMASTER_DEVICE_PAD, 0u);
    CHECK_EQ_U64(COGMASTER_DEVICE_NONE, 1u);
    CHECK_EQ_U64(COGMASTER_DEVICE_PADDLE, 2u);
    CHECK_EQ_U64(COGMASTER_DEVICE_SPORTS_PAD, 3u);
    CHECK_EQ_U64(COGMASTER_DEVICE_LIGHT_PHASER, 4u);

    CHECK_EQ_U64(sizeof(cogmaster_input), 48u);
    CHECK_EQ_U64(offsetof(cogmaster_input, x1), 32u);
    CHECK_EQ_U64(offsetof(cogmaster_input, y1), 36u);
    CHECK_EQ_U64(offsetof(cogmaster_input, x2), 40u);
    CHECK_EQ_U64(offsetof(cogmaster_input, y2), 44u);
    CHECK_EQ_U64(sizeof(cogmaster_run_result), 24u);
    CHECK_EQ_U64(sizeof(cogmaster_info), 76u);
    CHECK_EQ_U64(sizeof(cogmaster_cart_info), 536u);
    CHECK_DONE();
}

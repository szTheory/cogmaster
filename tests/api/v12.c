/* API v12: the controller devices, the axis fields of cogmaster_input, the eye reported in
   cogmaster_run_result and the default devices in cogmaster_info and cogmaster_cart_info, with the
   three version numbers. Every name the version adds is used here. */
#include <stddef.h>
#include "cogmaster.h"
#include "../check.h"

int main(void)
{
    CHECK_EQ_U64(COGMASTER_API_VERSION, 12u);
    CHECK_EQ_U64(COGMASTER_ABI_VERSION, 1u);
    CHECK_EQ_U64(COGMASTER_BEHAVIOUR_REVISION, 14u);
    CHECK_EQ_U64(COGMASTER_STATE_FORMAT_VERSION, 14u);

    cogmaster_versions v = {0};
    v.size = (uint32_t)sizeof v;
    CHECK(cogmaster_get_versions(&v) == COGMASTER_STATUS_OK);
    CHECK_EQ_U64(v.api, 12u);
    CHECK_EQ_U64(v.behaviour, 14u);
    CHECK_EQ_U64(v.state, 14u);

    CHECK_EQ_U64(COGMASTER_DEVICE_PAD, 0u);
    CHECK_EQ_U64(COGMASTER_DEVICE_NONE, 1u);
    CHECK_EQ_U64(COGMASTER_DEVICE_PADDLE, 2u);
    CHECK_EQ_U64(COGMASTER_DEVICE_SPORTS_PAD, 3u);
    CHECK_EQ_U64(COGMASTER_GLASSES_NONE, 0u);
    CHECK_EQ_U64(COGMASTER_GLASSES_RIGHT, 1u);
    CHECK_EQ_U64(COGMASTER_GLASSES_LEFT, 2u);

    CHECK_EQ_U64(sizeof(cogmaster_input), 48u);
    CHECK_EQ_U64(offsetof(cogmaster_input, port2), 28u);
    CHECK_EQ_U64(offsetof(cogmaster_input, x1), 32u);
    CHECK_EQ_U64(offsetof(cogmaster_input, y1), 36u);
    CHECK_EQ_U64(offsetof(cogmaster_input, x2), 40u);
    CHECK_EQ_U64(offsetof(cogmaster_input, y2), 44u);

    CHECK_EQ_U64(sizeof(cogmaster_run_result), 24u);
    CHECK_EQ_U64(offsetof(cogmaster_run_result, display_enabled), 16u);
    CHECK_EQ_U64(offsetof(cogmaster_run_result, glasses), 20u);

    CHECK_EQ_U64(sizeof(cogmaster_info), 76u);
    CHECK_EQ_U64(offsetof(cogmaster_info, fm), 64u);
    CHECK_EQ_U64(offsetof(cogmaster_info, default_port1), 68u);
    CHECK_EQ_U64(offsetof(cogmaster_info, default_port2), 72u);

    CHECK_EQ_U64(sizeof(cogmaster_cart_info), 536u);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, unsupported_reason), 400u);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, default_port1), 528u);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, default_port2), 532u);
    CHECK_DONE();
}

/* API v5 exposes explicit video mode without changing previous input tags. */
#include <stddef.h>
#include "cogmaster.h"
#include "../check.h"
int main(void)
{
    CHECK(COGMASTER_API_VERSION >= 5);
    CHECK_EQ_U64(COGMASTER_VIDEO_AUTO, 0u);
    CHECK_EQ_U64(COGMASTER_VIDEO_SMS, 1u);
    CHECK_EQ_U64(COGMASTER_VIDEO_GG, 2u);
    CHECK_EQ_U64(offsetof(cogmaster_media, video_mode), sizeof(void *) == 8u ? 40u : 24u);
    CHECK_EQ_U64(offsetof(cogmaster_info, video_mode), 56u);
#if UINTPTR_MAX > UINT32_MAX
    cogmaster_media media = {0};
    CHECK_EQ_U64(media.reserved, 0u);
#endif
    CHECK_DONE();
}

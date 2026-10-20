/* API v7 adds transactional raw battery import. */
#include "cogmaster.h"
#include "../check.h"
int main(void)
{
    CHECK(COGMASTER_API_VERSION >= 7);
    CHECK_EQ_U64(cogmaster_import_battery(NULL, NULL, 0u), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_EQ_U64(cogmaster_import_battery(NULL, NULL, SIZE_MAX), COGMASTER_STATUS_ERR_ARGUMENT);
    CHECK_DONE();
}

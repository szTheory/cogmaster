/* API v10: the Korean-board constants and the board-source fields of cogmaster_cart_info.
   Every name the version adds is used here. */
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "cogmaster.h"
#include "../check.h"

#define V9_CART_INFO_SIZE 376u
#define IMAGE_SIZE 32768u

int main(void)
{
    CHECK(COGMASTER_API_VERSION >= 10);
    CHECK_EQ_U64(COGMASTER_ABI_VERSION, 1u);
    CHECK(COGMASTER_BEHAVIOUR_REVISION >= 12);
    CHECK(COGMASTER_STATE_FORMAT_VERSION >= 12);

    CHECK_EQ_U64(COGMASTER_BOARD_ROM_ONLY, 1u);
    CHECK_EQ_U64(COGMASTER_BOARD_SEGA, 2u);
    CHECK_EQ_U64(COGMASTER_BOARD_CODEMASTERS, 3u);
    CHECK_EQ_U64(COGMASTER_BOARD_KOREAN, 4u);
    CHECK_EQ_U64(COGMASTER_BOARD_KOREAN_NB, 5u);
    CHECK_EQ_U64(COGMASTER_BOARD_HICOM, 6u);
    CHECK_EQ_U64(COGMASTER_BOARD_ZEMINA, 7u);
    CHECK_EQ_U64(COGMASTER_BOARD_NEMESIS, 8u);
    CHECK_EQ_U64(COGMASTER_BOARD_4PAK, 9u);
    CHECK_EQ_U64(COGMASTER_BOARD_SOURCE_DB, 1u);
    CHECK_EQ_U64(COGMASTER_BOARD_SOURCE_HEADER, 2u);
    CHECK_EQ_U64(COGMASTER_BOARD_SOURCE_HEURISTIC, 3u);
    CHECK_EQ_U64(COGMASTER_BOARD_SOURCE_DEFAULT, 4u);

    /* The version 9 struct ends at 376 on every target; the appended fields follow it. */
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, board_source), V9_CART_INFO_SIZE);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, board_candidate), V9_CART_INFO_SIZE + 4u);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, scan_sega), V9_CART_INFO_SIZE + 8u);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, scan_candidate), V9_CART_INFO_SIZE + 12u);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, scan_runner_up), V9_CART_INFO_SIZE + 16u);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, unsupported_id), V9_CART_INFO_SIZE + 20u);
    CHECK_EQ_U64(offsetof(cogmaster_cart_info, unsupported_reason), V9_CART_INFO_SIZE + 24u);
    CHECK(sizeof(cogmaster_cart_info) >= V9_CART_INFO_SIZE + 6u * 4u + 128u);

    static uint8_t image[IMAGE_SIZE];
    cogmaster_cart_info old;
    memset(&old, 0xA5, sizeof old);
    old.size = V9_CART_INFO_SIZE;
    cogmaster_status st = cogmaster_probe(image, sizeof image, &old);
    CHECK(st == COGMASTER_STATUS_OK || st == COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD);
    CHECK_EQ_U64(old.image_size, IMAGE_SIZE);
    CHECK_EQ_U64(old.board_source, 0xA5A5A5A5u);
    CHECK_EQ_U64(old.board_candidate, 0xA5A5A5A5u);
    CHECK_EQ_U64(old.unsupported_id, 0xA5A5A5A5u);
    CHECK_EQ_U64((unsigned char)old.unsupported_reason[0], 0xA5u);
    CHECK_EQ_U64((unsigned char)old.unsupported_reason[127], 0xA5u);

    cogmaster_cart_info now;
    memset(&now, 0xA5, sizeof now);
    now.size = sizeof now;
    st = cogmaster_probe(image, sizeof image, &now);
    CHECK(st == COGMASTER_STATUS_OK || st == COGMASTER_STATUS_ERR_UNSUPPORTED_BOARD);
    CHECK(now.board_source <= COGMASTER_BOARD_SOURCE_DEFAULT);
    CHECK(now.board_candidate <= COGMASTER_BOARD_4PAK);
    CHECK_EQ_U64(now.scan_sega, 0u);
    CHECK_EQ_U64(now.scan_candidate, 0u);
    CHECK_EQ_U64(now.scan_runner_up, 0u);
    CHECK_EQ_U64(now.unsupported_id, 0u);
    CHECK(memchr(now.unsupported_reason, 0, sizeof now.unsupported_reason) != NULL);
    CHECK_DONE();
}

// The public header alone, compiled as C++17 with warnings fatal. Compiling
// this file is the test; nothing links it.
#include "cogmaster.h"

// Every boundary struct starts with its size tag.
static_assert(offsetof(cogmaster_config, size) == 0, "config size first");
static_assert(offsetof(cogmaster_versions, size) == 0, "versions size first");
static_assert(offsetof(cogmaster_media, size) == 0, "media size first");
static_assert(offsetof(cogmaster_info, size) == 0, "info size first");
static_assert(offsetof(cogmaster_frame, size) == 0, "frame size first");
static_assert(offsetof(cogmaster_run_result, size) == 0, "run result size first");
static_assert(offsetof(cogmaster_fault, size) == 0, "fault size first");
static_assert(sizeof(cogmaster_allocator) >= 3 * sizeof(void *), "allocator holds three pointers");
static_assert(sizeof(cogmaster_status) == 4, "status is 32 bits");
static_assert(COGMASTER_API_VERSION >= 1, "API version");

// Each function keeps its declared signature, with C linkage.
static cogmaster_status (*const get_versions_fn)(cogmaster_versions *) = cogmaster_get_versions;
static cogmaster_status (*const create_fn)(const cogmaster_config *,
                                           cogmaster **) = cogmaster_create;
static void (*const destroy_fn)(cogmaster *) = cogmaster_destroy;
static cogmaster_status (*const load_fn)(cogmaster *, const cogmaster_media *) = cogmaster_load;
static cogmaster_status (*const run_frame_fn)(cogmaster *,
                                              cogmaster_run_result *) = cogmaster_run_frame;
static cogmaster_status (*const get_frame_fn)(const cogmaster *,
                                              cogmaster_frame *) = cogmaster_get_frame;
static cogmaster_status (*const get_info_fn)(const cogmaster *,
                                             cogmaster_info *) = cogmaster_get_info;
static cogmaster_status (*const reset_fn)(cogmaster *, uint32_t) = cogmaster_reset;
static cogmaster_status (*const get_fault_fn)(const cogmaster *,
                                              cogmaster_fault *) = cogmaster_get_fault;

int header_check(cogmaster_status st);

int header_check(cogmaster_status st)
{
    // A C++ caller zeroes a struct with empty braces, then sets its size.
    cogmaster_frame io = {};
    io.size = static_cast<uint32_t>(sizeof io);
    (void)get_versions_fn;
    (void)create_fn;
    (void)destroy_fn;
    (void)load_fn;
    (void)run_frame_fn;
    (void)get_frame_fn;
    (void)get_info_fn;
    (void)reset_fn;
    (void)get_fault_fn;
    // A status compares against the constants without a sign or enum warning.
    return (st == COGMASTER_STATUS_OK || st == COGMASTER_STATUS_ERR_BUFFER_TOO_SMALL) &&
           io.flags == 0u;
}

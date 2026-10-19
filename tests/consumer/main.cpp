// A C++ program that includes the installed header and calls the library.
// Exits 0 and prints one fixed line on success, 1 otherwise.
#include <cogmaster.h>
#include <cstdio>
#include <cstring>

int main()
{
    cogmaster_versions v;
    std::memset(&v, 0, sizeof v);
    v.size = static_cast<uint32_t>(sizeof v);
    if (cogmaster_get_versions(&v) != COGMASTER_STATUS_OK) {
        std::printf("cogmaster_get_versions failed\n");
        return 1;
    }
    std::printf("consumer_cxx: api %u\n", static_cast<unsigned>(v.api));
    return 0;
}

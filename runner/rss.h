/* The runner's peak resident memory for --bench. Runner-only. */
#ifndef COGMASTER_RUNNER_RSS_H
#define COGMASTER_RUNNER_RSS_H

#include <stdint.h>

/* The process's peak resident set so far in KiB, rounded up; 0 where the
   platform does not report it (WASI) or the call fails. */
uint64_t cogmaster__max_rss_kib(void);

#endif /* COGMASTER_RUNNER_RSS_H */

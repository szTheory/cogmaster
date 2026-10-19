/* The runner's stop on SIGINT or SIGTERM. Runner-only: the core never sees
   a signal. */
#ifndef COGMASTER_RUNNER_STOP_H
#define COGMASTER_RUNNER_STOP_H

/* Installs the handlers. Call once, after the command line is valid. */
void cogmaster__stop_install(void);

/* 1 once SIGINT or SIGTERM has arrived, else 0. */
int cogmaster__stop_requested(void);

#endif /* COGMASTER_RUNNER_STOP_H */

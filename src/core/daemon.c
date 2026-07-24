#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "daemon.h"
#include "timesync.h"

static volatile sig_atomic_t g_shutdown_requested = 0;

static void signal_handler(int signum) {
    (void)signum;
    g_shutdown_requested = 1;
}

static int setup_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Failed to setup SIGTERM handler: %s", strerror(errno));
        return -1;
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Failed to setup SIGINT handler: %s", strerror(errno));
        return -1;
    }
    return 0;
}

static int interruptible_sleep(int seconds) {
    struct timespec req, rem;
    req.tv_sec = seconds;
    req.tv_nsec = 0;
    
    while (req.tv_sec > 0 && !g_shutdown_requested) {
        if (nanosleep(&req, &rem) == -1) {
            if (errno == EINTR) {
                if (g_shutdown_requested) return -1;
                req = rem;
            } else {
                syslog(LOG_WARNING, "nanosleep failed: %s", strerror(errno));
                break;
            }
        } else {
            break;
        }
    }
    return g_shutdown_requested ? -1 : 0;
}

int daemon_run(int interval_seconds) {
    int ret = 0;
    char timestamp[20];
    int sync_count = 0;
    int fail_count = 0;

    if (interval_seconds < DAEMON_MIN_INTERVAL) {
        syslog(LOG_WARNING, "Interval %d too low, using minimum %d seconds", interval_seconds, DAEMON_MIN_INTERVAL);
        interval_seconds = DAEMON_MIN_INTERVAL;
    } else if (interval_seconds > DAEMON_MAX_INTERVAL) {
        syslog(LOG_WARNING, "Interval %d too high, using maximum %d seconds", interval_seconds, DAEMON_MAX_INTERVAL);
        interval_seconds = DAEMON_MAX_INTERVAL;
    }

    openlog("gmk67sts", LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "GMK67STS daemon starting (interval: %d seconds)", interval_seconds);

    if (setup_signal_handlers() != 0) { ret = 1; goto cleanup; }

    while (!g_shutdown_requested) {
        time_t now = time(NULL);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

        int sync_result = timesync_sync(false);

        if (sync_result == 0) {
            sync_count++;
            syslog(LOG_INFO, "[%s] Time synchronized (count: %d)", timestamp, sync_count);
        } else if (sync_result == 1) {
            fail_count++;
            syslog(LOG_WARNING, "[%s] Keyboard not found (failures: %d). Retrying in %d seconds", timestamp, fail_count, interval_seconds);
        } else {
            fail_count++;
            syslog(LOG_ERR, "[%s] Sync failed with error %d (failures: %d)", timestamp, sync_result, fail_count);
        }

        if (g_shutdown_requested) break;

        if (interruptible_sleep(interval_seconds) != 0) break;
    }

    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    syslog(LOG_INFO, "[%s] GMK67STS daemon shutting down (synced: %d, failures: %d)", timestamp, sync_count, fail_count);

cleanup:
    closelog();
    return ret;
}
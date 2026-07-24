#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <strings.h>

#include "../core/timesync.h"
#include "../core/daemon.h"
#include "../core/profiles.h"
#include "../core/config.h"
#include "../core/device.h"

#define VERSION "1.5.1"

static void print_usage(const char* prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("\n");
    printf("GMK67STS - Time synchronization and lighting control for GMK67 keyboard\n");
    printf("\n");
    printf("Options:\n");
    printf("  -s, --sync             Sync current system time to keyboard\n");
    printf("  -d, --daemon           Run as daemon with periodic sync\n");
    printf("  -i, --interval SEC     Sync interval (default: %d, min: %d, max: %d)\n",
           DAEMON_DEFAULT_INTERVAL, DAEMON_MIN_INTERVAL, DAEMON_MAX_INTERVAL);
    printf("  -r, --restore-defaults Restore factory default config (from saved dump)\n");
    printf("  -l, --load-profile NAME Load lighting profile (see --list-profiles)\n");
    printf("      --list-profiles    List available lighting profiles\n");
    printf("      --dump             Read-only: dump current config as C array (no writes)\n");
    printf("  -h, --help             Show this help message\n");
    printf("  -v, --version          Show version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --sync                    Sync time once\n", prog);
    printf("  %s --restore-defaults        Restore saved factory config\n", prog);
    printf("  %s --load-profile gaming     Load gaming profile\n", prog);
    printf("  %s --daemon                  Run as daemon (12-hour interval)\n", prog);
    printf("  %s -d -i 3600                Daemon with 1-hour interval\n", prog);
    printf("  %s --dump                    Dump current config (read-only)\n", prog);
    printf("\n");
    printf("Note: May require sudo or proper udev rules for USB access.\n");
}

static void print_version(void) {
    printf("gmk67sts version %s\n", VERSION);
}

int main(int argc, char* argv[]) {
    static struct option long_options[] = {
        {"sync",             no_argument,       NULL, 's'},
        {"daemon",           no_argument,       NULL, 'd'},
        {"interval",         required_argument, NULL, 'i'},
        {"restore-defaults", no_argument,       NULL, 'r'},
        {"load-profile",     required_argument, NULL, 'l'},
        {"list-profiles",    no_argument,       NULL, 'L'},
        {"dump",             no_argument,       NULL, 'D'},
        {"help",             no_argument,       NULL, 'h'},
        {"version",          no_argument,       NULL, 'v'},
        {NULL,               0,                 NULL, 0}
    };

    int opt;
    bool do_sync = false;
    bool do_daemon = false;
    bool do_restore = false;
    bool do_list = false;
    bool do_dump = false;
    const char* load_profile_name = NULL;
    int interval = DAEMON_DEFAULT_INTERVAL;

    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    while ((opt = getopt_long(argc, argv, "sdi:rl:hv", long_options, NULL)) != -1) {
        switch (opt) {
            case 's': do_sync = true; break;
            case 'd': do_daemon = true; break;
            case 'i':
                interval = atoi(optarg);
                if (interval <= 0) {
                    fprintf(stderr, "Error: Invalid interval '%s'\n", optarg);
                    return EXIT_FAILURE;
                }
                break;
            case 'r': do_restore = true; break;
            case 'l': load_profile_name = optarg; break;
            case 'L': do_list = true; break;
            case 'D': do_dump = true; break;
            case 'h': print_usage(argv[0]); return EXIT_SUCCESS;
            case 'v': print_version(); return EXIT_SUCCESS;
            default: print_usage(argv[0]); return EXIT_FAILURE;
        }
    }

    int actions = (int)do_sync + (int)do_daemon + (int)do_restore + (load_profile_name != NULL) + (int)do_list + (int)do_dump;
    if (actions != 1) {
        fprintf(stderr, "Error: Specify exactly one action\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (do_list) {
        profile_list_all();
        return EXIT_SUCCESS;
    }

    if (do_dump) {
        return config_dump();
    }

    if (do_restore) {
        int ret = profiles_restore_defaults(NULL, true);
        return ret;
    }

    if (load_profile_name) {
        int ret = profiles_load_by_name(NULL, load_profile_name, true);
        return ret;
    }

    if (do_sync) {
        return timesync_sync(true);
    }

    if (do_daemon) {
        return daemon_run(interval);
    }

    print_usage(argv[0]);
    return EXIT_FAILURE;
}
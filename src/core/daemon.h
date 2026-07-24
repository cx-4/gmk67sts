#ifndef GMK67STS_DAEMON_H
#define GMK67STS_DAEMON_H

#define DAEMON_DEFAULT_INTERVAL 43200
#define DAEMON_MIN_INTERVAL 60
#define DAEMON_MAX_INTERVAL 604800

int daemon_run(int interval_seconds);

#endif /* GMK67STS_DAEMON_H */
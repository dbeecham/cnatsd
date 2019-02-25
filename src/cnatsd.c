#define _POSIX_C_SOURCE 201805L
#ifdef DEBUG
#warning Building in DEBUG mode.
#endif

#include "config.h"

#include "cnatsd.h"

#include "cnatsd_worker_task.h"
#include "cnatsd_accept_task.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/signal.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <err.h> // TODO: remove

int cnatsd_init (struct cnatsd_s * app)
{

    // Create the epoll instance
    app->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (-1 == app->epoll_fd) {
        syslog(LOG_ERR, "%s:%d: epoll_create: %s", __func__, __LINE__, strerror(errno));
        return -1;
    }

    return 0;
}


int cnatsd_start(struct cnatsd_s * app)
{
    int ret = 0;

    // Bind to port
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };
    struct addrinfo *servinfo, *p;

    /* Get server information. */
    ret = getaddrinfo(HOST, PORT, &hints, &servinfo);
    if (0 != ret) {
            /* Failed to get address information. Print an error message,
             * sleep for an hour and then try again. */
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        app->listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (-1 == app->listen_fd) {
            warn("socket");
            continue;
        }

        int yes = 1;
        if (-1 == setsockopt(app->listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
            err(EXIT_FAILURE, "setsockopt");
        }

        if (-1 == bind(app->listen_fd, p->ai_addr, p->ai_addrlen)) {
            close(app->listen_fd);
            warn("bind");
            continue;
        }

        break;

    }
    freeaddrinfo(servinfo);

    if (NULL == p) {
        fprintf(stderr, "bind failed\n");
        exit(EXIT_FAILURE);
    }

    if (-1 == listen(app->listen_fd, BACKLOG)) {
        err(EXIT_FAILURE, "listen");
    }

    return 0;
}


int main(int argc, const char *argv[])
{

    int ret = 0;
    struct cnatsd_s app = {0};

    openlog("cnatsd", LOG_CONS | LOG_PID, LOG_USER);

    syslog(LOG_INFO, "%s:%d: hello! argc=%d, argv=%p", __func__, __LINE__, argc, argv);


    ret = cnatsd_init(&app);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d: cnatsd_init returned -1", __func__, __LINE__);
        exit(EXIT_FAILURE);
    }

    ret = cnatsd_start(&app);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d: cnatsd_start returned -1", __func__, __LINE__);
        exit(EXIT_FAILURE);
    }

    ret = cnatsd_accept_task_start(&app);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d: cnatsd_accept_thread_start returned -1", __func__, __LINE__);
        exit(EXIT_FAILURE);
    }

    ret = pthread_create(&app.worker_tasks[0], NULL, cnatsd_worker_task, &app);
    if (0 != ret) {
        syslog(LOG_ERR, "%s:%d: pthread_create: %s", __func__, __LINE__, strerror(errno));
        return -1;
    }

    siginfo_t siginfo;
    int signal_number = 0;
    for (signal_number = sigwaitinfo(&app.sigset, &siginfo);
         signal_number != -1;
         signal_number = sigwaitinfo(&app.sigset, &siginfo))
    {
        syslog(LOG_INFO, "signal cought");
        // dispatch on siginfo...
        // pthread_cancel -> pthread_join?
        // write to stderr, out?
    }
    if (-1 == signal_number) {
        syslog(LOG_ERR, "%s:%d: sigwaitinfo: %s", __func__, __LINE__, strerror(errno));
        return -1;
    }
    

    

    return 0;
}

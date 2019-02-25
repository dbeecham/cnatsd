#include "config.h"
#include "cnatsd.h"
#include "cnatsd_client.h"
#include "cnatsd_accept_task.h"

#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <syslog.h>
#include <string.h>


// Free up associated data structures of a user
static int cnatsd_worker_task_free_user(
    struct cnatsd_s * app,
    int client
)
{
    cnatsd_client_free(app, &app->clients[client]);
    return 0;
}


// kill of a live client (and free up data structures)
static int cnatsd_worker_task_kill_user(
    struct cnatsd_s * app,
    int client
)
{

    close(client);
    return cnatsd_worker_task_free_user(app, client);

}


static int cnatsd_worker_task_epoll_event_dispatch(
    struct cnatsd_s * app,
    struct epoll_event * event
)
{

    int ret = 0;

    assert(((event->events & EPOLLIN) == EPOLLIN) && "not epollin");


    // TODO: look up this user the user table - if user not found, then error
    // read data from user
    // TODO: parse data using user's parser
    // TODO: re-arm user's fd in epoll


    // read in data from user...
    char BUF[128];
    ssize_t bytes_read = read(event->data.fd, BUF, 127);

    syslog(LOG_DEBUG, "%s:%d: read %zd bytes\n", __func__, __LINE__, bytes_read);

    if (bytes_read < 0) {
        syslog(LOG_ERR, "%s:%d: read: %s", __func__, __LINE__, strerror(errno));
        return -1;
    }
    if (0 == bytes_read) {
        // TODO: clean this user from the structures...
        syslog(LOG_INFO, "%s:%d: client disconnecting...", __func__, __LINE__);
        return cnatsd_worker_task_kill_user(app, event->data.fd);
    }


    ret = cnatsd_client_parse(&app->clients[event->data.fd], BUF, bytes_read);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d: cnatsd_client_parse returned %d", __func__, __LINE__, ret);
        // TODO: clean this user
    }


    // re-arm user's fd in epoll
    // Re-arm EPOLLONESHOT file descriptor in epoll
    ret = epoll_ctl(
        app->epoll_fd,
        EPOLL_CTL_MOD,
        event->data.fd,
        &(struct epoll_event){
            .events = EPOLLIN | EPOLLONESHOT,
            .data = event->data
        }
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d: epoll_ctl: %s", __func__, __LINE__, strerror(errno));
        return -1;
    }
    

    return 0;
}


static int cnatsd_worker_task_epoll_handle_events(
    struct cnatsd_s * app,
    struct epoll_event events[WORKER_TASK_NUM_EPOLL_EVENTS],
    int ep_num_events
)
{
    int ret = 0;
    for (int i = 0; i < ep_num_events; i++) {
        ret = cnatsd_worker_task_epoll_event_dispatch(app, &events[i]);
        if (0 != ret) {
            return ret;
        }
    }
}


void * cnatsd_worker_task(void * arg)
{

    int ret = 0;
    struct cnatsd_s * app = arg;

    int ep_num_events = 0;
    struct epoll_event events[WORKER_TASK_NUM_EPOLL_EVENTS];
    for (ep_num_events = epoll_wait(app->epoll_fd, events, WORKER_TASK_NUM_EPOLL_EVENTS, -1);
         ep_num_events > 0;
         ep_num_events = epoll_wait(app->epoll_fd, events, WORKER_TASK_NUM_EPOLL_EVENTS, -1))
    {
        ret = cnatsd_worker_task_epoll_handle_events(app, events, ep_num_events);
        if (-1 == ret) {
            break;
        }
    }
    if (-1 == ep_num_events) {
        syslog(LOG_ERR, "%s:%d: epoll_wait: %s", __func__, __LINE__, strerror(errno));
        exit(EXIT_FAILURE);
    }
    

    return NULL;
}

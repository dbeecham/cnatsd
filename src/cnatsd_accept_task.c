#define _POSIX_C_SOURCE 201805L
#ifdef DEBUG
#warning Building in DEBUG mode.
#endif

#include "config.h"
#include "cnatsd.h"
#include "cnatsd_accept_task.h"
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <syslog.h>

#include "cnatsd.h"

#include "cnatsd_client.h"

#define loop for(;;)


// cnatsd_accept_task_add_client gets called by
// cnatsd_accept_task_accept_client once a client has been
// `accept(2)`ed - i.e. we have established a TCP connection with
// the client. Our job is to set up necessary data structures
// for this user as well as to trigger whatever followup behaviour
// is necessary for the user (i.e. authentication, etc). Interaction
// with the client is not our job though, so we just add the client
// to epoll in such a way so that the worker thread can figure out
// what's up and deal with it.
int cnatsd_accept_task_add_client(struct cnatsd_s * app, int client_fd)
{

    if (NUM_MAX_CLIENTS <= client_fd) {
        write(client_fd, STR_SERVER_FULL, strlen(STR_SERVER_FULL));
        close(client_fd);
        return 0;
    }

    int ret = 0;

    // Create data structures for this user
    app->clients[client_fd] = (struct cnatsd_client_s){0};
    ret = cnatsd_client_init(app, &app->clients[client_fd]);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d: cnats_client_init returned %d", __func__, __LINE__, ret);
        return -1;
    }


    // Add the user to epoll fd so that worker threads can
    // pick up from here.
    ret = epoll_ctl(
        app->epoll_fd,
        EPOLL_CTL_ADD,
        client_fd,
        &(struct epoll_event){
            .events = EPOLLIN | EPOLLONESHOT,
            .data = {
                .fd = client_fd
            }
        }
    );
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d: epoll_ctl: %s", __func__, __LINE__, strerror(errno));
        return -1;
    }

    return 0;
}




// cnatsd_accept_task_accept_client is called from
// epoll_wait when there is a client waiting to connect
// to our service. All we need to do is to `accept(2)`
// the client and add the client to the application-global
// list of clients.
static int cnatsd_accept_task_accept_client(
    struct cnatsd_s * app
)
{

    int ret = 0;
    socklen_t sin_size;
    struct sockaddr_storage their_addr;
    int client_fd = 0;

    syslog(LOG_DEBUG, "%s:%d: hi! app=%p", __func__, __LINE__, app);

    sin_size = sizeof(their_addr);
    client_fd = accept(app->listen_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (-1 == client_fd) {
        syslog(LOG_WARNING, "%s:%d: failed to accept client: %s",
                __func__, __LINE__, strerror(errno));
        return 0;
    }


    syslog(LOG_DEBUG, "%s:%d: client %d connected", __func__, __LINE__, client_fd);

    // Client has been accepted! Add the client to the list of users.
    ret = cnatsd_accept_task_add_client(app, client_fd);
    if (-1 == ret) {
        syslog(LOG_ERR, "%s:%d: cnatsd_accept_task_add_client returned -1",
                __func__, __LINE__);
        return -1;
    }

    return 0;
}



void * cnatsd_accept_task(void * arg)
{
    int ret = 0;
    struct cnatsd_s * app = arg;

    loop {
        ret = cnatsd_accept_task_accept_client(app);
        if (-1 == ret) {
            syslog(LOG_WARNING, "%s:%d: cnatsd_accept_task_accept_client returned %d (too many connections?)", __func__, __LINE__, ret);
        }
    }
}



int cnatsd_accept_task_start(struct cnatsd_s * app)
{

    int ret = pthread_create(&app->accept_task.thread, NULL, cnatsd_accept_task, app);
    if (0 != ret) {
        syslog(LOG_ERR, "%s:%d: pthread_create: %s", __func__, __LINE__, strerror(errno));
        return -1;
    }
    return 0;
}

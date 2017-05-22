#include <stdlib.h>
#include <stdio.h>

#include "public.h"

#define srs_trace(msg, ...)   printf(msg, ##__VA_ARGS__);printf("\n")

int io_port = 9320;
int sleep_ms = 100;

#if 1
int sleep_test()
{
    srs_trace("===================================================");
    srs_trace("sleep test: start");
    
    srs_trace("1. sleep...");
    st_utime_t start = st_utime();
    st_usleep(sleep_ms * 1000);
    st_utime_t end = st_utime();
    
    srs_trace("2. sleep ok, sleep=%dus, deviation=%dus", 
        (int)(sleep_ms * 1000), (int)(end - start - sleep_ms * 1000));

    srs_trace("sleep test: end");
    
    return 0;
}

void* sleep2_func0(void* arg)
{
    int sleep_ms = 100;
    st_utime_t start = st_utime();
    st_usleep(sleep_ms * 1000);
    st_utime_t end = st_utime();
    
    srs_trace("sleep ok, sleep=%dus, deviation=%dus", 
        (int)(sleep_ms * 1000), (int)(end - start - sleep_ms * 1000));
        
    return NULL;
}

void* sleep2_func1(void* arg)
{
    int sleep_ms = 250;
    st_utime_t start = st_utime();
    st_usleep(sleep_ms * 1000);
    st_utime_t end = st_utime();
    
    srs_trace("sleep ok, sleep=%dus, deviation=%dus", 
        (int)(sleep_ms * 1000), (int)(end - start - sleep_ms * 1000));
        
    return NULL;
}

int sleep2_test()
{
    srs_trace("===================================================");
    srs_trace("sleep2 test: start");
    
    st_thread_t trd0 = st_thread_create(sleep2_func0, NULL, 1, 0);
    st_thread_t trd1 = st_thread_create(sleep2_func1, NULL, 1, 0);
    st_thread_join(trd0, NULL);
    st_thread_join(trd1, NULL);

    srs_trace("sleep test: end");
    
    return 0;
}

st_mutex_t sleep_work_cond = NULL;
void* sleep_deviation_func(void* arg)
{
    st_mutex_lock(sleep_work_cond);
    srs_trace("2. work thread start.");

    int64_t i;
    for (i = 0; i < 3000000000ULL; i++) {
    }
    
    st_mutex_unlock(sleep_work_cond);
    srs_trace("3. work thread end.");
    
    return NULL;
}

int sleep_deviation_test()
{
    srs_trace("===================================================");
    srs_trace("sleep deviation test: start");
    
    sleep_work_cond = st_mutex_new();
    
    st_thread_create(sleep_deviation_func, NULL, 0, 0);
    st_mutex_lock(sleep_work_cond);
    
    srs_trace("1. sleep...");
    st_utime_t start = st_utime();
    
    // other thread to do some complex work.
    st_mutex_unlock(sleep_work_cond);
    st_usleep(1000 * 1000);
    
    st_utime_t end = st_utime();
    
    srs_trace("4. sleep ok, sleep=%dus, deviation=%dus", 
        (int)(sleep_ms * 1000), (int)(end - start - sleep_ms * 1000));

    st_mutex_lock(sleep_work_cond);
    srs_trace("sleep deviation test: end");
    
    st_mutex_destroy(sleep_work_cond);
    
    return 0;
}

void* thread_func(void* arg)
{
    srs_trace("1. thread run");
    st_usleep(sleep_ms * 1000);
    srs_trace("2. thread completed");
    return NULL;
}

int thread_test()
{
    srs_trace("===================================================");
    srs_trace("thread test: start");
    
    st_thread_t trd = st_thread_create(thread_func, NULL, 1, 0);
    if (trd == NULL) {
        srs_trace("st_thread_create failed");
        return -1;
    }
    
    st_thread_join(trd, NULL);
    srs_trace("3. thread joined");
    
    srs_trace("thread test: end");
    
    return 0;
}

st_mutex_t sync_start = NULL;
st_cond_t sync_cond = NULL;
st_mutex_t sync_mutex = NULL;
st_cond_t sync_end = NULL;

void* sync_master(void* arg)
{
    // wait for main to sync_start this thread.
    st_mutex_lock(sync_start);
    st_mutex_unlock(sync_start);
    
    st_usleep(sleep_ms * 1000);
    st_cond_signal(sync_cond);
    
    st_mutex_lock(sync_mutex);
    srs_trace("2. st mutex is ok");
    st_mutex_unlock(sync_mutex);
    
    st_usleep(sleep_ms * 1000);
    srs_trace("3. st thread is ok");
    st_cond_signal(sync_cond);
    
    return NULL;
}

void* sync_slave(void* arg)
{
    // lock mutex to control thread.
    st_mutex_lock(sync_mutex);
    
    // wait for main to sync_start this thread.
    st_mutex_lock(sync_start);
    st_mutex_unlock(sync_start);
    
    // wait thread to ready.
    st_cond_wait(sync_cond);
    srs_trace("1. st cond is ok");
    
    // release mutex to control thread
    st_usleep(sleep_ms * 1000);
    st_mutex_unlock(sync_mutex);
    
    // wait thread to exit.
    st_cond_wait(sync_cond);
    srs_trace("4. st is ok");
    
    st_cond_signal(sync_end);
    
    return NULL;
}

int sync_test()
{
    srs_trace("===================================================");
    srs_trace("sync test: start");
    
    if ((sync_start = st_mutex_new()) == NULL) {
        srs_trace("st_mutex_new sync_start failed");
        return -1;
    }
    st_mutex_lock(sync_start);

    if ((sync_cond = st_cond_new()) == NULL) {
        srs_trace("st_cond_new cond failed");
        return -1;
    }

    if ((sync_end = st_cond_new()) == NULL) {
        srs_trace("st_cond_new end failed");
        return -1;
    }
    
    if ((sync_mutex = st_mutex_new()) == NULL) {
        srs_trace("st_mutex_new mutex failed");
        return -1;
    }
    
    if (!st_thread_create(sync_master, NULL, 0, 0)) {
        srs_trace("st_thread_create failed");
        return -1;
    }
    
    if (!st_thread_create(sync_slave, NULL, 0, 0)) {
        srs_trace("st_thread_create failed");
        return -1;
    }
    
    // run all threads.
    st_mutex_unlock(sync_start);
    
    st_cond_wait(sync_end);
    srs_trace("sync test: end");
    
    return 0;
}

void* io_client(void* arg)
{
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        srs_trace("create linux socket error.");
        return NULL;
    }
    srs_trace("6. client create linux socket success. fd=%d", fd);
    
    st_netfd_t stfd;
    if ((stfd = st_netfd_open_socket(fd)) == NULL){
        srs_trace("st_netfd_open_socket open socket failed.");
        return NULL;
    }
    srs_trace("7. client st open socket success. fd=%d", fd);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(io_port);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    if (st_connect(stfd, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in), ST_UTIME_NO_TIMEOUT) == -1) {
        srs_trace("connect socket error.");
        return NULL;
    }
    
    char buf[128];
    if (st_read_fully(stfd, buf, sizeof(buf), ST_UTIME_NO_TIMEOUT) != sizeof(buf)) {
        srs_trace("st_read_fully failed");
        return NULL;
    }
    if (st_write(stfd, buf, sizeof(buf), ST_UTIME_NO_TIMEOUT) != sizeof(buf)) {
        srs_trace("st_write failed");
        return NULL;
    }
    
    st_netfd_close(stfd);
    
    return NULL;
}

int io_test()
{
    srs_trace("===================================================");
    srs_trace("io test: start, port=%d", io_port);
    
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        srs_trace("create linux socket error.");
        return -1;
    }
    srs_trace("1. server create windows socket success. fd=%d", fd);
    
    int reuse_socket = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_socket, sizeof(int)) == -1) {
        srs_trace("setsockopt reuse-addr error.");
        return -1;
    }
    srs_trace("2. server setsockopt reuse-addr success. fd=%d", fd);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(io_port);
	addr.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
    if (bind(fd, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        srs_trace("bind socket error.");
        return -1;
    }
    srs_trace("3. server bind socket success. fd=%d", fd);
    
    if (listen(fd, 10) == -1) {
        srs_trace("listen socket error.");
        return -1;
    }
    srs_trace("4. server listen socket success. fd=%d", fd);
    
	st_netfd_t stfd;
	if ((stfd = st_netfd_open_socket(fd)) == NULL){
		srs_trace("st_netfd_open_socket open socket failed.");
		return -1;
	}
	srs_trace("5. server st open socket success. fd=%d", fd);

	if (!st_thread_create(io_client, NULL, 0, 0)) {
		srs_trace("st_thread_create failed");
		return -1;
	}
    
    st_netfd_t client_stfd = st_accept(stfd, NULL, NULL, ST_UTIME_NO_TIMEOUT);
    srs_trace("8. server get a client. fd=%d", st_netfd_fileno(client_stfd));
    
    char buf[128];
    if (st_write(client_stfd, buf, sizeof(buf), ST_UTIME_NO_TIMEOUT) != sizeof(buf)) {
        srs_trace("st_write failed");
        return -1;
    }
    if (st_read_fully(client_stfd, buf, sizeof(buf), ST_UTIME_NO_TIMEOUT) != sizeof(buf)) {
        srs_trace("st_read_fully failed");
        return -1;
    }
    srs_trace("9. server io completed.");
    
    st_netfd_close(stfd);
    st_netfd_close(client_stfd);
    
    srs_trace("io test: end");
    return 0;
}

int io_test_2()
{
	srs_trace("===================================================");
	srs_trace("io test 2: start client");

#if 0
	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		srs_trace("create socket error.");
		return NULL;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(io_port);
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	int ret = connect(fd, (const struct sockaddr*)&addr, sizeof(struct sockaddr_in));
	if (ret < 0){
		srs_trace("connect error");
	}

#else
	st_thread_t trd = st_thread_create(io_client, NULL, 1, 0);
	if (trd == NULL) {
		srs_trace("st_thread_create failed");
		return -1;
	}
	st_thread_join(trd, NULL);
#endif
	srs_trace("io test: end");
	return 0;
}
#endif

int main(int argc, char** argv)
{
    srs_trace("test begin");

	if (st_set_eventsys(ST_EVENTSYS_SELECT) < 0) {
        srs_trace("st_set_eventsys failed");
        return -1;
    }
    
    if (st_init() < 0) {
        srs_trace("st_init failed");
        return -1;
    }

    if (io_test() < 0) {
        srs_trace("io_test failed");
        //return -1;
    }
#if 0
	if (sleep_test() < 0) {
		srs_trace("sleep_test failed");
		return -1;
	}

	if (thread_test() < 0) {
		srs_trace("thread_test failed");
		return -1;
	}

	if (sync_test() < 0) {
		srs_trace("sync_test failed");
		return -1;
	}

	if (sleep_deviation_test() < 0) {
		srs_trace("sleep_deviation_test failed");
		return -1;
	}

	if (io_test_2() < 0) {
		srs_trace("io_test failed");
		return -1;
	}
#endif  
    // cleanup.
    srs_trace("wait for all thread completed");
    //st_thread_exit(NULL);
    // the following never enter, 
    // the above code will exit when all thread exit,
    // current is a primordial st-thread, when all thread exit,
    // the st idle thread will exit(0), see _st_idle_thread_start()
    //srs_trace("all thread completed");
	printf("print any key to continue....");
	getchar();
    return 0;
}


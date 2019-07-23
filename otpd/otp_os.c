// Copyright[2015] <SPRD>
#include "otp_os.h"

typedef struct {
	pthread_mutex_t mutex;
} THREAD_COMMUNICAT;
static THREAD_COMMUNICAT communicate;

void getMutex(void) { pthread_mutex_lock(&communicate.mutex); }
void putMutex(void) { pthread_mutex_unlock(&communicate.mutex); }

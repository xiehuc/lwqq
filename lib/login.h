/**
 * @file   login.h
 * @author mathslinux <riegamaths@gmail.com>
 * @date   Sun May 20 02:25:51 2012
 *
 * @brief  Linux WebQQ Login API
 *
 *
 */

#ifndef LWQQ_LOGIN_H
#define LWQQ_LOGIN_H

#include "type.h"

typedef struct LwqqAsyncEvent LwqqAsyncEvent;

/**
 * WebQQ login function
 *
 * @param client Lwqq Client
 * @param err Error code
 */
LwqqAsyncEvent* lwqq_login(LwqqClient* lc, LwqqStatus status);

LwqqAsyncEvent* lwqq_relink(LwqqClient* lc);

/**
 * WebQQ logout function
 *
 * @param client Lwqq Client
 * @param wait_time block wait to quit until wait_time
 */
LwqqErrorCode lwqq_logout(LwqqClient* client, unsigned wait_time);
#endif /* LWQQ_LOGIN_H */


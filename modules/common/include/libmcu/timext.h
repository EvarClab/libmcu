#ifndef LIBMCU_TIMEXT_H
#define LIBMCU_TIMEXT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>

void timeout_set(unsigned int *goal, unsigned int msec);
bool timeout_is_expired(unsigned int goal);

void timeout_set_sec(unsigned int *goal, unsigned int sec);
bool timeout_is_expired_sec(unsigned int goal);

void sleep_ms(unsigned int msec);

#if defined(__cplusplus)
}
#endif

#endif /* LIBMCU_TIMEXT_H */

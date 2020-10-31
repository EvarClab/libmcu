#include "semaphore.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "compiler.h"

struct semaphore {
	SemaphoreHandle_t handle;
};

int sem_init(sem_t *sem, int UNUSED pshared, unsigned int value)
{
	struct semaphore *psem = (struct semaphore *)sem;

	psem->handle = xSemaphoreCreateCounting(value, 0);
	if (psem->handle == NULL) {
		return -1;
	}

	return 0;
}

int sem_destroy(sem_t *sem)
{
	struct semaphore *psem = (struct semaphore *)sem;
	vSemaphoreDelete(psem->handle);
	return 0;
}

int sem_wait(sem_t *sem)
{
	struct semaphore *psem = (struct semaphore *)sem;
	return xSemaphoreTake(psem->handle, portMAX_DELAY) == pdPASS? 0 : -1;
}

int sem_timedwait(sem_t *sem, unsigned int timeout_ms)
{
	struct semaphore *psem = (struct semaphore *)sem;
	return xSemaphoreTake(psem->handle, timeout_ms / portTICK_PERIOD_MS)
		== pdPASS? 0 : -1;
}

int sem_trywait(sem_t *sem)
{
	struct semaphore *psem = (struct semaphore *)sem;
	return xSemaphoreTake(psem->handle, 0) == pdPASS? 0 : -1;
}

int sem_post(sem_t *sem)
{
	struct semaphore *psem = (struct semaphore *)sem;
	return xSemaphoreGive(psem->handle) == pdPASS? 0 : -1;
}

int sem_getvalue(sem_t *sem, int *sval)
{
	struct semaphore *psem = (struct semaphore *)sem;
	*sval = uxSemaphoreGetCount(psem->handle);
	return 0;
}

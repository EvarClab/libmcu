#include "libmcu/logging.h"

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#if defined(__STDC_HOSTED__)
#include <stdio.h>
#endif

#include "libmcu/compiler.h"
#include "libmcu/assert.h"

#if !defined(MIN)
#define MIN(a, b)				((a) > (b)? (b) : (a))
#endif

#define LOGGING_MAGIC				0xA5A5U

typedef uint16_t logging_magic_t;

typedef struct {
	time_t timestamp;
	uintptr_t pc;
	uintptr_t lr;
	logging_magic_t magic;
	uint16_t message_length;
	logging_t type;
	uint8_t message[];
} LIBMCU_PACKED logging_data_t;
LIBMCU_STATIC_ASSERT(sizeof(logging_t) == sizeof(uint8_t),
		"The size of logging_t must be the same of uint8_t.");
LIBMCU_STATIC_ASSERT(LOGGING_TYPE_MAX <= (1U << (sizeof(logging_t) * 8)) - 1,
		"TYPE_MAX must not exceed its data type size.");
LIBMCU_STATIC_ASSERT(LOGGING_MESSAGE_MAXLEN
		< (1U << (sizeof(((logging_data_t *)0)->message_length) * 8)),
		"MESSAGE_MAXLEN must not exceed its data type size.");

struct logging_tag {
	const char *tag;
	logging_t min_log_level;
};

static struct {
	struct logging_tag tags[LOGGING_TAGS_MAXNUM];
	struct logging_tag global_tag;

	const logging_storage_t *storage;
} m;

static const char *stringify_type(logging_t type)
{
	switch (type) {
	case LOGGING_TYPE_VERBOSE:
		return "VERBOSE";
	case LOGGING_TYPE_DEBUG:
		return "DEBUG";
	case LOGGING_TYPE_INFO:
		return "INFO";
	case LOGGING_TYPE_WARN:
		return "WARN";
	case LOGGING_TYPE_ERROR:
		return "ERROR";
	case LOGGING_TYPE_NONE:
		return "NONE";
	default:
		break;
	}
	return "UNKNOWN";
}

static logging_magic_t compute_magic(const logging_data_t *entry)
{
	return (logging_magic_t)(entry->pc ^ entry->lr ^ LOGGING_MAGIC);
}

static bool is_global_tag(const struct logging_tag *tag)
{
	return tag == &m.global_tag;
}

static struct logging_tag *get_global_tag(void)
{
	return &m.global_tag;
}

static void clear_tags(void)
{
	for (int i = 0; i < LOGGING_TAGS_MAXNUM; i++) {
		memset(&m.tags[i], 0, sizeof(m.tags[i]));
	}

	memset(get_global_tag(), 0, sizeof(*get_global_tag()));
}

static struct logging_tag *get_empty_tag(void)
{
	for (int i = 0; i < LOGGING_TAGS_MAXNUM; i++) {
		struct logging_tag *p = &m.tags[i];
		if (p->tag == NULL) {
			return p;
		}
	}

	return NULL;
}

static struct logging_tag *get_tag_from_string(const char *tag)
{
	for (int i = 0; i < LOGGING_TAGS_MAXNUM; i++) {
		struct logging_tag *p = &m.tags[i];
		if (p->tag == tag) {
			return p;
		}
	}

	return NULL;
}

static struct logging_tag *register_tag(const char *tag)
{
	struct logging_tag *p = get_empty_tag();

	if (p == NULL) {
		return get_global_tag();
	}

	p->tag = tag;
	return p;
}

static struct logging_tag *obtain_tag(const char *tag)
{
	struct logging_tag *p = get_tag_from_string(tag);

	if (p == NULL) {
		p = register_tag(tag);
	}

	return p;
}

static bool is_logging_type_enabled(const struct logging_tag *tag,
		const logging_t type)
{
	if (!is_global_tag(tag) && type < get_global_tag()->min_log_level) {
		return false;
	}
	if (type < tag->min_log_level) {
		return false;
	}
	return true;
}

static bool is_logging_type_valid(const logging_t type)
{
	if (type >= LOGGING_TYPE_MAX) {
		return false;
	}
	return true;
}

static size_t get_log_size(const logging_data_t *entry)
{
	size_t sz = sizeof(*entry);

	if (entry && entry->message_length) {
		sz += entry->message_length;
	}

	return sz;
}

static size_t consume_internal(size_t consume_size)
{
	if (!consume_size) {
		return 0;
	}
	return m.storage->consume(consume_size);
}

static size_t peek_internal(void *buf, size_t bufsize)
{
	if (!buf || bufsize < sizeof(logging_data_t)) {
		return 0;
	}
	return m.storage->peek(buf, bufsize);
}

#define pack_message(ptr, basearg) do { \
	va_list ap; \
	const char *fmt; \
	int len = 0; \
	va_start(ap, basearg); \
	fmt = va_arg(ap, char *); \
	if (fmt) { \
		len = vsnprintf((char *)ptr->message, \
				LOGGING_MESSAGE_MAXLEN - 1, \
				fmt, ap); \
	} \
	va_end(ap); \
	ptr->message_length = MIN((uint16_t)len, LOGGING_MESSAGE_MAXLEN); \
} while (0)

static void pack_log(logging_data_t *entry, logging_t type,
		const void *pc, const void *lr)
{
	*entry = (logging_data_t) {
		.timestamp = time(NULL),
		.type = type,
		.pc = (uintptr_t)pc,
		.lr = (uintptr_t)lr,
		.magic = LOGGING_MAGIC,
		.message_length = 0,
	};
	entry->magic = compute_magic(entry);
}

size_t logging_save(logging_t type, const struct logging_context *ctx, ...)
{
	static uint8_t buf[LOGGING_MESSAGE_MAXLEN + sizeof(logging_data_t)];
	size_t result = 0;

	assert(ctx != NULL);

	logging_lock();
	{
		const struct logging_tag *tag = obtain_tag(ctx->tag);

		if (!is_logging_type_valid(type)) {
			goto out;
		}
		if (!is_logging_type_enabled(tag, type)) {
			goto out;
		}

		logging_data_t *log = (logging_data_t *)buf;
		pack_log(log, type, ctx->pc, ctx->lr);
		pack_message(log, ctx);
		// TODO: logging_encode(log)

		result = m.storage->write(log, get_log_size(log));
	}
out:
	logging_unlock();

	return result;
}

size_t logging_peek(void *buf, size_t bufsize)
{
	size_t result = 0;

	logging_lock();
	{
		result = peek_internal(buf, bufsize);
	}
	logging_unlock();

	return result;
}

size_t logging_consume(size_t consume_size)
{
	size_t result = 0;

	logging_lock();
	{
		result = consume_internal(consume_size);
	}
	logging_unlock();

	return result;
}

size_t logging_read(void *buf, size_t bufsize)
{
	size_t size_read = 0;

	logging_lock();
	{
		size_read = m.storage->read(buf, bufsize);
	}
	logging_unlock();

	return size_read;
}

size_t logging_count(void)
{
	size_t result = 0;

	logging_lock();
	{
		result = m.storage->count();
	}
	logging_unlock();

	return result;
}

size_t logging_count_tags(void)
{
	size_t cnt = 0;

	logging_lock();
	{
		for (int i = 0; i < LOGGING_TAGS_MAXNUM; i++) {
			if (m.tags[cnt].tag != NULL) {
				cnt++;
			}
		}
	}
	logging_unlock();

	return cnt;
}

void logging_set_level(const char *tag, logging_t min_log_level)
{
	struct logging_tag *p;

	logging_lock();
	{
		p = obtain_tag(tag);
	}
	logging_unlock();

	if (min_log_level < LOGGING_TYPE_MAX && !is_global_tag(p)) {
		p->min_log_level = min_log_level;
	}
}

logging_t logging_get_level(const char *tag)
{
	logging_t result;

	logging_lock();
	{
		result = obtain_tag(tag)->min_log_level;
	}
	logging_unlock();

	return result;
}

void logging_set_level_global(logging_t min_log_level)
{
	if (min_log_level < LOGGING_TYPE_MAX) {
		get_global_tag()->min_log_level = min_log_level;
	}
}

logging_t logging_get_level_global(void)
{
	return get_global_tag()->min_log_level;
}

void logging_init(const logging_storage_t *log_storage)
{
	assert(log_storage != NULL);

	logging_lock_init();

	clear_tags();
	m.storage = log_storage;
}

void logging_iterate_tag(void (*callback_each)(const char *tag,
			logging_t min_log_level))
{
	assert(callback_each != NULL);

	for (int i = 0; i < LOGGING_TAGS_MAXNUM; i++) {
		const struct logging_tag *p = &m.tags[i];
		if (p->tag == NULL) {
			continue;
		}
		callback_each(p->tag, p->min_log_level);
	}
}

const char *logging_stringify(char *buf, size_t bufsize, const void *log)
{
	const logging_data_t *p = (const logging_data_t *)log;
	int len = snprintf(buf, bufsize-2, "%lu: [%s] <%p,%p> ",
			(unsigned long)p->timestamp, stringify_type(p->type),
			(void *)p->pc, (void *)p->lr);
	buf[bufsize-1] = '\0';

	if (len > 0) {
		size_t msglen = MIN(bufsize - (size_t)len - 1, p->message_length);
		memcpy(&buf[len], p->message, msglen);
		buf[msglen + (size_t)len] = '\0';
	}

	return buf;
}

LIBMCU_WEAK void logging_lock_init(void)
{
}

LIBMCU_WEAK void logging_lock(void)
{
}

LIBMCU_WEAK void logging_unlock(void)
{
}

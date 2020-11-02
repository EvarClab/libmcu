#include "CppUTest/TestHarness.h"

extern "C" {
#include "mode.h"
}

TEST_GROUP(RunMode) {
	void setup(void) {
		mode_init();
		mode_set(REPORT_MODE);
	}
	void teardown() {
	}
};

static int nr_handler_called;
static long current_mode;
static void mode_change_handler(void *param) {
	current_mode = (long)param;
	nr_handler_called++;
}

TEST(RunMode, mode_set_ShouldKeepTheModeAsIs_WhenUnknownMode) {
	mode_set(UNKNOWN_MODE);
	CHECK_EQUAL(REPORT_MODE, mode_get());
}

TEST(RunMode, mode_set_ShouldKeepTheModeAsIs_WhenTheSameMode) {
	mode_set(REPORT_MODE);
	CHECK_EQUAL(REPORT_MODE, mode_get());
}

TEST(RunMode, mode_set_ShouldCallHandlersRegistered) {
	mode_register_transition_callback(mode_change_handler);
	nr_handler_called = 0;
	mode_set(PROVISIONING_MODE);
	CHECK_EQUAL(1, nr_handler_called);
	CHECK_EQUAL(PROVISIONING_MODE, current_mode);
	mode_unregister_transition_callback(mode_change_handler);
}

TEST(RunMode, mode_set_ShouldCallHandlersRegisteredOnlyOnce_WhenSameHandlerRegisteredTwice) {
	mode_register_transition_callback(mode_change_handler);
	mode_register_transition_callback(mode_change_handler);
	nr_handler_called = 0;
	mode_set(PROVISIONING_MODE);
	CHECK_EQUAL(1, nr_handler_called);
	CHECK_EQUAL(PROVISIONING_MODE, current_mode);
	mode_unregister_transition_callback(mode_change_handler);
}

TEST(RunMode, mode_get_ShouldReturnTheCurrentMode) {
	CHECK_EQUAL(REPORT_MODE, mode_get());
	mode_set(PROVISIONING_MODE);
	CHECK_EQUAL(PROVISIONING_MODE, mode_get());
}

TEST(RunMode, callback_register_ShouldReturnZero) {
	CHECK_EQUAL(0, mode_register_transition_callback(mode_change_handler));
	mode_unregister_transition_callback(mode_change_handler);
}

#if 0
TEST(RunMode, stringify_ShouldReturnString_WhenReturnCodeGiven) {
	STRCMP_EQUAL("provisioning", mode_stringify(PROVISIONING_MODE));
	STRCMP_EQUAL("report", mode_stringify(REPORT_MODE));
	STRCMP_EQUAL("unknown", mode_stringify(UNKNOWN_MODE));
}
#endif

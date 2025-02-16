COMPONENT_NAME = logging

SRC_FILES = \
	../modules/logging/src/logging.c

TEST_SRC_FILES = \
	src/logging/logging_test.cpp \
	src/test_all.cpp \

INCLUDE_DIRS = \
	stubs/overrides \
	../modules/logging/include \
	../modules/common/include \
	$(CPPUTEST_HOME)/include \

CPPUTEST_CPPFLAGS = -DLOGGING_TAG=TAG
MOCKS_SRC_DIRS =

include MakefileRunner.mk

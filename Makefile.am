ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4

bin_PROGRAMS = fast-repo

src/%.pb.cc: src/%.proto
	if test "no${PROTOC}" == "no"; then \
	  cat protobuf-stub.txt > $@ ; \
	else \
	  $(PROTOC) --proto_path=$(dir $<) --cpp_out=$(dir $<) $< ; \
	fi

BUILT_SOURCES = src/repo-command-response.pb.cc src/repo-command-response.pb.h \
    src/repo-command-parameter.pb.cc src/repo-command-parameter.pb.h

fast_repo_SOURCES = src/main.cpp \
    third_party/docopt/docopt.cpp \
    src/repo/fast-repo.cpp \
    src/storage/storage-engine.cpp \
    src/repo-command-response.pb.cc \
    src/repo-command-parameter.pb.cc \
    src/handles/base-handle.cpp \
    src/handles/pattern-handle.cpp \
    src/handles/delete-handle.cpp \
    src/handles/read-handle.cpp \
    src/handles/watch-handle.cpp \
    src/handles/write-handle.cpp \
    src/handles/patterns/counter-pattern.cpp

fast_repo_LDADD = ${LIBS} -lcnl-cpp \
    ${BOOST_SYSTEM_LIB} ${BOOST_ASIO_LIB} ${BOOST_THREAD_LIB} ${BOOST_TIMER_LIB}


# Testing setup. Type 'make check' or 'make recheck' to run the tests.
# TESTS defines a list of tests to run. Define their sources in the usual way.
GTEST_ROOT=$(top_srcdir)/third_party/googletest/googletest
GMOCK_ROOT=$(top_srcdir)/third_party/googletest/googlemock
TEST_FLAGS = -I$(top_srcdir)/src \
    -I${GTEST_ROOT} -I${GMOCK_ROOT} \
    -I${GTEST_ROOT}/include -I${GMOCK_ROOT}/include
    # -DGTEST_HAS_PTHREAD=1
TEST_LIBS = ${LIBS} -lcnl-cpp ${BOOST_SYSTEM_LIB} ${BOOST_FILESYSTEM_LIBS}
TEST_COMMON_SOURCE = ${GMOCK_ROOT}/src/gmock-all.cc ${GTEST_ROOT}/src/gtest-all.cc

TESTS = tests/test_prefix_match
check_PROGRAMS = $(TESTS)

tests_test_prefix_match_SOURCES = ${TEST_COMMON_SOURCE} tests/test_prefix_match.cc src/storage/storage-engine.cpp
tests_test_prefix_match_CPPFLAGS = $(TEST_FLAGS)
tests_test_prefix_match_LDADD =  $(TEST_LIBS)

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
    src/handles/write-handle.cpp

fast_repo_LDADD = ${LIBS} -lcnl-cpp \
    ${BOOST_SYSTEM_LIB} ${BOOST_ASIO_LIB} ${BOOST_THREAD_LIB} ${BOOST_TIMER_LIB}
//
// test_prefix_match.cc
//
//  Created by Peter Gusev on 20 October 2018.
//  Copyright 2013-2018 Regents of the University of California
//

#include <stdlib.h>
#include <ndn-cpp/name.hpp>
#include <ndn-cpp/data.hpp>
#include <ndn-cpp/interest.hpp>
#include <execinfo.h>

#include "config.hpp"
#include "gtest/gtest.h"
#include "storage/storage-engine.hpp"

#ifdef HAVE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
#endif

using namespace fast_repo;
using namespace ndn;

static const std::string dbPath = "/tmp/fast-repo-db-test";

#include <rocksdb/db.h>

namespace db_namespace=rocksdb;

TEST(TestDb, TestCreate)
{
    // db_namespace::Options options;
    // options.create_if_missing = true;
    // db_namespace::Status status;
    // db_namespace::DB *db;

    // status = db_namespace::DB::Open(options, dbPath, &db);
    // EXPECT_TRUE(status.ok());
    // delete db;

    StorageEngine *storage;

    EXPECT_NO_THROW(
        storage = new StorageEngine(dbPath);
        sleep(1);
    );
    
#ifdef HAVE_BOOST_FILESYSTEM
    // check DB folder exists
    EXPECT_TRUE(boost::filesystem::exists(dbPath));
    boost::filesystem::remove_all(dbPath);
#endif

    delete storage;
}

TEST(TestDb, TestPrefixMatch)
{
    // setup db
    StorageEngine storage(dbPath);

    Name n("/hello-ndn/rtc-stream/ndnrtc/%FD%03/video/camera/_meta");

    // put data into storage
    for (int version = 0; version < 3; ++version)
    {
        std::stringstream ss;
        ss << "test-data-" << version;

        Name packetName(n);
        packetName.appendVersion(version).appendSegment(0);

        Data testData(packetName);
        
        testData.setContent((uint8_t*)ss.str().c_str(), ss.str().size());
        storage.put(testData);
    }

    // retieve data by prefix
    { // test simple retrieval by prefix
        Interest i(n);
        std::shared_ptr<Data> d = storage.read(i);

        EXPECT_TRUE(d.get());
        EXPECT_EQ(d->getName()[-2].toVersion(), 2);
    }

    {
        Interest i(n);
        i.setCanBePrefix(false);

        std::shared_ptr<Data> d = storage.read(i);

        EXPECT_FALSE(d.get());
    }

    {
        Interest i(n);
        i.setCanBePrefix(true);
        i.setMaxSuffixComponents(2);

        std::shared_ptr<Data> d = storage.read(i);

        EXPECT_TRUE(d.get());
        EXPECT_EQ(d->getName()[-2].toVersion(), 2);
    }

    {
        Interest i(Name("/hello-ndn/rtc-stream/ndnrtc/%FD%03/video/camera"));
        i.setCanBePrefix(true);
        i.setMaxSuffixComponents(2);
        
        std::shared_ptr<Data> d = storage.read(i);

        EXPECT_FALSE(d.get());
    }

#ifdef HAVE_BOOST_FILESYSTEM
    EXPECT_TRUE(boost::filesystem::exists(dbPath));
    boost::filesystem::remove_all(dbPath);
#endif
}

void handler(int sig)
{
    void *array[10];
    size_t size;

    if (sig == SIGABRT || sig == SIGSEGV)
    {
        fprintf(stderr, "Received signal %d:\n", sig);
        // get void*'s for all entries on the stack
        size = backtrace(array, 10);
        // print out all the frames to stderr
        backtrace_symbols_fd(array, size, STDERR_FILENO);
        exit(1);
    }
}

int main(int argc, char **argv) {
    signal(SIGABRT, handler);
    signal(SIGSEGV, handler);
    signal(SIGINT, &handler);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
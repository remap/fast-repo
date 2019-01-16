//
// ndnrtc-pattern.cpp
//
//  Created by Peter Gusev on 2 November 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "ndnrtc-pattern.hpp"

#include <ndnrtc/stream-recorder.hpp>
#include <ndnrtc/simple-log.hpp>
#include <ndn-cpp/face.hpp>
#include <ndn-cpp/security/key-chain.hpp>
#include <nlohmann/json.hpp>

#if HAVE_LIBNDNRTC
// using namespace ndnrtc;
using namespace ndn;
using namespace std;
using namespace ndnrtc;
using namespace fast_repo;
using namespace ndnlog::new_api;
using json = nlohmann::json;

using boost::shared_ptr;
using boost::make_shared;

void
NdnrtcPattern::fetch(const Name& prefix)
{
    Logger::initAsyncLogging();
    Logger::getLoggerPtr("")->setLogLevel(ndnlog::NdnLoggerDetailLevelNone);

    NamespaceInfo ninfo;
    
    if (NameComponents::extractInfo(prefix, ninfo))
    {
        StoreData storeFun = storePacketFun_;
        streamRecorder_ = make_shared<StreamRecorder>(
            [storeFun](const boost::shared_ptr<const ndn::Data> &d){ storeFun(*d); },
            ninfo, 
            make_shared<Face>(face_), 
            make_shared<KeyChain>(keyChain_));
        streamRecorder_->setLogger(Logger::getLoggerPtr(""));
        
        std::cout << "will fetch ndnrtc stream " << ninfo.getPrefix(prefix_filter::Stream) << std::endl;
        streamRecorder_->start(StreamRecorder::Default);
        // return true;
    }
    else
        // TODO: update interface to return boolean here
        return; //false;
}

void
NdnrtcPattern::cancel()
{
    if (streamRecorder_)
    {
        streamRecorder_->stop();
        std::cout << "stopped fetching ndnrtc stream:" << streamRecorder_->getStreamPrefix() << std::endl;

        streamRecorder_.reset();
    }
}

std::string 
NdnrtcPattern::getStatusReport() const
{
    StreamRecorder::Stats streamStats = streamRecorder_->getCurrentStats();
    json status;
    
    status["manifests"] = streamStats.manifestsStored_;
    status["meta"]["stream"] = streamStats.streamMetaStored_;
    status["meta"]["thread"] = streamStats.threadMetaStored_;
    status["key"]["requested_no"] = streamStats.latestKeyRequested_;
    status["key"]["received_no"] = streamStats.latestKeyFetched_;
    status["key"]["fetched"] = streamStats.keyStored_;
    status["key"]["failed"] = streamStats.keyFailed_;
    status["delta"]["requested_no"] = streamStats.latestDeltaRequested_;
    status["delta"]["received_no"] = streamStats.latestDeltaFetched_;
    status["delta"]["fetched"] = streamStats.deltaStored_;
    status["delta"]["failed"] = streamStats.deltaFailed_;
    status["frames_pending"] = streamStats.pendingFrames_;

    return status.dump();
}

#endif
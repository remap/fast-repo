//
// ndnrtc-pattern.hpp
//
//  Created by Peter Gusev on 2 November 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#ifndef __ndnrtc_pattern_hpp__
#define __ndnrtc_pattern_hpp__

#include "base-pattern.hpp"

#if HAVE_LIBNDNRTC

#include <ndnrtc/name-components.hpp>

namespace ndnrtc {
    class StreamRecorder;
}

namespace fast_repo {

using std::shared_ptr;
using std::make_shared;

class NdnrtcPattern : public BasePattern {
public:
    NdnrtcPattern(ndn::Face& face, ndn::KeyChain& keyChain, StoreData storePacketFun):
        BasePattern(face, keyChain, storePacketFun) {}

    static const ndn::Name getPatternKeyword()
    {
        // TODO: add version number from the actual library
        std::stringstream patternId;
        patternId << "ndnrtc" << ndnrtc::NameComponents::nameApiVersion();

        return patternId.str();
    }

    static shared_ptr<IFetchPattern> create(ndn::Face& face,
                                                 ndn::KeyChain & keyChain,
                                                 StoreData storePacketFun)
    {
        return std::make_shared<NdnrtcPattern>(face, keyChain, storePacketFun);
    }

    void fetch(const ndn::Name& prefix) override;
    void cancel() override;
    std::string getStatusReport() const;

private:
    // TODO: support multiple recorders
    shared_ptr<ndnrtc::StreamRecorder> streamRecorder_;
};

}

#endif
#endif

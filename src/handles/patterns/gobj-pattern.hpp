//
// gobj-pattern.hpp
//
//  Created by Peter Gusev on 9 March 2019.
//  Copyright 2019 Regents of the University of California
//

#ifndef __gobj_pattern_hpp__
#define __gobj_pattern_hpp__

#include "base-pattern.hpp"

#if HAVE_LIBCNL_CPP

namespace cnl_cpp {
    class Namespace;
}

namespace fast_repo {
    using std::shared_ptr;

    class GeneralizedObjectPattern : public BasePattern {
    public:
        GeneralizedObjectPattern(ndn::Face& face, ndn::KeyChain& keyChain, StoreData storePacketFun):
            BasePattern(face, keyChain, storePacketFun) {}

        static const ndn::Name getPatternKeyword()
        {
            return "gobj";
        }

        static shared_ptr<IFetchPattern> create(ndn::Face& face,
                                                ndn::KeyChain& keyChain,
                                                StoreData storePacketFun)
        {
            return std::make_shared<GeneralizedObjectPattern>(face, keyChain, storePacketFun);
        }

        void fetch(const ndn::Name& prefix) override;
        void cancel() override;
        std::string getStatusReport() const;

    private:
        std::map<ndn::Name, shared_ptr<cnl_cpp::Namespace>> namespaces_;
    };
}

#endif
#endif

//
// gobj-pattern.cpp
//
//  Created by Peter Gusev on 9 March 2019.
//  Copyright 2019 Regents of the University of California
//

#include "gobj-pattern.hpp"

#if HAVE_LIBCNL_CPP

#include <cnl-cpp/generalized-object/generalized-object-handler.hpp>

using namespace std;
using namespace ndn;
using namespace cnl_cpp;
using namespace fast_repo;

using std::shared_ptr;
using std::make_shared;

void
GeneralizedObjectPattern::fetch(const Name& prefix)
{
    shared_ptr<Namespace> gobj = std::make_shared<Namespace>(prefix);
    gobj->setFace(&face_);

    auto onObject = [&]
          (const ptr_lib::shared_ptr<ContentMetaInfoObject>& contentMetaInfo,
           Namespace& objectNamespace) {
               vector<shared_ptr<Data>> dataList;
               objectNamespace.getAllData(dataList);

               cout << "Got generalized object " << objectNamespace.getName() <<
                 ", content-type " << contentMetaInfo->getContentType() << endl;

               for (auto& d:dataList)
               {
                   cout << "store " << d->getName() << endl;
                   storePacketFun_(*d);
               }

               cout << "there are " << this->namespaces_.size() << " pending gobj fetchings" << endl;
               this->namespaces_.erase(objectNamespace.getName());
               cout << this->namespaces_.size() << endl;

        };

    // TODO: update the code below to the latest CNL
    // auto handler = std::make_shared<GeneralizedObjectHandler>(onObject);
    // TODO: this parameter must be set by requester
    // Allow one component after the prefix for the <version>.
    // handler->setNComponentsAfterObjectNamespace(1);
    // TODO: this parameter must be set by requester
    // gobj->setHandler(handler).objectNeeded(true);
    // namespaces_[prefix] = gobj;
}

void
GeneralizedObjectPattern::cancel()
{
    // TODO
}

std::string
GeneralizedObjectPattern::getStatusReport() const
{
    // TODO: do we need this?
    return "n/a";
}

#endif

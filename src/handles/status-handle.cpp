//
// status-handle.cpp
//
//  Created by Peter Gusev on 4 December 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "status-handle.hpp"

#include <cnl-cpp/namespace.hpp>
#include <nlohmann/json.hpp>

using namespace ndn;
using namespace fast_repo;
using namespace cnl_cpp;
using json=nlohmann::json;
using std::shared_ptr;
using std::make_shared;

StatusHandle::StatusHandle(ndn::Face &face, StorageEngine &storage, ndn::KeyChain &keyChain)
    : BaseHandle(face, storage, keyChain) { }

void StatusHandle::listen(const ndn::Name &prefix)
{
    statusNamespace_ = make_shared<Namespace>(Name(prefix).append("status"), &getKeyChain());
    statusNamespace_->setFace(&getFace(), [](const shared_ptr<const Name>& prefix){
        std::cerr << "Register failed for prefix " << prefix << std::endl;
    });

    MetaInfo metaInfo;
    metaInfo.setFreshnessPeriod(100);

    auto onObjectNeeded = [metaInfo, this]
        (Namespace& nameSpace, Namespace& neededNamespace, uint64_t callbackId) {
            if (&neededNamespace != statusNamespace_.get())
                return false;

            Namespace& versionedNamespace = (*statusNamespace_)
                [Name::Component::fromVersion((uint64_t)ndn_getNowMilliseconds())];
            versionedNamespace.setNewDataMetaInfo(metaInfo);

            handler_.setObject(versionedNamespace,
                              Blob::fromRawStr(publishStatus()),
                              "application/json");
            return true;
        };

    statusNamespace_->addOnObjectNeeded(onObjectNeeded);
}

void StatusHandle::addStatusReportSource(GetStatusReport getStatusReportFun)
{
    statusReportSources_.push_back(getStatusReportFun);
}

std::string StatusHandle::publishStatus()
{
    json status;
    
    for (auto reportSource : statusReportSources_)
    {
        try {
            std::pair<std::string, std::string> reportData = reportSource();
            status[reportData.first] = json::parse(reportData.second);
        }
        catch (std::runtime_error& e)
        {
            std::cerr << "Error while polling status report: " << e.what() << std::endl;
        }
    }

    return status.dump();
}
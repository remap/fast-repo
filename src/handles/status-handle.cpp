//
// status-handle.cpp
//
//  Created by Peter Gusev on 4 December 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#include "status-handle.hpp"

#include <cnl-cpp/namespace.hpp>
#include <cnl-cpp/generalized-object/generalized-object-handler.hpp>
#include <nlohmann/json.hpp>

using namespace ndn;
using namespace fast_repo;
using namespace cnl_cpp;
using json=nlohmann::json;
using boost::shared_ptr;
using boost::make_shared;

StatusHandle::StatusHandle(ndn::Face &face, StorageEngine &storage, ndn::KeyChain &keyChain)
    : BaseHandle(face, storage, keyChain) { }

void StatusHandle::listen(const ndn::Name &prefix)
{
    statusNamespace_ = make_shared<Namespace>(Name(prefix).append("status"));
    statusNamespace_->setFace(&getFace(), [](const shared_ptr<const Name>& prefix){
        std::cerr << "Register failed for prefix " << prefix << std::endl;
    });
    statusNamespace_->addOnObjectNeeded([this](Namespace&, Namespace& neededNamespace, uint64_t)
    {
        publishStatus();
        // neededNamespace.serializeObject();
        return false;
    });
}

void StatusHandle::addStatusReportSource(GetStatusReport getStatusReportFun)
{
    statusReportSources_.push_back(getStatusReportFun);
}

void StatusHandle::publishStatus()
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

    std::cout << "status:" << std::endl << status.dump() << std::endl;
    // GeneralizedObjectHandler().setObject(*statusNamespace_, 
    //                                      Blob::fromRawStr("status ok"), 
    //                                      "text/html");
}
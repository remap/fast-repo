//
// status-handle.hpp
//
//  Created by Peter Gusev on 4 December 2018.
//  Copyright 2018-2019 Regents of the University of California
//

#ifndef __status_handle_hpp__
#define __status_handle_hpp__

#include "base-handle.hpp"

namespace ndn 
{
class Face;
class KeyChain;
class InterestFilter;
}

namespace cnl_cpp
{
class Namespace;
}

namespace fast_repo 
{

using boost::shared_ptr;

class StatusHandle : public repo_ng::BaseHandle 
{
    typedef std::function<std::pair<std::string,std::string>()> GetStatusReport;

  public: 
    StatusHandle(ndn::Face &face, StorageEngine &storage, ndn::KeyChain &keyChain);

    void listen(const ndn::Name &prefix);

    /**
     * Adds status report source function.
     * Status handle will poll each status report function and add returned 
     * pair as key and value of JSON dictionary.
     * NOTE: value should be a valid JSON dictionary.
     * @param getStatusReportFun GetStatusReport function that returns a pair:
     *      module name and status report JSON dictionary.
     */ 
    void addStatusReportSource(GetStatusReport getStatusReportFun);

  private:
    shared_ptr<cnl_cpp::Namespace> statusNamespace_;
    std::vector<GetStatusReport> statusReportSources_;

    void publishStatus();
};

}

#endif

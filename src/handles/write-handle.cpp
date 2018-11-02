/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017, Regents of the University of California.
 *
 * This file is part of NDN repo-ng (Next generation of NDN repository).
 * See AUTHORS.md for complete list of repo-ng authors and contributors.
 *
 * repo-ng is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * repo-ng is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * repo-ng, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "write-handle.hpp"

using std::bind;
using std::shared_ptr;
using std::make_shared;

namespace repo_ng {

WriteHandle::WriteHandle(ndn::Face &face, RepoStorage &storageHandle, ndn::KeyChain &keyChain
                         /*,ndn::Validator& validator*/)
  : BaseHandle(face, storageHandle, keyChain)
  //, m_validator(validator)
{
}

void
WriteHandle::listen(const ndn::Name& prefix)
{
  getFace().setInterestFilter(ndn::Name(prefix).append("insert"),
                              bind(&WriteHandle::onInterest, this, _1, _2, _3, _4, _5));
}

void
WriteHandle::onInterest(const shared_ptr<const ndn::Name> &prefix,
                        const shared_ptr<const ndn::Interest> &interest, ndn::Face &face,
                        uint64_t interestFilterId,
                        const shared_ptr<const ndn::InterestFilter> &filter)
{
  // TODO: Set up validator
  // m_validator.validate(interest, )
  this->onValidated(*interest, *prefix);
}

void
WriteHandle::onValidated(const ndn::Interest& interest, const ndn::Name &prefix)
{
  ndn_message::RepoCommandParameterMessage parameter;
  try{
      extractParameter(interest, prefix, parameter);
  }
  catch (std::exception& e){
    negativeReply(interest, 403);
    return;
  }

  // TODO: Segmented
  processSingleInsertCommand(interest, parameter);
}

void
WriteHandle::processSingleInsertCommand(const ndn::Interest& interest, ndn_message::RepoCommandParameterMessage& parameter)
{
  uint64_t processId = generateProcessId();

  ProcessInfo& process = m_processes[processId];

  auto response = process.response.mutable_repo_command_response();
  response->set_status_code(100);
  response->set_process_id(processId);
  response->set_insert_num(0);

  reply(interest, process.response);

  response->set_status_code(300);

  ndn::Name fetchName;
  for(int i = 0, sz = parameter.repo_command_parameter().name().component_size(); i < sz; i ++){
    fetchName.append(parameter.repo_command_parameter().name().component(i));
  }

  ndn::Interest fetchInterest(fetchName);
  fetchInterest.setInterestLifetimeMilliseconds(4000.0);
  getFace().expressInterest(fetchInterest,
                            bind(&WriteHandle::onData, this, _1, _2, processId),
                            bind(&WriteHandle::onTimeout, this, _1, processId),
                            bind(&WriteHandle::onTimeout, this, _1, processId)); //Nack
}

void
WriteHandle::onTimeout(const shared_ptr<const ndn::Interest>& interest, uint64_t processId)
{
  std::cerr << "Timeout" << std::endl;
  m_processes.erase(processId);
}

void
WriteHandle::onData(const shared_ptr<const ndn::Interest>& interest, const shared_ptr<ndn::Data>& data, uint64_t processId)
{
  //TODO: Validate data
  onDataValidated(*interest, *data, processId);
}

void
WriteHandle::onDataValidated(const ndn::Interest& interest, const ndn::Data& data, uint64_t processId)
{
  if (m_processes.count(processId) == 0) {
    return;
  }

  ProcessInfo& process = m_processes[processId];
  auto& response = process.response;

  if (response.repo_command_response().insert_num() == 0) {
    getStorageHandle().put(data);
    response.mutable_repo_command_response()->set_insert_num(1);
    this->onDataInsertion(data.getName());
  }

  //deferredDeleteProcess(processId);
  // TODO: Wait for a certain time and delete it.
  m_processes.erase(processId);
}

} // namespace repo
#if 0
namespace repo {

static const int RETRY_TIMEOUT = 3;
static const int DEFAULT_CREDIT = 12;
static const milliseconds NOEND_TIMEOUT(10000);
static const milliseconds PROCESS_DELETE_TIME(10000);
static const milliseconds DEFAULT_INTEREST_LIFETIME(4000);

WriteHandle::WriteHandle(Face& face, RepoStorage& storageHandle, KeyChain& keyChain,
                         Scheduler& scheduler,
                         Validator& validator)
  : BaseHandle(face, storageHandle, keyChain, scheduler)
  , m_validator(validator)
  , m_retryTime(RETRY_TIMEOUT)
  , m_credit(DEFAULT_CREDIT)
  , m_noEndTimeout(NOEND_TIMEOUT)
  , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
{
}

void
WriteHandle::deleteProcess(ProcessId processId)
{
  m_processes.erase(processId);
}

// Interest.
void
WriteHandle::onInterest(const Name& prefix, const Interest& interest)
{
  m_validator.validate(interest,
                       bind(&WriteHandle::onValidated, this, _1, prefix),
                       bind(&WriteHandle::onValidationFailed, this, _1, _2));
}

void
WriteHandle::onValidated(const Interest& interest, const Name& prefix)
{
  //m_validResult = 1;
  RepoCommandParameter parameter;
  try {
    extractParameter(interest, prefix, parameter);
  }
  catch (RepoCommandParameter::Error) {
    negativeReply(interest, 403);
    return;
  }

  if (parameter.hasStartBlockId() || parameter.hasEndBlockId()) {
    if (parameter.hasSelectors()) {
      negativeReply(interest, 402);
      return;
    }
    processSegmentedInsertCommand(interest, parameter);
  }
  else {
    processSingleInsertCommand(interest, parameter);
  }
  if (parameter.hasInterestLifetime())
    m_interestLifetime = parameter.getInterestLifetime();
}

void
WriteHandle::onValidationFailed(const Interest& interest, const ValidationError& error)
{
  std::cerr << error << std::endl;
  negativeReply(interest, 401);
}

void
WriteHandle::onData(const Interest& interest, const Data& data, ProcessId processId)
{
  m_validator.validate(data,
                       bind(&WriteHandle::onDataValidated, this, interest, _1, processId),
                       bind(&WriteHandle::onDataValidationFailed, this, _1, _2));
}

void
WriteHandle::onDataValidated(const Interest& interest, const Data& data, ProcessId processId)
{
  if (m_processes.count(processId) == 0) {
    return;
  }

  ProcessInfo& process = m_processes[processId];
  RepoCommandResponse& response = process.response;

  if (response.getInsertNum() == 0) {
    getStorageHandle().insertData(data);
   // getStorageHandle().insertEntry(data);
   // getStoreIndex().insert(data);
    response.setInsertNum(1);
  }

  deferredDeleteProcess(processId);
}

void
WriteHandle::onDataValidationFailed(const Data& data, const ValidationError& error)
{
  std::cerr << error << std::endl;
}

void
WriteHandle::onSegmentData(const Interest& interest, const Data& data, ProcessId processId)
{
  m_validator.validate(data,
                       bind(&WriteHandle::onSegmentDataValidated, this, interest, _1, processId),
                       bind(&WriteHandle::onDataValidationFailed, this, _1, _2));
}

void
WriteHandle::onSegmentDataValidated(const Interest& interest, const Data& data, ProcessId processId)
{
  if (m_processes.count(processId) == 0) {
    return;
  }
  RepoCommandResponse& response = m_processes[processId].response;

  //refresh endBlockId
  Name::Component finalBlockId = data.getFinalBlockId();

  if (!finalBlockId.empty()) {
    SegmentNo final = finalBlockId.toSegment();
    if (response.hasEndBlockId()) {
      if (final < response.getEndBlockId()) {
        response.setEndBlockId(final);
      }
    }
    else {
      response.setEndBlockId(final);
    }
  }

  //insert data
  if (getStorageHandle().insertData(data)) {
    response.setInsertNum(response.getInsertNum() + 1);
  }

  onSegmentDataControl(processId, interest);
}

void
WriteHandle::onTimeout(const Interest& interest, ProcessId processId)
{
  std::cerr << "Timeout" << std::endl;
  m_processes.erase(processId);
}

void
WriteHandle::onSegmentTimeout(const Interest& interest, ProcessId processId)
{
  std::cerr << "SegTimeout" << std::endl;

  onSegmentTimeoutControl(processId, interest);
}

void
WriteHandle::listen(const Name& prefix)
{
  getFace().setInterestFilter(Name(prefix).append("insert"),
                              bind(&WriteHandle::onInterest, this, _1, _2));
  getFace().setInterestFilter(Name(prefix).append("insert check"),
                              bind(&WriteHandle::onCheckInterest, this, _1, _2));
}

void
WriteHandle::segInit(ProcessId processId, const RepoCommandParameter& parameter)
{
  ProcessInfo& process = m_processes[processId];
  process.credit = 0;

  map<SegmentNo, int>& processRetry = process.retryCounts;

  Name name = parameter.getName();
  SegmentNo startBlockId = parameter.getStartBlockId();

  uint64_t initialCredit = m_credit;

  if (parameter.hasEndBlockId()) {
    initialCredit =
      std::min(initialCredit, parameter.getEndBlockId() - parameter.getStartBlockId() + 1);
  }
  else {
    // set noEndTimeout timer
    process.noEndTime = ndn::time::steady_clock::now() +
                        m_noEndTimeout;
  }
  process.credit = initialCredit;
  SegmentNo segment = startBlockId;

  for (; segment < startBlockId + initialCredit; ++segment) {
    Name fetchName = name;
    fetchName.appendSegment(segment);
    Interest interest(fetchName);
    interest.setInterestLifetime(m_interestLifetime);
    getFace().expressInterest(interest,
                              bind(&WriteHandle::onSegmentData, this, _1, _2, processId),
                              bind(&WriteHandle::onSegmentTimeout, this, _1, processId), // Nack
                              bind(&WriteHandle::onSegmentTimeout, this, _1, processId));
    process.credit--;
    processRetry[segment] = 0;
  }

  queue<SegmentNo>& nextSegmentQueue = process.nextSegmentQueue;

  process.nextSegment = segment;
  nextSegmentQueue.push(segment);
}

void
WriteHandle::onSegmentDataControl(ProcessId processId, const Interest& interest)
{
  if (m_processes.count(processId) == 0) {
    return;
  }
  ProcessInfo& process = m_processes[processId];
  RepoCommandResponse& response = process.response;
  int& processCredit = process.credit;
  //onSegmentDataControl is called when a data returns.
  //When data returns, processCredit++
  processCredit++;
  SegmentNo& nextSegment = process.nextSegment;
  queue<SegmentNo>& nextSegmentQueue = process.nextSegmentQueue;
  map<SegmentNo, int>& retryCounts = process.retryCounts;

  //read whether notime timeout
  if (!response.hasEndBlockId()) {

    ndn::time::steady_clock::TimePoint& noEndTime = process.noEndTime;
    ndn::time::steady_clock::TimePoint now = ndn::time::steady_clock::now();

    if (now > noEndTime) {
      std::cerr << "noEndtimeout: " << processId << std::endl;
      //m_processes.erase(processId);
      //StatusCode should be refreshed as 405
      response.setStatusCode(405);
      //schedule a delete event
      deferredDeleteProcess(processId);
      return;
    }
  }

  //read whether this process has total ends, if ends, remove control info from the maps
  if (response.hasEndBlockId()) {
    uint64_t nSegments =
      response.getEndBlockId() - response.getStartBlockId() + 1;
    if (response.getInsertNum() >= nSegments) {
      //m_processes.erase(processId);
      //All the data has been inserted, StatusCode is refreshed as 200
      response.setStatusCode(200);
      deferredDeleteProcess(processId);
      return;
    }
  }

  //check whether there is any credit
  if (processCredit == 0)
    return;


  //check whether sent queue empty
  if (nextSegmentQueue.empty()) {
    //do not do anything
    return;
  }

  //pop the queue
  SegmentNo sendingSegment = nextSegmentQueue.front();
  nextSegmentQueue.pop();

  //check whether sendingSegment exceeds
  if (response.hasEndBlockId() && sendingSegment > response.getEndBlockId()) {
    //do not do anything
    return;
  }

  //read whether this is retransmitted data;
  SegmentNo fetchedSegment =
    interest.getName().get(interest.getName().size() - 1).toSegment();

  BOOST_ASSERT(retryCounts.count(fetchedSegment) != 0);

  //find this fetched data, remove it from this map
  //rit->second.erase(oit);
  retryCounts.erase(fetchedSegment);
  //express the interest of the top of the queue
  Name fetchName(interest.getName().getPrefix(-1));
  fetchName.appendSegment(sendingSegment);
  Interest fetchInterest(fetchName);
  fetchInterest.setInterestLifetime(m_interestLifetime);
  getFace().expressInterest(fetchInterest,
                            bind(&WriteHandle::onSegmentData, this, _1, _2, processId),
                            bind(&WriteHandle::onSegmentTimeout, this, _1, processId), // Nack
                            bind(&WriteHandle::onSegmentTimeout, this, _1, processId));
  //When an interest is expressed, processCredit--
  processCredit--;
  if (retryCounts.count(sendingSegment) == 0) {
    //not found
    retryCounts[sendingSegment] = 0;
  }
  else {
    //found
    retryCounts[sendingSegment] = retryCounts[sendingSegment] + 1;
  }
  //increase the next seg and put it into the queue
  if (!response.hasEndBlockId() || (nextSegment + 1) <= response.getEndBlockId()) {
    nextSegment++;
    nextSegmentQueue.push(nextSegment);
  }
}

void
WriteHandle::onSegmentTimeoutControl(ProcessId processId, const Interest& interest)
{
  if (m_processes.count(processId) == 0) {
    return;
  }
  ProcessInfo& process = m_processes[processId];
  // RepoCommandResponse& response = process.response;
  // SegmentNo& nextSegment = process.nextSegment;
  // queue<SegmentNo>& nextSegmentQueue = process.nextSegmentQueue;
  map<SegmentNo, int>& retryCounts = process.retryCounts;

  SegmentNo timeoutSegment = interest.getName().get(-1).toSegment();

  std::cerr << "timeoutSegment: " << timeoutSegment << std::endl;

  BOOST_ASSERT(retryCounts.count(timeoutSegment) != 0);

  //read the retry time. If retry out of time, fail the process. if not, plus
  int& retryTime = retryCounts[timeoutSegment];
  if (retryTime >= m_retryTime) {
    //fail this process
    std::cerr << "Retry timeout: " << processId << std::endl;
    m_processes.erase(processId);
    return;
  }
  else {
    //Reput it in the queue, retryTime++
    retryTime++;
    Interest retryInterest(interest.getName());
    retryInterest.setInterestLifetime(m_interestLifetime);
    getFace().expressInterest(retryInterest,
                              bind(&WriteHandle::onSegmentData, this, _1, _2, processId),
                              bind(&WriteHandle::onSegmentTimeout, this, _1, processId), // Nack
                              bind(&WriteHandle::onSegmentTimeout, this, _1, processId));
  }

}

void
WriteHandle::onCheckInterest(const Name& prefix, const Interest& interest)
{
  m_validator.validate(interest,
                       bind(&WriteHandle::onCheckValidated, this, _1, prefix),
                       bind(&WriteHandle::onCheckValidationFailed, this, _1, _2));

}

void
WriteHandle::onCheckValidated(const Interest& interest, const Name& prefix)
{
  RepoCommandParameter parameter;
  try {
    extractParameter(interest, prefix, parameter);
  }
  catch (RepoCommandParameter::Error) {
    negativeReply(interest, 403);
    return;
  }

  if (!parameter.hasProcessId()) {
    negativeReply(interest, 403);
    return;
  }
  //check whether this process exists
  ProcessId processId = parameter.getProcessId();
  if (m_processes.count(processId) == 0) {
    std::cerr << "no such processId: " << processId << std::endl;
    negativeReply(interest, 404);
    return;
  }

  ProcessInfo& process = m_processes[processId];

  RepoCommandResponse& response = process.response;

  //Check whether it is single data fetching
  if (!response.hasStartBlockId() &&
      !response.hasEndBlockId()) {
    reply(interest, response);
    return;
  }

  //read if noEndtimeout
  if (!response.hasEndBlockId()) {
    extendNoEndTime(process);
    reply(interest, response);
    return;
  }
  else {
    reply(interest, response);
  }
}

void
WriteHandle::onCheckValidationFailed(const Interest& interest, const ValidationError& error)
{
  std::cerr << error << std::endl;
  negativeReply(interest, 401);
}

void
WriteHandle::deferredDeleteProcess(ProcessId processId)
{
  getScheduler().scheduleEvent(PROCESS_DELETE_TIME,
                               bind(&WriteHandle::deleteProcess, this, processId));
}

void
WriteHandle::processSingleInsertCommand(const Interest& interest,
                                        RepoCommandParameter& parameter)
{
  ProcessId processId = generateProcessId();

  ProcessInfo& process = m_processes[processId];

  RepoCommandResponse& response = process.response;
  response.setStatusCode(100);
  response.setProcessId(processId);
  response.setInsertNum(0);

  reply(interest, response);

  response.setStatusCode(300);

  Interest fetchInterest(parameter.getName());
  fetchInterest.setInterestLifetime(m_interestLifetime);
  if (parameter.hasSelectors()) {
    fetchInterest.setSelectors(parameter.getSelectors());
  }
  getFace().expressInterest(fetchInterest,
                            bind(&WriteHandle::onData, this, _1, _2, processId),
                            bind(&WriteHandle::onTimeout, this, _1, processId), // Nack
                            bind(&WriteHandle::onTimeout, this, _1, processId));
}

void
WriteHandle::processSegmentedInsertCommand(const Interest& interest,
                                           RepoCommandParameter& parameter)
{
  if (parameter.hasEndBlockId()) {
    //normal fetch segment
    if (!parameter.hasStartBlockId()) {
      parameter.setStartBlockId(0);
    }

    SegmentNo startBlockId = parameter.getStartBlockId();
    SegmentNo endBlockId = parameter.getEndBlockId();
    if (startBlockId > endBlockId) {
      negativeReply(interest, 403);
      return;
    }

    ProcessId processId = generateProcessId();
    ProcessInfo& process = m_processes[processId];
    RepoCommandResponse& response = process.response;
    response.setStatusCode(100);
    response.setProcessId(processId);
    response.setInsertNum(0);
    response.setStartBlockId(startBlockId);
    response.setEndBlockId(endBlockId);

    reply(interest, response);

    //300 means data fetching is in progress
    response.setStatusCode(300);

    segInit(processId, parameter);
  }
  else {
    //no EndBlockId, so fetch FinalBlockId in data, if timeout, stop
    ProcessId processId = generateProcessId();
    ProcessInfo& process = m_processes[processId];
    RepoCommandResponse& response = process.response;
    response.setStatusCode(100);
    response.setProcessId(processId);
    response.setInsertNum(0);
    response.setStartBlockId(parameter.getStartBlockId());
    reply(interest, response);

    //300 means data fetching is in progress
    response.setStatusCode(300);

    segInit(processId, parameter);
  }
}

void
WriteHandle::extendNoEndTime(ProcessInfo& process)
{
  ndn::time::steady_clock::TimePoint& noEndTime = process.noEndTime;
  ndn::time::steady_clock::TimePoint now = ndn::time::steady_clock::now();
  RepoCommandResponse& response = process.response;
  if (now > noEndTime) {
    response.setStatusCode(405);
    return;
  }
  //extends noEndTime
  process.noEndTime =
    ndn::time::steady_clock::now() + m_noEndTimeout;

}

void
WriteHandle::negativeReply(const Interest& interest, int statusCode)
{
  RepoCommandResponse response;
  response.setStatusCode(statusCode);
  reply(interest, response);
}

} // namespace repo
#endif
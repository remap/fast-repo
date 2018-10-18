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

#include "watch-handle.hpp"
#if 0
namespace repo {

static const milliseconds PROCESS_DELETE_TIME(10000);
static const milliseconds DEFAULT_INTEREST_LIFETIME(4000);

WatchHandle::WatchHandle(Face& face, RepoStorage& storageHandle, KeyChain& keyChain,
                         Scheduler& scheduler, Validator& validator)
  : BaseHandle(face, storageHandle, keyChain, scheduler)
  , m_validator(validator)
  , m_interestNum(0)
  , m_maxInterestNum(0)
  , m_interestLifetime(DEFAULT_INTEREST_LIFETIME)
  , m_watchTimeout(0)
  , m_startTime(steady_clock::now())
  , m_size(0)
{
}

void
WatchHandle::deleteProcess(const Name& name)
{
  m_processes.erase(name);
}

// Interest.
void
WatchHandle::onInterest(const Name& prefix, const Interest& interest)
{
  m_validator.validate(interest,
                       bind(&WatchHandle::onValidated, this, _1, prefix),
                       bind(&WatchHandle::onValidationFailed, this, _1, _2));
}

void
WatchHandle::onValidated(const Interest& interest, const Name& prefix)
{
  RepoCommandParameter parameter;
  try {
    extractParameter(interest, prefix, parameter);
  }
  catch (RepoCommandParameter::Error) {
    negativeReply(interest, 403);
    return;
  }

  processWatchCommand(interest, parameter);
}

void WatchHandle::watchStop(const Name& name)
{
  m_processes[name].second = false;
  m_maxInterestNum = 0;
  m_interestNum = 0;
  m_startTime = steady_clock::now();
  m_watchTimeout = milliseconds(0);
  m_interestLifetime = DEFAULT_INTEREST_LIFETIME;
  m_size = 0;
}

void
WatchHandle::onValidationFailed(const Interest& interest, const ValidationError& error)
{
  std::cerr << error << std::endl;
  negativeReply(interest, 401);
}

void
WatchHandle::onData(const Interest& interest, const ndn::Data& data, const Name& name)
{
  m_validator.validate(data,
                       bind(&WatchHandle::onDataValidated, this, interest, _1, name),
                       bind(&WatchHandle::onDataValidationFailed, this, interest, _1, _2, name));
}

void
WatchHandle::onDataValidated(const Interest& interest, const Data& data, const Name& name)
{
  if (!m_processes[name].second) {
    return;
  }
  if (getStorageHandle().insertData(data)) {
    m_size++;
    if (!onRunning(name))
      return;

    Interest fetchInterest(interest.getName());
    fetchInterest.setSelectors(interest.getSelectors());
    fetchInterest.setInterestLifetime(m_interestLifetime);
    fetchInterest.setChildSelector(1);

    // update selectors
    // if data name is equal to interest name, use MinSuffixComponents selecor to exclude this data
    if (data.getName().size() == interest.getName().size()) {
      fetchInterest.setMinSuffixComponents(2);
    }
    else {
      Exclude exclude;
      if (!interest.getExclude().empty()) {
        exclude = interest.getExclude();
      }

      exclude.excludeBefore(data.getName()[interest.getName().size()]);
      fetchInterest.setExclude(exclude);
    }

    ++m_interestNum;
    getFace().expressInterest(fetchInterest,
                              bind(&WatchHandle::onData, this, _1, _2, name),
                              bind(&WatchHandle::onTimeout, this, _1, name), // Nack
                              bind(&WatchHandle::onTimeout, this, _1, name));
  }
  else {
    BOOST_THROW_EXCEPTION(Error("Insert into Repo Failed"));
  }
  m_processes[name].first.setInsertNum(m_size);
}

void
WatchHandle::onDataValidationFailed(const Interest& interest, const Data& data,
                                    const ValidationError& error, const Name& name)
{
  std::cerr << error << std::endl;
  if (!m_processes[name].second) {
    return;
  }
  if (!onRunning(name))
    return;

  Interest fetchInterest(interest.getName());
  fetchInterest.setSelectors(interest.getSelectors());
  fetchInterest.setInterestLifetime(m_interestLifetime);
  fetchInterest.setChildSelector(1);

  // update selectors
  // if data name is equal to interest name, use MinSuffixComponents selecor to exclude this data
  if (data.getName().size() == interest.getName().size()) {
    fetchInterest.setMinSuffixComponents(2);
  }
  else {
    Exclude exclude;
    if (!interest.getExclude().empty()) {
      exclude = interest.getExclude();
    }
    // Only exclude this data since other data whose names are smaller may be validated and satisfied
    exclude.excludeBefore(data.getName()[interest.getName().size()]);
    fetchInterest.setExclude(exclude);
  }

  ++m_interestNum;
  getFace().expressInterest(fetchInterest,
                            bind(&WatchHandle::onData, this, _1, _2, name),
                            bind(&WatchHandle::onTimeout, this, _1, name), // Nack
                            bind(&WatchHandle::onTimeout, this, _1, name));
}

void
WatchHandle::onTimeout(const ndn::Interest& interest, const Name& name)
{
  std::cerr << "Timeout" << std::endl;
  if (!m_processes[name].second) {
    return;
  }
  if (!onRunning(name))
    return;
  // selectors do not need to be updated
  Interest fetchInterest(interest.getName());
  fetchInterest.setSelectors(interest.getSelectors());
  fetchInterest.setInterestLifetime(m_interestLifetime);
  fetchInterest.setChildSelector(1);

  ++m_interestNum;
  getFace().expressInterest(fetchInterest,
                            bind(&WatchHandle::onData, this, _1, _2, name),
                            bind(&WatchHandle::onTimeout, this, _1, name), // Nack
                            bind(&WatchHandle::onTimeout, this, _1, name));

}

void
WatchHandle::listen(const Name& prefix)
{
  getFace().setInterestFilter(Name(prefix).append("watch").append("start"),
                              bind(&WatchHandle::onInterest, this, _1, _2));
  getFace().setInterestFilter(Name(prefix).append("watch").append("check"),
                              bind(&WatchHandle::onCheckInterest, this, _1, _2));
  getFace().setInterestFilter(Name(prefix).append("watch").append("stop"),
                              bind(&WatchHandle::onStopInterest, this, _1, _2));
}

void
WatchHandle::onStopInterest(const Name& prefix, const Interest& interest)
{
  m_validator.validate(interest,
                       bind(&WatchHandle::onStopValidated, this, _1, prefix),
                       bind(&WatchHandle::onStopValidationFailed, this, _1, _2));
}

void
WatchHandle::onStopValidated(const Interest& interest, const Name& prefix)
{
  RepoCommandParameter parameter;
  try {
    extractParameter(interest, prefix, parameter);
  }
  catch (RepoCommandParameter::Error) {
    negativeReply(interest, 403);
    return;
  }

  watchStop(parameter.getName());
  negativeReply(interest, 101);
}

void
WatchHandle::onStopValidationFailed(const Interest& interest, const ValidationError& error)
{
  std::cerr << error << std::endl;
  negativeReply(interest, 401);
}

void
WatchHandle::onCheckInterest(const Name& prefix, const Interest& interest)
{
  m_validator.validate(interest,
                       bind(&WatchHandle::onCheckValidated, this, _1, prefix),
                       bind(&WatchHandle::onCheckValidationFailed, this, _1, _2));
}

void
WatchHandle::onCheckValidated(const Interest& interest, const Name& prefix)
{
  RepoCommandParameter parameter;
  try {
    extractParameter(interest, prefix, parameter);
  }
  catch (RepoCommandParameter::Error) {
    negativeReply(interest, 403);
    return;
  }

  if (!parameter.hasName()) {
    negativeReply(interest, 403);
    return;
  }
  //check whether this process exists
  Name name = parameter.getName();
  if (m_processes.count(name) == 0) {
    std::cerr << "no such process name: " << name << std::endl;
    negativeReply(interest, 404);
    return;
  }

  RepoCommandResponse& response = m_processes[name].first;
    if (!m_processes[name].second) {
    response.setStatusCode(101);
  }

  reply(interest, response);

}

void
WatchHandle::onCheckValidationFailed(const Interest& interest, const ValidationError& error)
{
  std::cerr << error << std::endl;
  negativeReply(interest, 401);
}

void
WatchHandle::deferredDeleteProcess(const Name& name)
{
  getScheduler().scheduleEvent(PROCESS_DELETE_TIME,
                               bind(&WatchHandle::deleteProcess, this, name));
}

void
WatchHandle::processWatchCommand(const Interest& interest,
                                 RepoCommandParameter& parameter)
{
  // if there is no watchTimeout specified, m_watchTimeout will be set as 0 and this handle will run forever
  if (parameter.hasWatchTimeout()) {
    m_watchTimeout = parameter.getWatchTimeout();
  }
  else {
    m_watchTimeout = milliseconds(0);
  }

  // if there is no maxInterestNum specified, m_maxInterestNum will be 0, which means infinity
  if (parameter.hasMaxInterestNum()) {
    m_maxInterestNum = parameter.getMaxInterestNum();
  }
  else {
    m_maxInterestNum = 0;
  }

  if (parameter.hasInterestLifetime()) {
    m_interestLifetime = parameter.getInterestLifetime();
  }

  reply(interest, RepoCommandResponse().setStatusCode(100));

  m_processes[parameter.getName()] =
                std::make_pair(RepoCommandResponse().setStatusCode(300), true);
  Interest fetchInterest(parameter.getName());
  if (parameter.hasSelectors()) {
    fetchInterest.setSelectors(parameter.getSelectors());
  }
  fetchInterest.setChildSelector(1);
  fetchInterest.setInterestLifetime(m_interestLifetime);
  m_startTime = steady_clock::now();
  m_interestNum++;
  getFace().expressInterest(fetchInterest,
                            bind(&WatchHandle::onData, this, _1, _2, parameter.getName()),
                            bind(&WatchHandle::onTimeout, this, _1, parameter.getName()), // Nack
                            bind(&WatchHandle::onTimeout, this, _1, parameter.getName()));
}


void
WatchHandle::negativeReply(const Interest& interest, int statusCode)
{
  RepoCommandResponse response;
  response.setStatusCode(statusCode);
  reply(interest, response);
}

bool
WatchHandle::onRunning(const Name& name)
{
  bool isTimeout = (m_watchTimeout != milliseconds::zero() &&
                    steady_clock::now() - m_startTime > m_watchTimeout);
  bool isMaxInterest = m_interestNum >= m_maxInterestNum && m_maxInterestNum != 0;
  if (isTimeout || isMaxInterest) {
    deferredDeleteProcess(name);
    watchStop(name);
    return false;
  }
  return true;
}

} // namespace repo
#endif
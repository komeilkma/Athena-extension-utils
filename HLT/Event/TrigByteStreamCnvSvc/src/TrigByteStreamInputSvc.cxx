/*
 Copyright (C) 2022 Komeilkma/leandrogardi for the benefit of the ATLAS collaboration
*/

// Trigger includes
#include "TrigByteStreamInputSvc.h"
#include "TrigKernel/HltExceptions.h"

// Athena includes
#include "AthenaKernel/EventContextClid.h"
#include "StoreGate/StoreGateSvc.h"

// TDAQ includes
#include "hltinterface/DataCollector.h"

// System includes
#include <charconv>

namespace {
  constexpr float wordsToKiloBytes = 0.001*sizeof(uint32_t);
}

// =============================================================================
// Standard constructor
// =============================================================================
TrigByteStreamInputSvc::TrigByteStreamInputSvc(const std::string& name, ISvcLocator* svcLoc)
: ByteStreamInputSvc(name, svcLoc) {}

// =============================================================================
// Standard destructor
// =============================================================================
TrigByteStreamInputSvc::~TrigByteStreamInputSvc() {}

// =============================================================================
// Implementation of IInterface::queryInterface
// =============================================================================
StatusCode TrigByteStreamInputSvc::queryInterface(const InterfaceID& riid, void** ppvInterface) {
  ATH_MSG_VERBOSE("start of " << __FUNCTION__);

  if(ByteStreamInputSvc::interfaceID().versionMatch(riid))
    *ppvInterface = static_cast<ByteStreamInputSvc*>(this);
  else
    return AthService::queryInterface(riid, ppvInterface);

  addRef();
  ATH_MSG_VERBOSE("end of " << __FUNCTION__);
  return StatusCode::SUCCESS;
}

// =============================================================================
// Implementation of Service::initialize
// =============================================================================
StatusCode TrigByteStreamInputSvc::initialize() {
  ATH_MSG_VERBOSE("start of " << __FUNCTION__);

  ATH_CHECK(m_robDataProviderSvc.retrieve());
  ATH_CHECK(m_evtStore.retrieve());
  if (!m_monTool.empty()) ATH_CHECK(m_monTool.retrieve());

  ATH_MSG_VERBOSE("end of " << __FUNCTION__);
  return StatusCode::SUCCESS;
}

// =============================================================================
// Implementation of Service::finalize
// =============================================================================
StatusCode TrigByteStreamInputSvc::finalize() {
  ATH_MSG_VERBOSE("start of " << __FUNCTION__);
  if (m_robDataProviderSvc.release().isFailure()) {
    ATH_MSG_WARNING("Cannot release rob data provider");
  }
  ATH_MSG_VERBOSE("end of " << __FUNCTION__);
  return StatusCode::SUCCESS;
}

// =============================================================================
// Implementation of ByteStreamInputSvc::nextEvent
// =============================================================================
const RawEvent* TrigByteStreamInputSvc::nextEvent() {
  ATH_MSG_VERBOSE("start of " << __FUNCTION__);

  // Get the EventContext via event store because the interface doesn't allow passing it explicitly as an argument
  // and we don't want to use ThreadLocalContext. Don't use ReadHandle here because it calls ThreadLocalContext if
  // not given a context (which we want to retrieve).
  const EventContext* eventContext = nullptr;
  if (m_evtStore->retrieve(eventContext).isFailure()) {
    ATH_MSG_ERROR("Failed to retrieve EventContext from the event store, new event cannot be read");
    return nullptr;
  }

  ATH_MSG_DEBUG("Reading new event for event context " << *eventContext);

  // Find the cache corresponding to the current slot
  EventCache* cache = m_eventsCache.get(*eventContext);

  // Free the memory allocated to the previous event processed in the current slot
  cache->releaseEvent();

  using DCStatus = hltinterface::DataCollector::Status;
  DCStatus status = DCStatus::NO_EVENT;
  auto monLBN = Monitored::Scalar<uint16_t>("getNext_LBN", m_maxLB);
  auto monNoEvent = Monitored::Scalar<bool>("getNext_noEvent", false);
  try {
    auto t_getNext = Monitored::Timer<std::chrono::duration<float, std::milli>>("TIME_getNext");
    status = hltinterface::DataCollector::instance()->getNext(cache->rawData);
    auto mon = Monitored::Group(m_monTool, t_getNext);
  }
  catch (const std::exception& ex) {
    ATH_MSG_ERROR("Failed to read new event, caught an unexpected exception: " << ex.what()
                  << ". Throwing hltonl::Exception::EventSourceCorrupted" );
    throw hltonl::Exception::EventSourceCorrupted();
  }
  catch (...) {
    ATH_MSG_ERROR("Failed to read new event, caught an unexpected exception. "
                  << "Throwing hltonl::Exception::EventSourceCorrupted" );
    throw hltonl::Exception::EventSourceCorrupted();
  }

  if (status == DCStatus::STOP) {
    ATH_MSG_DEBUG("DataCollector::getNext returned STOP - no more events available");
    throw hltonl::Exception::NoMoreEvents();
  }
  else if (status == DCStatus::NO_EVENT) {
    ATH_MSG_DEBUG("DataCollector::getNext returned NO_EVENT - no events available temporarily");
    monNoEvent = true;
    auto mon = Monitored::Group(m_monTool, monLBN, monNoEvent);
    throw hltonl::Exception::NoEventsTemporarily();
  }
  else if (status != DCStatus::OK) {
    ATH_MSG_ERROR("Unhandled return Status " << static_cast<int>(status) << " from DataCollector::getNext");
    return nullptr;
  }
  ATH_MSG_VERBOSE("DataCollector::getNext returned Status::OK");

  // Create a cached FullEventFragment object from the cached raw data
  cache->fullEventFragment.reset(new RawEvent(cache->rawData.get()));

  // Update LB number for monitoring
  if (m_maxLB < cache->fullEventFragment->lumi_block()) {
    m_maxLB = cache->fullEventFragment->lumi_block();
    monLBN = m_maxLB;
  }

  // Monitor the input
  auto numROBs = Monitored::Scalar<int>("L1Result_NumROBs",
                                        cache->fullEventFragment->nchildren());
  auto fragSize = Monitored::Scalar<float>("L1Result_FullEvFragSize",
                                           cache->fullEventFragment->fragment_size_word()*wordsToKiloBytes);
  std::vector<eformat::read::ROBFragment> robVec;
  cache->fullEventFragment->robs(robVec);
  std::vector<std::string> subdetNameVec;
  for (const eformat::read::ROBFragment& rob : robVec) {
    eformat::helper::SourceIdentifier sid(rob.rob_source_id());
    subdetNameVec.push_back(sid.human_detector());
  }
  auto subdets = Monitored::Collection<std::vector<std::string>>("L1Result_SubDets", subdetNameVec);
  auto mon = Monitored::Group(m_monTool, numROBs, fragSize, subdets, monLBN, monNoEvent);

  // Give the FullEventFragment pointer to ROBDataProviderSvc
  m_robDataProviderSvc->setNextEvent(*eventContext, cache->fullEventFragment.get());
  ATH_MSG_VERBOSE("end of " << __FUNCTION__);

  // Check the CTP fragment (request from readout if not part of the cache), ATR-25217
  if (m_checkCTPFragmentModuleID.value() >= 0) {
    const eformat::helper::SourceIdentifier sid{eformat::SubDetector::TDAQ_CTP,
                                                static_cast<uint16_t>(m_checkCTPFragmentModuleID.value())};
    std::vector<const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment*> vrobf;
    m_robDataProviderSvc->getROBData(*eventContext, {sid.code()}, vrobf, name());
    if (vrobf.empty()) {
      ATH_MSG_INFO("The CTP ROB fragment 0x" << std::hex << sid.code() << std::dec << " is missing. "
                    << "Throwing hltonl::Exception::MissingCTPFragment");
      throw hltonl::Exception::MissingCTPFragment();
    }
    uint32_t robStatus = vrobf.at(0)->nstatus()>0 ? *(vrobf.at(0)->status()) : 0;
    if (robStatus!=0) {
      std::string hexStatus(8, char{0});
      std::to_chars(hexStatus.data(), hexStatus.data()+hexStatus.size(), robStatus, 16);
      ATH_MSG_INFO("The CTP ROB fragment 0x" << std::hex << sid.code() << std::dec << " has non-zero status word: 0x"
                    << hexStatus << ". Throwing hltonl::Exception::BadCTPFragment");
      throw hltonl::Exception::BadCTPFragment("Non-zero ROB status 0x"+hexStatus);
    }
    try {
      vrobf.at(0)->check();
    }
    catch (const std::exception& ex) {
      ATH_MSG_INFO("The CTP ROB fragment 0x" << std::hex << sid.code() << std::dec << " is corrupted: "
                    << ex.what() << ". Throwing hltonl::Exception::BadCTPFragment");
      throw hltonl::Exception::BadCTPFragment(ex.what());
    }
  }

  // Return the FullEventFragment pointer (do not transfer ownership)
  return cache->fullEventFragment.get();
}

// =============================================================================
// Implementation of ByteStreamInputSvc::previousEvent
// =============================================================================
const RawEvent* TrigByteStreamInputSvc::previousEvent() {
  ATH_MSG_FATAL("The method " << __FUNCTION__ << " is not implemented for online running");
  return nullptr;
}

// =============================================================================
// Implementation of ByteStreamInputSvc::currentEvent
// =============================================================================
const RawEvent* TrigByteStreamInputSvc::currentEvent() const {
  ATH_MSG_FATAL("The method " << __FUNCTION__ << " is not implemented for online running");
  return nullptr;
}

// =============================================================================
void TrigByteStreamInputSvc::EventCache::releaseEvent() {
  this->rawData.reset();
  this->fullEventFragment.reset();
}

/*
 Copyright (C) 2022 Komeilkma/leandrogardi for the benefit of the ATLAS collaboration
*/

// Trigger includes
#include "TrigByteStreamCnvSvc.h"
#include "TrigSteeringEvent/OnlineErrorCode.h"

// Athena includes
#include "AthenaKernel/EventContextClid.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "StoreGate/StoreGateSvc.h"

// Gaudi includes
#include "GaudiKernel/ITHistSvc.h"

// TDAQ includes
#include "eformat/FullEventFragmentNoTemplates.h"
#include "eformat/ROBFragmentNoTemplates.h"
#include "eformat/StreamTag.h"
#include "hltinterface/DataCollector.h"

// System includes
#include <sstream>
#include <iomanip>
#include <chrono>

// Local helper functions
namespace {
  constexpr float wordsToKiloBytes = 0.001*sizeof(uint32_t);
  template<typename T> inline bool contains(const std::vector<T>& vec, const T& val) {
    return std::find(vec.cbegin(), vec.cend(), val)!=vec.cend();
  }
  template<typename T> inline int index(const std::vector<T>& vec, const T& val) {
    typename std::vector<T>::const_iterator it =  std::find(vec.cbegin(), vec.cend(), val);
    return it==vec.cend() ? -1 : std::distance(vec.cbegin(), it);
  }
  template<typename T> struct printWordHex {
    printWordHex(const T w) : word(w) {}
    T word;
  };
  template<typename T> std::ostream& operator<<(std::ostream& str, const printWordHex<T>& pw) {
    str << "0x" << std::hex << std::setfill('0') << std::setw(2*sizeof(T));
    // Prevent printing char as ASCII character
    if (sizeof(T)==1)
      str << static_cast<int>(pw.word);
    else
      str << pw.word;
    str << std::dec;
    return str;
  }
  template<typename T> struct printNWordsHex {
    printNWordsHex(const size_t n, const T* w, const std::string s=" ") : nwords(n), words(w), sep(s) {}
    size_t nwords;
    const T* words;
    std::string sep;
  };
  template<typename T> std::ostream& operator<<(std::ostream& str, const printNWordsHex<T>& pnw) {
    for (size_t i=0; i<pnw.nwords; ++i) {
      str << printWordHex<T>(pnw.words[i]);
      if (i!=pnw.nwords-1) str << pnw.sep;
    }
    return str;
  }
  // StreamTag monitoring accessors
  inline const std::string mon_streamTypeName(const eformat::helper::StreamTag& st){
    return st.type+"_"+st.name;
  }
  inline const std::string& mon_streamType(const eformat::helper::StreamTag& st){
    return st.type;
  }
  inline bool mon_streamIsPeb(const eformat::helper::StreamTag& st){
    return st.robs.size()>0 || st.dets.size()>0;
  }
  inline size_t mon_streamPebRobsNum(const eformat::helper::StreamTag& st){
    return st.robs.size();
  }
  inline size_t mon_streamPebSubDetsNum(const eformat::helper::StreamTag& st){
    return st.dets.size();
  }
}

// =============================================================================
// Standard constructor
// =============================================================================
TrigByteStreamCnvSvc::TrigByteStreamCnvSvc(const std::string& name, ISvcLocator* svcLoc)
: ByteStreamCnvSvcBase(name, svcLoc) {}

// =============================================================================
// Standard destructor
// =============================================================================
TrigByteStreamCnvSvc::~TrigByteStreamCnvSvc() {}

// =============================================================================
// Implementation of Service::initialize
// =============================================================================
StatusCode TrigByteStreamCnvSvc::initialize() {
  ATH_MSG_VERBOSE("start of " << __FUNCTION__);
  ATH_CHECK(ByteStreamCnvSvcBase::initialize());
  ATH_CHECK(m_evtStore.retrieve());
  ATH_CHECK(m_robDataProviderSvc.retrieve());
  if (!m_monTool.empty()) ATH_CHECK(m_monTool.retrieve());
  ATH_MSG_VERBOSE("end of " << __FUNCTION__);
  return StatusCode::SUCCESS;
}

// =============================================================================
// Implementation of Service::finalize
// =============================================================================
StatusCode TrigByteStreamCnvSvc::finalize() {
  ATH_MSG_VERBOSE("start of " << __FUNCTION__);
  if (m_robDataProviderSvc.release().isFailure())
    ATH_MSG_WARNING("Failed to release service " << m_robDataProviderSvc.typeAndName());
  if (m_evtStore.release().isFailure())
    ATH_MSG_WARNING("Failed to release service " << m_evtStore.typeAndName());
  ATH_MSG_VERBOSE("end of " << __FUNCTION__);
  ATH_CHECK(ByteStreamCnvSvcBase::finalize());
  return StatusCode::SUCCESS;
}

// =============================================================================
// Implementation of IConversionSvc::connectOutput
// The argument outputFile is not used
// =============================================================================
StatusCode TrigByteStreamCnvSvc::connectOutput(const std::string& outputFile) {
  const EventContext* eventContext = currentContext();
  if (eventContext == nullptr) return StatusCode::FAILURE;
  return connectOutput(outputFile, *eventContext);
}

StatusCode TrigByteStreamCnvSvc::connectOutput(const std::string& /*outputFile*/, const EventContext& eventContext) {
  ATH_MSG_VERBOSE("start of " << __FUNCTION__);

  ATH_MSG_DEBUG("Creating new RawEventWrite for EventContext = " << eventContext);
  // Create a new RawEventWrite and copy the header from the input RawEvent
  RawEventWrite* re = setRawEvent(std::make_unique<RawEventWrite>(), eventContext);
  const uint32_t* inputRawEvent = m_robDataProviderSvc->getEvent(eventContext)->start();
  if (!inputRawEvent) {
    ATH_MSG_ERROR("Input RawEvent is nullptr, cannot create output");
    return StatusCode::FAILURE;
  }
  re->copy_header(inputRawEvent);

  ATH_MSG_VERBOSE("Created RawEventWrite pointer = " << re);

  ATH_MSG_VERBOSE("end of " << __FUNCTION__);
  return StatusCode::SUCCESS;
}

// =============================================================================
// Implementation of IConversionSvc::connectOutput
// The arguments are not used, this overload is implemented only for interface compatibility
// =============================================================================
StatusCode TrigByteStreamCnvSvc::connectOutput(const std::string& outputFile, const std::string& /*openMode*/) {
  return connectOutput(outputFile);
}

// =============================================================================
// Implementation of IConversionSvc::commitOutput
// The arguments outputFile and do_commit are not used
// NOTE: In online HLT, m_rawEventWrite is not a full event, it contains only the HLTResult ROBFragments
// =============================================================================
StatusCode TrigByteStreamCnvSvc::commitOutput(const std::string& outputFile, bool do_commit) {
  const EventContext* eventContext = currentContext();
  if (eventContext == nullptr) return StatusCode::FAILURE;
  return commitOutput(outputFile, do_commit, *eventContext);
}

StatusCode TrigByteStreamCnvSvc::commitOutput(const std::string& /*outputFile*/, bool /*do_commit*/, const EventContext& eventContext) {
  ATH_MSG_VERBOSE("start of " << __FUNCTION__);

  if (msgLvl(MSG::DEBUG)) printRawEvent(eventContext);

  RawEventWrite* re = getRawEvent(eventContext);

  // Serialise the output FullEventFragment
  std::unique_ptr<uint32_t[]> rawEventPtr;
  try {
    const eformat::write::node_t* top = re->bind();
    uint32_t rawEventSize = re->size_word();
    rawEventPtr = std::make_unique<uint32_t[]>(rawEventSize);
    uint32_t copiedSize = eformat::write::copy(*top,rawEventPtr.get(),rawEventSize);
    if(copiedSize!=rawEventSize) {
      ATH_MSG_ERROR("FullEventFragment serialisation failed");
      return StatusCode::FAILURE;
    }
  }
  catch (const std::exception& e) {
    ATH_MSG_ERROR("FullEventFragment serialisation failed, caught an unexpected std::exception " << e.what());
    clearRawEvent(eventContext);
    return StatusCode::FAILURE;
  }
  catch (...) {
    ATH_MSG_ERROR("FullEventFragment serialisation failed, caught an unexpected exception");
    clearRawEvent(eventContext);
    return StatusCode::FAILURE;
  }

  {
    auto t_mon = Monitored::Timer("TIME_monitorRawEvent");
    monitorRawEvent(rawEventPtr);
    Monitored::Group(m_monTool, t_mon);
  }

  // Send output to the DataCollector
  StatusCode result = StatusCode::SUCCESS;
  try {
    auto t_eventDone = Monitored::Timer<std::chrono::duration<float, std::milli>>("TIME_eventDone");
    hltinterface::DataCollector::instance()->eventDone(std::move(rawEventPtr));
    Monitored::Group(m_monTool, t_eventDone);
    ATH_MSG_DEBUG("Serialised FullEventFragment with HLT result was returned to DataCollector successfully, "
                  << "the eventDone call took " << (double)t_eventDone << " milliseconds");
  }
  catch (const std::exception& e) {
    ATH_MSG_ERROR("Sending output to DataCollector failed, caught an unexpected std::exception " << e.what());
    result = StatusCode::FAILURE;
  }
  catch (...) {
    ATH_MSG_ERROR("Sending output to DataCollector failed, caught an unexpected exception");
    result = StatusCode::FAILURE;
  }

  clearRawEvent(eventContext);

  ATH_MSG_VERBOSE("end of " << __FUNCTION__);
  return result;
}

// =============================================================================
const EventContext* TrigByteStreamCnvSvc::currentContext() const {
  // Get the EventContext via event store because the base class doesn't allow passing it explicitly as an argument
  // and we don't want to use ThreadLocalContext. Don't use ReadHandle here because it calls ThreadLocalContext if
  // not given a context (which we want to retrieve). This relies on IHiveWhiteBoard::selectStore being called on the
  // current thread before we arrive here (it is done in HltEventLoopMgr).
  const EventContext* eventContext = nullptr;
  if (m_evtStore->retrieve(eventContext).isFailure()) {
    ATH_MSG_ERROR("Failed to retrieve EventContext from the event store");
  }
  return eventContext;
}

// =============================================================================
RawEventWrite* TrigByteStreamCnvSvc::getRawEvent() {
  const EventContext* eventContext = currentContext();
  if (eventContext == nullptr) return nullptr;
  return getRawEvent(*eventContext);
}

// =============================================================================
RawEventWrite* TrigByteStreamCnvSvc::getRawEvent(const EventContext& eventContext) const {
  return m_rawEventWriteCache.get(eventContext)->get();
}

// =============================================================================
RawEventWrite* TrigByteStreamCnvSvc::setRawEvent(std::unique_ptr<RawEventWrite>&& rawEventWrite, const EventContext& eventContext) {
  *(m_rawEventWriteCache.get(eventContext)) = std::move(rawEventWrite);
  return getRawEvent(eventContext);
}

// =============================================================================
void TrigByteStreamCnvSvc::clearRawEvent(const EventContext& eventContext) {
  m_rawEventWriteCache.get(eventContext)->reset();
}

// =============================================================================
void TrigByteStreamCnvSvc::monitorRawEvent(const std::unique_ptr<uint32_t[]>& rawEventPtr) const {
  // Create a read fragment from the pointer
  eformat::read::FullEventFragment rawEvent(rawEventPtr.get());

  // Monitor error code
  if (rawEvent.nstatus() > 1) {
    HLT::OnlineErrorCode errorCode = static_cast<HLT::OnlineErrorCode>(rawEvent.status()[1]);
    std::ostringstream ss;
    ss << errorCode;
    auto monOnlineErrorCode = Monitored::Scalar<std::string>("OnlineErrorCode", ss.str());
    Monitored::Group(m_monTool, monOnlineErrorCode);
  }

  // Decode stream tags
  std::vector<eformat::helper::StreamTag> streamTags;
  try {
    eformat::helper::decode(rawEvent.nstream_tag(), rawEvent.stream_tag(), streamTags);
  }
  catch (const std::exception& ex) {
    ATH_MSG_ERROR("StreamTag decoding failed, caught an unexpected std::exception " << ex.what());
    return;
  }
  catch (...) {
    ATH_MSG_ERROR("StreamTag decoding failed, caught an unexpected exception");
    return;
  }

  // Get HLT result sizes
  std::vector<eformat::read::ROBFragment> robs;
  rawEvent.robs(robs);
  std::vector<uint16_t> resultSizeMap_moduleID;
  std::vector<uint32_t> resultSizeMap_size;
  uint32_t totalSizeWords = 0;
  try {
    for (const eformat::read::ROBFragment& rob : robs) {
      eformat::helper::SourceIdentifier sid(rob.rob_source_id());
      if (sid.subdetector_id() != eformat::SubDetector::TDAQ_HLT)
        continue;
      const uint16_t module_id = sid.module_id();
      const uint32_t size = rob.fragment_size_word();
      totalSizeWords += size;
      if (!contains(resultSizeMap_moduleID, module_id)) {
        resultSizeMap_moduleID.push_back(module_id);
        resultSizeMap_size.push_back(size);
      }
      else {
        ATH_MSG_ERROR("HLT result ROB monitoring found multiple HLT ROBs with the same module ID " << module_id);
      }
    }
  }
  catch (const std::exception& ex) {
    ATH_MSG_ERROR("HLT result ROB monitoring failed, caught an unexpected std::exception " << ex.what());
    return;
  }
  catch (...) {
    ATH_MSG_ERROR("HLT result ROB monitoring failed, caught an unexpected exception");
    return;
  }

  // Fill helper containers for monitoring
  std::vector<std::string> sdFromRobList;
  std::vector<std::string> sdFromSubDetList;
  std::vector<std::string> streamTagCorrA;
  std::vector<std::string> streamTagCorrB;
  std::vector<float> streamResultSize; // Correlated with streamTags vector
  streamTagCorrA.reserve(streamTags.size() * streamTags.size());
  streamTagCorrB.reserve(streamTags.size() * streamTags.size());
  streamResultSize.reserve(streamTags.size());
  for (const eformat::helper::StreamTag& st : streamTags) {
    bool hasHLTSubDet = st.dets.find(eformat::SubDetector::TDAQ_HLT) != st.dets.end();
    bool includeAll = st.robs.empty() && (st.dets.empty() || hasHLTSubDet);
    // includeAll means a stream with full event building or all HLT results included
    uint32_t sizeWords = includeAll ? totalSizeWords : 0;
    for (const eformat::SubDetector sd : st.dets) {
      const std::string& detName = eformat::helper::SubDetectorDictionary.string(sd);
      if (!contains(sdFromSubDetList, detName)) sdFromSubDetList.push_back(detName);
    }
    for (const uint32_t robid : st.robs) {
      eformat::helper::SourceIdentifier sid(robid);
      const std::string& detName = sid.human_detector();
      if (!contains(sdFromRobList, detName)) sdFromRobList.push_back(detName);
      if (!includeAll && sid.subdetector_id() == eformat::SubDetector::TDAQ_HLT) {
        if (const int ix = index(resultSizeMap_moduleID, sid.module_id()); ix >= 0) {
          sizeWords += resultSizeMap_size[ix];
        }
        else {
          ATH_MSG_WARNING("Stream tag " << st.type << "_" << st.name << " declares " << sid.human()
                          << " in ROB list, but the ROBFragment is missing");
        }
      }
    }
    streamResultSize.push_back(sizeWords*wordsToKiloBytes);
    for (const eformat::helper::StreamTag& st2 : streamTags) {
      streamTagCorrA.push_back(mon_streamTypeName(st));
      streamTagCorrB.push_back(mon_streamTypeName(st2));
    }
  }

  // General stream tag monitoring
  auto monStreamTagsNum         = Monitored::Scalar<size_t>("StreamTagsNum", streamTags.size());
  auto monStreamTags            = Monitored::Collection("StreamTags", streamTags, mon_streamTypeName);
  auto monStreamTagsType        = Monitored::Collection("StreamTagsType", streamTags, mon_streamType);
  auto monStreamTagCorrA        = Monitored::Collection("StreamTagCorrA", streamTagCorrA);
  auto monStreamTagCorrB        = Monitored::Collection("StreamTagCorrB", streamTagCorrB);
  // PEB stream tag monitoring
  auto monStreamIsPeb           = Monitored::Collection("StreamTagIsPeb", streamTags, mon_streamIsPeb);
  auto monPebRobsNum            = Monitored::Collection("StreamTagsPebRobsNum", streamTags, mon_streamPebRobsNum);
  auto monPebSubDetsNum         = Monitored::Collection("StreamTagsPebSubDetsNum", streamTags, mon_streamPebSubDetsNum);
  auto monSubDetsFromRobList    = Monitored::Collection("StreamTagsPebSubDetsFromRobList", sdFromRobList);
  auto monSubDetsFromSubDetList = Monitored::Collection("StreamTagsPebSubDetsFromSubDetList", sdFromSubDetList);
  // Result size monitoring
  auto monResultSizeTotal       = Monitored::Scalar<float>("ResultSizeTotal", totalSizeWords*wordsToKiloBytes);
  auto monResultSizeFullEvFrag  = Monitored::Scalar<float>("ResultSizeFullEvFrag", rawEvent.fragment_size_word()*wordsToKiloBytes);
  auto monResultCollModuleID    = Monitored::Collection("ResultModuleID", resultSizeMap_moduleID);
  auto monResultCollModuleSize  = Monitored::Collection("ResultModuleSize", resultSizeMap_size, [](uint32_t sw){return sw*wordsToKiloBytes;});
  auto monResultSizeByStream    = Monitored::Collection("ResultSizeStream", streamResultSize);
  // Collect all variables
  Monitored::Group(m_monTool, monStreamTagsNum, monStreamTags, monStreamTagsType, monStreamTagCorrA,
                   monStreamTagCorrB, monStreamIsPeb, monPebRobsNum, monPebSubDetsNum, monSubDetsFromRobList,
                   monSubDetsFromSubDetList, monResultSizeTotal, monResultSizeFullEvFrag, monResultCollModuleID,
                   monResultCollModuleSize, monResultSizeByStream);
}

// =============================================================================
void TrigByteStreamCnvSvc::printRawEvent(const EventContext& eventContext) const {
  RawEventWrite* re = getRawEvent(eventContext);

  if (!re) {
    ATH_MSG_WARNING("RawEventWrite pointer is null");
    return;
  }
  std::ostringstream ss;
  ss << "Dumping header of the FullEventFragment with HLT result:" << std::endl;
  ss << "--> status              = "
     << printNWordsHex<uint32_t>(re->nstatus(), re->status())
     << std::endl;
  ss << "--> source_id           = " << printWordHex<uint32_t>(re->source_id()) << std::endl;
  ss << "--> checksum_type       = " << printWordHex<uint32_t>(re->checksum_type()) << std::endl;
  ss << "--> compression_type    = " << printWordHex<uint32_t>(re->compression_type()) << std::endl;
  ss << "--> compression_level   = " << re->compression_level() << std::endl;
  ss << "--> bc_time_seconds     = " << re->bc_time_seconds() << std::endl;
  ss << "--> bc_time_nanoseconds = " << re->bc_time_nanoseconds() << std::endl;
  ss << "--> global_id           = " << re->global_id() << std::endl;
  ss << "--> run_type            = " << re->run_type() << std::endl;
  ss << "--> run_no              = " << re->run_no() << std::endl;
  ss << "--> lumi_block          = " << re->lumi_block() << std::endl;
  ss << "--> lvl1_id             = " << re->lvl1_id() << std::endl;
  ss << "--> bc_id               = " << re->bc_id() << std::endl;
  ss << "--> lvl1_trigger_type   = " << printWordHex<uint8_t>(re->lvl1_trigger_type()) << std::endl;
  ss << "--> lvl1_trigger_info   = "
     << printNWordsHex<uint32_t>(re->nlvl1_trigger_info(), re->lvl1_trigger_info())
     << std::endl;
  ss << "--> lvl2_trigger_info   = "
     << printNWordsHex<uint32_t>(re->nlvl2_trigger_info(), re->lvl2_trigger_info())
     << std::endl;
  ss << "--> event_filter_info   = "
     << printNWordsHex<uint32_t>(re->nevent_filter_info(), re->event_filter_info())
     << std::endl;
  ss << "--> hlt_info            = "
     << printNWordsHex<uint32_t>(re->nhlt_info(), re->hlt_info())
     << std::endl;

  std::vector<eformat::helper::StreamTag> stream_tags;
  try {
    eformat::helper::decode(re->nstream_tag(), re->stream_tag(), stream_tags);
  }
  catch (const std::exception& ex) {
    ATH_MSG_ERROR("StreamTag decoding failed, caught an unexpected std::exception " << ex.what());
    return;
  }
  catch (...) {
    ATH_MSG_ERROR("StreamTag decoding failed, caught an unexpected exception");
    return;
  }
  ss << "--> stream_tags         = ";
  bool first = true;
  for (const auto& st : stream_tags) {
    if (first) first=false;
    else ss << "                          ";
    ss << "{" << st.name << ", " << st.type << ", obeysLB=" << st.obeys_lumiblock << ", robs=[";
    for (const auto& robid : st.robs) ss << printWordHex<uint32_t>(robid) << ", ";
    ss << "], dets = [";
    for (const auto& detid : st.dets) ss << printWordHex<uint8_t>(detid) << ", ";
    ss << "]}" << std::endl;
  }

  ATH_MSG_DEBUG(ss.str());
}

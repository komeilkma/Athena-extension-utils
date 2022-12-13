/*
 Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
*/

#ifndef BYTESTREAMCNVSVC_BYTESTREAMEVENTSTORAGEINPUTSVC_H
#define BYTESTREAMCNVSVC_BYTESTREAMEVENTSTORAGEINPUTSVC_H

/**
 @class ByteStreamEmonInputSvc
 @brief implements the interface ByteStreamInputSvc for reading events
        from emon.

*/

// Include files.
#define BOOST_BIND_GLOBAL_PLACEHOLDERS // Needed to silence Boost pragma message
#include "emon/EventIterator.h"

#include "TrigConfData/L1Menu.h"

#include "ByteStreamCnvSvc/ByteStreamInputSvc.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "ByteStreamData/RawEvent.h"
#include "GaudiKernel/ServiceHandle.h"
#include "GaudiKernel/ITHistSvc.h"

#include <vector>
#include <stdint.h>
#include <memory>

#include <boost/regex.hpp>

class OHRootProvider;
class StoreGateSvc;

class ByteStreamEmonInputSvc: public ByteStreamInputSvc {
public:

   /// Constructors:
   ByteStreamEmonInputSvc(const std::string& name, ISvcLocator* svcloc);

   virtual StatusCode initialize() override;
   virtual StatusCode start() override;
   virtual StatusCode finalize() override;

   /// Implementation of the ByteStreamInputSvc interface methods.
   virtual const RawEvent* previousEvent() override;
   virtual const RawEvent* nextEvent() override;

   /// Implementation of the ByteStreamInputSvc interface methods.
   virtual const RawEvent* currentEvent() const override;

private:

   bool getIterator();
   void check_publish();
   void get_runparams();

   void updateHandler(Gaudi::Details::PropertyBase& p);

   int                m_totalEventCounter{0}; //!< event Counter

   std::unique_ptr<RawEvent>          m_re{nullptr};          //!< current event

   // Properties
   Gaudi::Property<std::string>               m_partition{this, "Partition", {}, "Partition name, default taken from $TDAQ_PARTITION if not set"};
   Gaudi::Property<std::string>               m_key{this, "Key", {}, &ByteStreamEmonInputSvc::updateHandler, "EMON Selection key, e.g. SFI"};
   Gaudi::Property<std::vector<std::string>>  m_value{this, "KeyValue", {}, &ByteStreamEmonInputSvc::updateHandler, "EMON key values, e.g. [SFI-1, SFI-2]; if empty all SFIs"};
   Gaudi::Property<unsigned int>              m_key_count{this, "KeyCount", 0, &ByteStreamEmonInputSvc::updateHandler, "EMON key count, e.g. 5 to get five random SFIs"};
   Gaudi::Property<std::string>               m_publish{this, "PublishName", "Athena", &ByteStreamEmonInputSvc::updateHandler, "Name under which to publish histograms"};
   Gaudi::Property<bool>                      m_exit {this, "ExitOnPartitionShutdown", false, &ByteStreamEmonInputSvc::updateHandler, "If true, exit if partition shuts down"};
   Gaudi::Property<std::string>               m_is_server{this, "ISServer", "", &ByteStreamEmonInputSvc::updateHandler, "IS Server to publish histograms to"};
   Gaudi::Property<std::string>               m_include{this, "Include", {}, &ByteStreamEmonInputSvc::updateHandler, "Regular expression to select histograms to publish"};
   Gaudi::Property<std::string>               m_exclude{this, "Exclude", {}, &ByteStreamEmonInputSvc::updateHandler, "Regular expression to select histograms not to publish"};
   Gaudi::Property<int>                       m_frequency{this, "Frequency", 60, &ByteStreamEmonInputSvc::updateHandler, "Frequency (in number of events) of publishing histograms"};
   Gaudi::Property<int>                       m_updatePeriod{this, "UpdatePeriod", 0, &ByteStreamEmonInputSvc::updateHandler, "Frequency (in seconds) of publishing histograms"};
   Gaudi::Property<float>                     m_updatePeriodRange{this, "UpdatePeriodRange", 0.2, "A percentage number to determine how close we have to be to the next update time to publish now"};
   Gaudi::Property<bool>                      m_clearHistograms{this, "ClearHistograms", true, &ByteStreamEmonInputSvc::updateHandler, "If true, clear histograms when new run starts (default: True)"};
   Gaudi::Property<std::vector<std::string>>  m_l1names{this, "LVL1Names", {}, &ByteStreamEmonInputSvc::updateHandler, "A list of L1 bit names"};
   Gaudi::Property<std::vector<unsigned int>> m_l1items{this, "LVL1Items", {}, &ByteStreamEmonInputSvc::updateHandler, "A list of L1 bit numbers"};
   Gaudi::Property<std::string>               m_l1logic{this, "LVL1Logic", "Ignore", &ByteStreamEmonInputSvc::updateHandler, "'And', 'Or' or 'Ignore' (default: Ignore)"};
   Gaudi::Property<std::string>               m_l1origin{this, "LVL1Origin", "TAV", &ByteStreamEmonInputSvc::updateHandler, "TBP, TAP, TAV (default: TAV"};
   Gaudi::Property<std::string>               m_stream_type{this, "StreamType", "physics", &ByteStreamEmonInputSvc::updateHandler, "HLT stream type (e.g. physics or calibration)"};
   Gaudi::Property<std::vector<std::string>>  m_stream_names{this, "StreamNames", {}, &ByteStreamEmonInputSvc::updateHandler, "List of HLT stream names"};
   Gaudi::Property<std::string>               m_stream_logic{this, "StreamLogic", "Ignore", &ByteStreamEmonInputSvc::updateHandler, "'And', 'Or' or 'Ignore' (default: Ignore)"};

   Gaudi::Property<unsigned int>              m_trigger_type{this, "TriggerType", 256, &ByteStreamEmonInputSvc::updateHandler, "LVL1 8 bit trigger type"};
   Gaudi::Property<std::string>               m_groupName{this, "GroupName", {}, &ByteStreamEmonInputSvc::updateHandler, "Name of the monitoring group"};
   Gaudi::Property<unsigned int>              m_buffer_size{this, "BufferSize", 2, "Number of buffers"};
   Gaudi::Property<bool>                      m_readDetectorMask{this, "ReadDetectorMaskFromIS", true, "Read detector mask from IS"};
   Gaudi::Property<int>                       m_timeout{this, "Timeout", 3600000, &ByteStreamEmonInputSvc::updateHandler, "Timeout in seconds, -1 == infinity"};
   Gaudi::Property<bool>                      m_corrupted_events{this, "ProcessCorruptedEvents", false, &ByteStreamEmonInputSvc::updateHandler, "Process corrupted events not passing check_tree()"};
   Gaudi::Property<std::string>              m_state{this, "State", "None", "Read-only property showing the state"};
   Gaudi::Property<bool>                      m_convertEfficiency{this, "ConvertEfficiency", true, "Convert TEfficiency to TProfile before publishing"};

   // Data handles
   SG::ReadHandleKey<TrigConf::L1Menu> m_l1MenuKey{this, "L1TriggerMenu", "DetectorStore+L1TriggerMenu", "Name of the L1Menu object to read configuration from"};

   // internal
   bool                      m_connect{false};
   std::auto_ptr<emon::EventIterator> m_eventIt;   //!< Event iterator

   ITHistSvc*                m_histSvc{nullptr};
   OHRootProvider*           m_provider{nullptr};

   boost::regex              m_include_rex;
   boost::regex              m_exclude_rex;

   int                       m_frequency_counter{60};
   int                       m_publish_target{0};

   ServiceHandle<StoreGateSvc>         m_inputMetaDataStore;
   ServiceHandle<StoreGateSvc>         m_sgSvc;
   ServiceHandle<IROBDataProviderSvc>  m_robProvider;

};

#endif

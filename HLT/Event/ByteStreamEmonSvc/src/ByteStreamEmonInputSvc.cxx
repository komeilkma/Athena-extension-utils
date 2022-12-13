/*
 Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
*/

//===================================================================
//  Implementation of ByteStreamEmonInputSvc
//===================================================================

// Include files.
#include "ByteStreamEmonInputSvc.h"

#include "eformat/Version.h"
#include "eformat/HeaderMarker.h"
#include "eformat/DetectorMask.h"
#include "oh/OHRootProvider.h"
#include "rc/RunParamsNamed.h"
#include "TProfile.h"
#include "TEfficiency.h"
#include "TProfile2D.h"

#include "GaudiKernel/ITHistSvc.h"

#include "StoreGate/StoreGateSvc.h"
#include "xAODEventInfo/EventInfo.h"
#include "xAODEventInfo/EventAuxInfo.h"
#include "ByteStreamData/ByteStreamMetadata.h"
#include "ByteStreamData/ByteStreamMetadataContainer.h"
#include "ByteStreamCnvSvcBase/ByteStreamAddress.h"
#include "CxxUtils/checker_macros.h"
#include "PersistentDataModel/DataHeader.h"
#include "./extract_histogram_tag.h"

#include <cstdlib>
#include <csignal>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/preprocessor/repetition.hpp>

#include <memory>

namespace {
    
    void handle_terminate ATLAS_NOT_THREAD_SAFE (int)
    {
        // online wants 
        ::exit(0);
    }
}

namespace {
    void copyAttributes(TH1* prof, const TEfficiency* eff) {
        // we take the profile as a TH1 since TProfile2D is not descended from TProfile
        // code adapted from TEfficiency::FillHistogram
        const auto* tot{eff->GetTotalHistogram()};
 
        // set the axis labels
        TString xlabel{tot->GetXaxis()->GetTitle()};
        TString ylabel{tot->GetYaxis()->GetTitle()};
        TString zlabel{tot->GetZaxis()->GetTitle()};
        if (xlabel) prof->GetXaxis()->SetTitle(xlabel);
        if (ylabel) prof->GetYaxis()->SetTitle(ylabel);
        if (zlabel) prof->GetZaxis()->SetTitle(zlabel);
 
        Int_t nbinsx = prof->GetNbinsX();
        Int_t nbinsy = prof->GetNbinsY();
 
        // copy axis labels if existing. Assume are there in the total histogram
        if (tot->GetXaxis()->GetLabels() != nullptr) {
            for (int ibinx = 1; ibinx <= nbinsx; ++ibinx)
                prof->GetXaxis()->SetBinLabel(ibinx, tot->GetXaxis()->GetBinLabel(ibinx));
        }
        if (tot->GetYaxis()->GetLabels() != nullptr) {
            for (int ibiny = 1; ibiny <= nbinsy; ++ibiny)
                prof->GetYaxis()->SetBinLabel(ibiny, tot->GetYaxis()->GetBinLabel(ibiny));
        }
 
        //copying style information
        eff->TAttLine::Copy(*prof);
        eff->TAttFill::Copy(*prof);
        eff->TAttMarker::Copy(*prof);
        prof->SetStats(0);
    }

    std::unique_ptr<const TProfile> create1DProfile(const TEfficiency* eff) {
        const auto* tot{eff->GetTotalHistogram()};
        const auto* pass{eff->GetPassedHistogram()};
        std::unique_ptr<TProfile> prof;
        const auto* xaxis{tot->GetXaxis()};
        if (xaxis->IsVariableBinSize()) {
            prof = std::make_unique<TProfile>("eff_histo",eff->GetTitle(),
                                xaxis->GetNbins(), xaxis->GetXbins()->GetArray());
        } else {
            prof = std::make_unique<TProfile>("eff_histo",eff->GetTitle(),
                                xaxis->GetNbins(), xaxis->GetXmin(), xaxis->GetXmax());
        }
        prof->SetDirectory(0);

        for (Int_t ibin = 0; ibin < tot->GetNbinsX()+2; ++ibin) {
            auto center{tot->GetBinCenter(ibin)};
            prof->Fill(center, 1, pass->GetBinContent(ibin));
            prof->Fill(center, 0, tot->GetBinContent(ibin)-pass->GetBinContent(ibin));
            prof->SetBinEntries(ibin, tot->GetBinContent(ibin));
        }
        copyAttributes(prof.get(), eff);

        return prof;
    }

    std::unique_ptr<const TProfile2D> create2DProfile(const TEfficiency* eff) {
        const auto* tot{eff->GetTotalHistogram()};
        const auto* pass{eff->GetPassedHistogram()};
        std::unique_ptr<TProfile2D> prof;
        Int_t nbinsx{tot->GetNbinsX()};
        Int_t nbinsy{tot->GetNbinsY()};
        const auto* xaxis{tot->GetXaxis()};
        const auto* yaxis{tot->GetYaxis()};

        if (xaxis->IsVariableBinSize() && yaxis->IsVariableBinSize() ) {
            prof = std::make_unique<TProfile2D>("eff_histo", 
                        eff->GetTitle(), nbinsx, xaxis->GetXbins()->GetArray(),
                        nbinsy, yaxis->GetXbins()->GetArray());
        } else if (xaxis->IsVariableBinSize() && ! yaxis->IsVariableBinSize() ) {
            prof = std::make_unique<TProfile2D>("eff_histo",
                        eff->GetTitle(), nbinsx, xaxis->GetXbins()->GetArray(),
                        nbinsy, yaxis->GetXmin(), yaxis->GetXmax());
        } else if (!xaxis->IsVariableBinSize() &&  yaxis->IsVariableBinSize() ) {
            prof = std::make_unique<TProfile2D>("eff_histo",
                        eff->GetTitle(), nbinsx, xaxis->GetXmin(), xaxis->GetXmax(),
                        nbinsy, yaxis->GetXbins()->GetArray());
        } else {
            prof = std::make_unique<TProfile2D>("eff_histo",
                        eff->GetTitle(), nbinsx, xaxis->GetXmin(), xaxis->GetXmax(),
                        nbinsy, yaxis->GetXmin(), yaxis->GetXmax());
        }
        prof->SetDirectory(0);
 
        for (Int_t ibinx = 0; ibinx < tot->GetNbinsX()+2; ++ibinx) {
            auto centerx{tot->GetXaxis()->GetBinCenter(ibinx)};
            for (Int_t ibiny = 0; ibiny < tot->GetNbinsY()+2; ++ibiny) {
                auto centery{tot->GetYaxis()->GetBinCenter(ibiny)};
                prof->Fill(centerx, centery, 1, pass->GetBinContent(ibinx, ibiny));
                prof->Fill(centerx, centery, 0, 
                        tot->GetBinContent(ibinx, ibiny)-pass->GetBinContent(ibinx, ibiny));
                prof->SetBinEntries(prof->GetBin(ibinx, ibiny), 
                                    tot->GetBinContent(ibinx, ibiny));

            }
        }
        copyAttributes(prof.get(), eff);

        return prof;
    }
}

// Constructor.
ByteStreamEmonInputSvc::ByteStreamEmonInputSvc(const std::string& name, ISvcLocator* svcloc) :
    ByteStreamInputSvc(name,svcloc),
    m_inputMetaDataStore("StoreGateSvc/InputMetaDataStore", name ),
    m_sgSvc("StoreGateSvc", name),
    m_robProvider("ROBDataProviderSvc", name)
{
}

// Open the first input file and read the first event.
StatusCode ByteStreamEmonInputSvc::initialize()
{
    setProperty("State", "Init").ignore();

    ATH_CHECK( ByteStreamInputSvc::initialize() );

    // check properties
    if(m_partition.empty() && getenv("TDAQ_PARTITION") != 0) {
        m_partition = getenv("TDAQ_PARTITION");
    }

    if(m_partition.empty()){
        ATH_MSG_ERROR("initialize: No partition name specified");
        return StatusCode::FAILURE;
    }
	 
    if(m_key.empty()) {
        ATH_MSG_ERROR("initialize: No emon key ");
        return StatusCode::FAILURE;
    } 

    m_connect = true;

    if (service("THistSvc", m_histSvc, true).isFailure()) {
        ATH_MSG_ERROR("Unable to locate THistSvc");
        m_is_server.clear();
    } else {
        if(!m_include.empty()) m_include_rex = m_include.value();
        if(!m_exclude.empty()) m_exclude_rex = m_exclude.value();
        if(m_frequency < 0) m_frequency = 100000;
        m_frequency_counter = m_frequency;

        // if time based update, disable frequency based update
        if(m_updatePeriod) {
            m_frequency = 0;
            // calculate our next target for publishing
            time_t now = time(0);
            m_publish_target = now - (now % m_updatePeriod) + m_updatePeriod;
        }
    }

    //-------------------------------------------------------------------------
    // Setup the InputMetaDataStore
    //-------------------------------------------------------------------------
    ATH_CHECK( m_inputMetaDataStore.retrieve() );
    ATH_CHECK( m_robProvider.retrieve() );

    // Initialise the L1Menu read handle if we need to map L1 item names to IDs
    ATH_CHECK(m_l1MenuKey.initialize(not m_l1names.value().empty()));

    signal(SIGTERM, handle_terminate);

    // Read run parameters from the partition
    if (m_readDetectorMask) {
        get_runparams();
    }

    ATH_MSG_INFO("initialized for: " << m_partition << " " << m_key << "/" << m_value);

    return StatusCode::SUCCESS;
}

bool ByteStreamEmonInputSvc::getIterator()
{
    setProperty("State", "Init").ignore();

    if(!IPCCore::isInitialised()) {
        char* argv[2] = { 0 };
        int argc = 0;
        IPCCore::init(argc, argv);
    }

    IPCPartition partition(m_partition);
    while(!partition.isValid()) {
        if(m_exit) {
            ATH_MSG_ERROR("No such partition: " << m_partition);
            return false;
        }
        ATH_MSG_INFO("No such partition (yet?): " << m_partition);
        sleep(20);
    }

    delete m_provider;
    m_provider = nullptr;

    std::unique_ptr<emon::SamplingAddress> address;

    if(m_key_count > 0) {
        address.reset(new emon::SamplingAddress(m_key, m_key_count));
    }  else {
        address.reset(new emon::SamplingAddress(m_key, m_value));
    }

    emon::Logic  l1_logic = emon::logic::IGNORE;
    emon::Logic  stream_logic = emon::logic::IGNORE;
    emon::Origin l1_origin = emon::origin::AFTER_VETO;

    if(m_l1logic == "Or") {
        l1_logic = emon::logic::OR;
    } else if (m_l1logic == "And") {
        l1_logic = emon::logic::AND;
    } else if (m_l1logic == "Ignore") {
        l1_logic = emon::logic::IGNORE;
    }

    if(m_l1origin == "TBP") {
        l1_origin = emon::origin::BEFORE_PRESCALE;
    } else if (m_l1origin == "TAP") {
        l1_origin = emon::origin::AFTER_PRESCALE;
    } else if(m_l1origin == "TAV") {
        l1_origin = emon::origin::AFTER_VETO;
    } else {
        ATH_MSG_FATAL("Invalid L1 origin");
    }

    if(m_stream_logic == "Or") {
        stream_logic = emon::logic::OR;
    } else if (m_stream_logic == "And") {
        stream_logic = emon::logic::AND;
    } else if (m_stream_logic == "Ignore") {
        stream_logic = emon::logic::IGNORE;
    }

    // now put together the bit mask from all
    // three sources: l1 bit mask, l1 names, l1 items

    std::vector<unsigned short> l1bits(m_l1items.begin(), m_l1items.end());

    // if names are given, read the mapping information from L1Menu
    if (not m_l1names.value().empty()) {
        ATH_MSG_DEBUG("Reading L1Menu to map " << m_l1names.name() << " to CTP IDs");
        SG::ReadHandle<TrigConf::L1Menu> l1Menu = SG::makeHandle(m_l1MenuKey);
        if (not l1Menu.isValid()) {
            ATH_MSG_ERROR("Cannot read L1Menu to map L1 item names to IDs. The property " << m_l1names.name() << " will be ignored!");
        } else {
            for (const std::string& l1name : m_l1names) {
                try {
                    const unsigned int id = l1Menu->item(l1name).ctpId();
                    ATH_MSG_DEBUG("Item " << l1name << " mapped to CTP ID " << id);
                    l1bits.push_back(static_cast<unsigned short>(id));
                } catch (const std::exception& ex) {
                    ATH_MSG_ERROR(ex.what());
                    continue;
                }
            }
        }
    }

    typedef emon::MaskedValue<unsigned char>	L1TriggerType;
    
    emon::SmartBitValue l1pattern(l1bits, l1_logic, l1_origin);
    emon::SmartStreamValue streamTags(m_stream_type, m_stream_names, stream_logic);

    emon::L1TriggerType l1triggerType(static_cast<unsigned char>(m_trigger_type), m_trigger_type > 255);

    emon::SelectionCriteria criteria(l1triggerType,
                                     l1pattern,
                                     streamTags,
                                     emon::StatusWord());

    while (true) {

        if (partition.name() != m_partition) {
            ATH_MSG_INFO("Partition name changed - reconnecting to " << m_partition);
            return getIterator();
        }

	try {
            m_eventIt.reset(0);
            m_eventIt.reset(new emon::EventIterator(partition, *address, criteria, m_buffer_size, m_groupName));
            if(m_readDetectorMask) {
                get_runparams();
            }
            setProperty("State", "Connected").ignore();
	    return true;
	} catch(ers::Issue& ex) {
	    ATH_MSG_INFO("Cannot connect to sampler (will wait 20s): " << m_key << "/" << m_value
                     << " Reason: " << ex.what());
	    sleep(20);
	}
    }
}

// Read previous event should not be called for this version of input svc
const RawEvent* ByteStreamEmonInputSvc::previousEvent()
{
    ATH_MSG_WARNING("previousEvent not implemented for ByteStreamEmonInputSvc");

    return nullptr;
}

// Read the next event.
const RawEvent* ByteStreamEmonInputSvc::nextEvent()
{
    if (m_re) {
        OFFLINE_FRAGMENTS_NAMESPACE::PointerType st = nullptr;
        m_re->start(st);
        if (st) delete [] st;
        m_re.reset();
    }

    while(m_re == nullptr) {

        if(m_connect) {
	   
            if(!getIterator()) {
                return nullptr;
            }  else {
                ATH_MSG_INFO("Got sampler...");
                m_connect = false;
            }
        }
       
        emon::Event event;
        try {
            int timeout = m_timeout;
            if(m_updatePeriod) {
                // try to hit lower end of update period
                timeout = (m_publish_target - time(0) - int(m_updatePeriodRange * m_updatePeriod)) * 1000;
                if(timeout <= 0) timeout = 1000;
            }
            event = m_eventIt->nextEvent(timeout);
        } catch(emon::NoMoreEvents& ex) {
            if(m_exit) {
                return nullptr;
            }
            check_publish();
            continue;
        } catch (ers::Issue& ex) {
            m_connect = true;
            continue;
        }

        setProperty("State", "Processing").ignore();
       
        OFFLINE_FRAGMENTS_NAMESPACE::DataType* buf = new OFFLINE_FRAGMENTS_NAMESPACE::DataType[event.size()];
        memcpy(buf, event.data(), event.size() * sizeof(OFFLINE_FRAGMENTS_NAMESPACE::DataType));


        if (buf[0] == eformat::FULL_EVENT) {

            // We got a full event
            m_re = std::make_unique<RawEvent>(buf);
            try {
                m_re->check_tree();
                ATH_MSG_INFO("nextEvent: Got valid fragment of size:" << event.size());
            } catch (ers::Issue& ex) {
	      
                // log in any case
		std::stringstream ss;
		ss << ex;
                ATH_MSG_ERROR("nextEvent: Invalid event fragment: " << ss.str());
               
                if(!m_corrupted_events) {

                    delete [] buf;
                    m_re.reset();
                    continue;
                } // else fall through
            }
	    m_robProvider->setNextEvent(m_re.get());
	    m_robProvider->setEventStatus(0);	    

        } else {
            // We got something we didn't expect.
            ATH_MSG_ERROR("nextEvent: Got invalid fragment of unknown type: 0x"
                          << std::hex << buf[0] << std::dec);
            delete [] buf;
            continue;
        }
        ++m_totalEventCounter;
    }

    // generate DataHeader
    DataHeader* Dh = new DataHeader();
    
   // Declare header primary
    Dh->setStatus(DataHeader::Input);

    // Now add ref to xAOD::EventInfo objects
    IOpaqueAddress* iop = new ByteStreamAddress(ClassID_traits<xAOD::EventInfo>::ID(), "EventInfo", "");
    StatusCode ioc = m_sgSvc->recordAddress("EventInfo",iop);
    if (ioc.isSuccess()) {
        const SG::DataProxy* ptmp = m_sgSvc->transientProxy(ClassID_traits<xAOD::EventInfo>::ID(), "EventInfo");
        if (ptmp !=0) {
            DataHeaderElement DheEI(ptmp, nullptr, "EventInfo");
            Dh->insert(DheEI);
        }
        //else ATH_MSG_ERROR("Failed to create xAOD::EventInfo proxy " << ptmp);
    }

	// Now add ref to xAOD::EventAuxInfo objects
    IOpaqueAddress* iopaux = new ByteStreamAddress(ClassID_traits<xAOD::EventAuxInfo>::ID(), "EventInfoAux.", "");
    StatusCode iocaux = m_sgSvc->recordAddress("EventInfoAux.",iopaux);
    if (iocaux.isSuccess()) {
        const SG::DataProxy* ptmpaux = m_sgSvc->transientProxy(ClassID_traits<xAOD::EventAuxInfo>::ID(), "EventInfoAux.");
        if (ptmpaux !=0) {
            DataHeaderElement DheEIAux(ptmpaux, nullptr, "EventInfoAux.");
            Dh->insert(DheEIAux);
        }
        //else ATH_MSG_ERROR("Failed to create xAOD::EventAuxInfo proxy " << ptmpaux);
    }

    // Record new data header.Boolean flags will allow it's deletion in case
    // of skipped events.
    StatusCode rec_sg = m_sgSvc->record<DataHeader>(Dh, "ByteStreamDataHeader", true, false, true);
    if (rec_sg != StatusCode::SUCCESS) {
        ATH_MSG_ERROR("Fail to record BS DataHeader in StoreGate. Skipping events?! " << rec_sg);
    }

    // we got an event, check if we have to publish
    check_publish();

    return m_re.get();

}
   
// Get a pointer to the current event. 
const RawEvent* ByteStreamEmonInputSvc::currentEvent() const 
{
    return m_re.get();
}

void ByteStreamEmonInputSvc::check_publish()
{
    // is a server specified ?
    if(m_is_server.empty()) return;

    // a time based update ?
    if(m_updatePeriod) {
        time_t now = time(0);

        if(now < m_publish_target - int(m_updatePeriodRange * m_updatePeriod)) {
            // it's too early to publish
            return;
        }

        if(now > m_publish_target + int(m_updatePeriodRange * m_updatePeriod)) {
            // it's too late
            unsigned int missed_publications = 0;
            while(m_publish_target < now) {
                missed_publications++;
                m_publish_target += m_updatePeriod;
            }

            // do NOT publish, but print a warning
            ATH_MSG_WARNING(" check_publish: missed " << missed_publications << " publications to OH");

            return;
        }
    }

    // an event based update ?
    if(m_frequency && --m_frequency_counter > 0) return;

    m_frequency_counter = m_frequency;

    try { 

        if(m_provider == nullptr) {
            IPCPartition part(m_partition);
            if(!part.isValid()) return;
            // might throw...
            m_provider = new OHRootProvider(part, m_is_server, m_publish);
        }

        for(const std::string& name : m_histSvc->getHists()) {

            if(!m_include.empty() && !regex_match(name, m_include_rex)) {
                continue;
            }

            if(!m_exclude.empty() && regex_match(name, m_exclude_rex)) {
                continue;
            }

            TH1 *h = nullptr;
            if(m_histSvc->getHist(name, h)) {
                // might throw...
                auto name_tag = detail::extract_histogram_tag(name);
                m_provider->publish(*h, name_tag.first, name_tag.second);
            };
        }
        for(const std::string& name : m_histSvc->getEfficiencies()) {

            if(!m_include.empty() && !regex_match(name, m_include_rex)) {
                continue;
            }

            if(!m_exclude.empty() && regex_match(name, m_exclude_rex)) {
                continue;
            }

            TEfficiency *h = nullptr;
            if(m_histSvc->getEfficiency(name, h)) {
                if (m_convertEfficiency) {
                    std::unique_ptr<const TH1> p;
                    if (h->GetDimension() == 1) {
                        p = create1DProfile(h);
                    } else if (h->GetDimension() == 2) {
                        p = create2DProfile(h);
                    }
                    // might throw...
                    auto name_tag = detail::extract_histogram_tag(name);
                    m_provider->publish(*p, name_tag.first, name_tag.second);
                    //m_provider->publish(*h, name_tag.first, name_tag.second);
                } // tdaq doesn't currently support publishing efficiencies, will change in the future
            };
        }
    } catch (daq::oh::Exception& ex) {
        ATH_MSG_ERROR(ex.what());
    } 

    m_publish_target += m_updatePeriod;
}

void ByteStreamEmonInputSvc::get_runparams()
{
    IPCPartition p(m_partition);
    
    RunParamsNamed runParams(p, "RunParams.RunParams");
    try {
        runParams.checkout();

        eformat::helper::DetectorMask mask(runParams.det_mask);

        auto metadatacont = std::make_unique<ByteStreamMetadataContainer>();
        metadatacont->push_back(std::make_unique<ByteStreamMetadata>(
                 runParams.run_number,
                 0,
                 0,
                 runParams.recording_enabled,
                 runParams.trigger_type,
                 mask.serialize().second,
                 runParams.beam_type,
                 runParams.beam_energy,
                 "",
                 "",
                 runParams.T0_project_tag,
                 0,
                 std::vector<std::string>()
                 ));
        // Record ByteStreamMetadataContainer in MetaData Store
        if(m_inputMetaDataStore->record(std::move(metadatacont),"ByteStreamMetadata").isFailure()) {
           ATH_MSG_WARNING("Unable to record MetaData in InputMetaDataStore");
        }
        else {
           ATH_MSG_DEBUG("Recorded MetaData in InputMetaDataStore");
        }

    } catch(ers::Issue& ex) {
        ATH_MSG_ERROR("Cannot get run parameters:" << ex.what());
    }
}

// start of run
StatusCode ByteStreamEmonInputSvc::start()
{
    if(m_clearHistograms) {
        ATH_MSG_INFO("Resetting histograms...");
        for(const std::string& name : m_histSvc->getHists()) {
            TH1 *h = nullptr;
            if(m_histSvc->getHist(name, h)) {
                h->Reset();
            };
        }
        for(const std::string& name : m_histSvc->getEfficiencies()) {
            TEfficiency *h = nullptr;
            if(m_histSvc->getEfficiency(name, h)) {
                std::unique_ptr<TH1> pass{h->GetCopyPassedHisto()};
                pass->Reset();
                h->SetPassedHistogram(*pass.get(), "");
                std::unique_ptr<TH1> tot{h->GetCopyTotalHisto()};
                tot->Reset();
                h->SetTotalHistogram(*tot.get(), "");
                h->SetWeight(1);
            };
        }
    }
    return StatusCode::SUCCESS;
}

void ByteStreamEmonInputSvc::updateHandler(Gaudi::Details::PropertyBase& /* p */)
{
    if(! m_connect) {
        m_connect = true;
        sleep(2);
        setProperty("State","Reconnect").ignore();
    }
}


StatusCode ByteStreamEmonInputSvc::finalize() 
{
    setProperty("State","Shutdown").ignore();
    m_inputMetaDataStore.release().ignore();
    m_robProvider.release().ignore();
    m_sgSvc.release().ignore();
    return StatusCode::SUCCESS;
}

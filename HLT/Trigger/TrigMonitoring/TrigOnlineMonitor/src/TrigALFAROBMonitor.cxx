/*
 Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
*/

#include "TrigALFAROBMonitor.h"

#include "AthenaKernel/Timeout.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "TrigT1MuctpiBits/MuCTPI_Bits.h"

#include "eformat/eformat.h"

#include <sstream>
#include <cassert>


////////////////////////////////////////////////////////////////////////////

TrigALFAROBMonitor::TrigALFAROBMonitor(const std::string& name, ISvcLocator* pSvcLocator) :
  AthReentrantAlgorithm(name, pSvcLocator), 
  m_robDataProviderSvc( "ROBDataProviderSvc", name ),

  //m_hist_failedChecksumForALFAROB(0),
  //m_histProp_failedChecksumForALFAROB(Gaudi::Histo1DDef("FailedChecksumForALFAROB" ,0,1,1)),
  m_lvl1muCTPIResult(0)
{
  // Declare the properties
  declareProperty("Lvl1CTPROBid",                       m_lvl1CTPROBid=0x770001);
  declareProperty("Lvl1ALFA2ROBid",                     m_lvl1ALFA2ROBid=0x840000);
  declareProperty("Lvl1ALFA1ROBid",                     m_lvl1ALFA1ROBid=0x840001);
  declareProperty("DaqCTPROBid",                        m_daqCTPROBid=0x770000);
  declareProperty("TestROBChecksum",                    m_doROBChecksum=true);
  //declareProperty("HistFailedChecksumForALFAROB",       m_histProp_failedChecksumForALFAROB,"ALFA ROBs with inconsistent checksum");
  declareProperty("MonitorALFATracks",                  m_doALFATracking=true);
  declareProperty("MonitorPMFactivity",                 m_doPMFMonitoring=true);
  declareProperty("DoGoodDataMonitoring",               m_doDataGoodMonitoring=true);
  declareProperty("DoODDistanceHistograming",           m_doODDistance=true);

 // fill vectors with names of trigger items
 m_map_TrgNamesToHistGroups["L1_ALFA_ELAST15"] = 0;
 m_map_TrgNamesToHistGroups["L1_ALFA_ELAST18"] = 0;

 m_map_TrgNamesToHistGroups["L1_ALFA_ANY"] = 1;

}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 

StatusCode TrigALFAROBMonitor::initialize(){

  // Get the messaging service
  
  // Print out the property values
  ATH_MSG_INFO( " ROB ID: Lvl1 CTP                           = " << m_lvl1CTPROBid 
      << std::setw(6) << " (=0x" << MSG::hex << m_lvl1CTPROBid.value() << MSG::dec << ")" );
  ATH_MSG_INFO( " ROB ID: Lvl1 ALFA1                        = " << m_lvl1ALFA1ROBid
      << std::setw(6) << " (=0x" << MSG::hex << m_lvl1ALFA1ROBid.value() << MSG::dec << ")" );
  ATH_MSG_INFO( " ROB ID: Lvl1 ALFA2                        = " << m_lvl1ALFA2ROBid
      << std::setw(6) << " (=0x" << MSG::hex << m_lvl1ALFA2ROBid.value() << MSG::dec << ")" );
  ATH_MSG_INFO( " ROB ID: DAQ CTP                            = " << m_daqCTPROBid
      << std::setw(6) << " (=0x" << MSG::hex << m_daqCTPROBid.value() << MSG::dec << ")" );
  ATH_MSG_INFO( " Do ROB checksum test                       = " << m_doROBChecksum );
  //ATH_MSG_INFO( "        Hist:FailedChecksumForALFAROB       = " << m_histProp_failedChecksumForALFAROB );

  // Locate the ROBDataProviderSvc
  StatusCode sc = m_robDataProviderSvc.retrieve();
  if (!sc.isSuccess()) {
    ATH_MSG_ERROR( "Could not find ROBDataProviderSvc" );
    return sc;
  } 
  
  m_ALFARobIds.push_back(m_lvl1ALFA1ROBid.value());
  m_ALFARobIds.push_back(m_lvl1ALFA2ROBid.value());

  ATH_CHECK( m_L1MenuKey.initialize() );
  ATH_CHECK( m_RBResultKey.initialize() );

  ATH_CHECK( m_monTools.retrieve() );

  char *val = getenv("TDAQ_PARTITION");
  std::string part = val == NULL ? std::string("") : std::string(val);
  m_inTDAQPart = part.find("TDAQ") != std::string::npos ? true : false;
  if (m_inTDAQPart) 
     ATH_MSG_INFO("Running in TDAQ partition - will skip the ROB CRC check");

  ATH_MSG_INFO("Initialize completed");
  return StatusCode::SUCCESS;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 

StatusCode TrigALFAROBMonitor::execute (const EventContext& ctx) const {

  uint32_t  LB; // luminosity block number

  std::vector <float> loc_pU[8][10];
  std::vector <float> loc_pV[8][10];

  std::map<int,int> triggerHitPattern;
  std::map<int,int> triggerHitPatternReady;


  //--------------------------------------------------------------------------
  // check that there is still time left after all HLT chains and TopAlg completed
  //--------------------------------------------------------------------------
  if (Athena::Timeout::instance(ctx).reached()) {
    ATH_MSG_DEBUG(" Time out reached in entry to execute.");
    return StatusCode::SUCCESS;
  }

  
  const EventIDBase p_EventIDBase = ctx.eventID();
  LB = p_EventIDBase.lumi_block();
  ATH_MSG_DEBUG(" Decoded lumi block nb: " <<LB);

  // Now try to extract L1 decisons from ROIB fragment
  //if(!evtStore()->contains<ROIB::RoIBResult>(m_RBResultKey)) {
       //ATH_MSG_INFO("RoIBResult does not exist with key: " << m_RBResultKey);
  //}

  SG::ReadHandle<ROIB::RoIBResult> h_roIBResult (m_RBResultKey, ctx);
  if (!h_roIBResult.isValid()) {
       ATH_MSG_INFO("RoIBResult does not exist with key: " << m_RBResultKey);
      return StatusCode::SUCCESS;
  }

  const ROIB::RoIBResult* roIBResult = h_roIBResult.cptr();

  const std::vector<ROIB::CTPRoI> ctpRoIVecAV = roIBResult->cTPResult().TAV();
  for (unsigned int iWord = 0; iWord < ctpRoIVecAV.size(); ++iWord) {
          uint32_t roIWord = ctpRoIVecAV[iWord].roIWord();
          ATH_MSG_DEBUG(" roiAV "<<std::hex<<roIWord<<std::dec);
  }

  // get the ALFA ROBs
  //std::vector<const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment*> ALFARobFragmentVec;
  const std::vector<uint32_t> mc_ALFARobIds = {0x840000,0x840001};
  IROBDataProviderSvc::VROBFRAG ALFARobFragmentVec;
  ALFARobFragmentVec.reserve(m_ALFARobIds.size());
  m_robDataProviderSvc->getROBData(ctx, m_ALFARobIds,ALFARobFragmentVec, name());

  if (ALFARobFragmentVec.size()==0) {
    ATH_MSG_DEBUG(" No ALFA ROB found.");
    return StatusCode::SUCCESS;
  } 


  bool event_with_checksum_failure(false);

  // loop over retrieved ROBs and do checks
  for (std::vector<const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment*>::iterator it = ALFARobFragmentVec.begin();
       it != ALFARobFragmentVec.end();++it) {
    // verify checksum - but skip if running with TDAQ partition (with preloaded data in ROSes)
    if (!m_inTDAQPart) {
        if (verifyALFAROBChecksum(**it )) event_with_checksum_failure=true ;
    } 


    // decode ALFA ROBs
    bool FiberHitsODNeg[8][3][30], FiberHitsODPos[8][3][30];
    if (! event_with_checksum_failure) {
       if (decodeALFA(**it, loc_pU, loc_pV, FiberHitsODNeg, FiberHitsODPos, triggerHitPattern, triggerHitPatternReady ) == 0) {
          findALFATracks(roIBResult, LB, loc_pU, loc_pV);
          findODTracks (FiberHitsODNeg, FiberHitsODPos, triggerHitPattern, triggerHitPatternReady);
      }
    }
  }

  return StatusCode::SUCCESS;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 

StatusCode TrigALFAROBMonitor::start() {

  ATH_MSG_INFO("TrigALFAROBMonitor::start()");

  SG::ReadHandle<TrigConf::L1Menu>  l1MenuHandle = SG::makeHandle( m_L1MenuKey );
  if( l1MenuHandle.isValid() ) {
    for ( const TrigConf::L1Item& item: *l1MenuHandle ){
      ATH_MSG_DEBUG("new L1 item: "<<item.name() << "; ctpId: " << item.ctpId() <<"; definition: " <<item.definition());
    }
    for (const TrigConf::L1Item& item: *l1MenuHandle) {
      ATH_MSG_DEBUG(" triggerItem "<<item.name().c_str()<< "ctpId "<<item.ctpId());
      std::map<std::string, int>::iterator it = m_map_TrgNamesToHistGroups.find(item.name());
      if (it != m_map_TrgNamesToHistGroups.end()) {
        m_map_TrgItemNumbersToHistGroups[item.ctpId()] = it->second;
        // locate golden alfa triggers for data quality assesment base on the ratio of tracks in elastic triggered events
        if (item.name().compare("L1_ALFA_ELAST15") == 0) { m_elast15 = item.ctpId(); continue; }
        if (item.name().compare("L1_ALFA_ELAST18") == 0) { m_elast18 = item.ctpId(); continue; }
        if (item.name().compare("L1_ALFA_SYST17")  == 0) { m_syst17  = item.ctpId(); continue; }
        if (item.name().compare("L1_ALFA_SYST18")  == 0)   m_syst18  = item.ctpId();

      }
    }
  }

  ATH_MSG_DEBUG("TrigALFAROBMonitor::start() 2 ; m_map_TrgItemNumbersToHistGroups.size() = "<<m_map_TrgItemNumbersToHistGroups.size());


  for (std::map<int, int>::iterator it=m_map_TrgItemNumbersToHistGroups.begin(); it != m_map_TrgItemNumbersToHistGroups.end(); ++it) {
          ATH_MSG_DEBUG(" triggerItem number: "<<it->first<< " histo group: "<<it->second);
  }
 
  // *-- booking path
  m_pathHisto = std::string("/EXPERT/") + name() + "/";

  // Specific source identifiers
  //eformat::helper::SourceIdentifier srcID_ALFA( eformat::FORWARD_ALPHA ,0);
  //eformat::helper::SourceIdentifier srcID_CTP( eformat::TDAQ_CTP ,0);
  //eformat::helper::SourceIdentifier srcID_HLT( eformat::TDAQ_HLT, 0);


  return StatusCode::SUCCESS;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
bool TrigALFAROBMonitor::verifyALFAROBChecksum(const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment& robFrag) const {

  bool failed_checksum(false);
  OFFLINE_FRAGMENTS_NAMESPACE::PointerType it(0); 
  uint32_t current_value(0);

  // print check for received ROB
  if(msgLvl(MSG::VERBOSE)) {
    robFrag.payload(it);
    current_value = eformat::helper::checksum(robFrag.checksum_type(), it, robFrag.payload_size_word());

    ATH_MSG_DEBUG(
     " ALFA ROB id = 0x"             << std::setw(6)  << MSG::hex << robFrag.source_id() << MSG::dec 
	<< " checksum: type = "            << std::setw(2)  << robFrag.checksum_type()
	<< " value = "                     << std::setw(12) << robFrag.checksum_value()
	<< " value (recalculated) = "      << std::setw(12) << current_value
	<< " check = "                     << std::setw(2)  << robFrag.checksum());
  }

  // checksum test failed
  if ( not robFrag.checksum() ) {
    failed_checksum = true;

    // recalculate checksum value
    robFrag.payload(it);
    current_value = eformat::helper::checksum(robFrag.checksum_type(), it, robFrag.payload_size_word());

    // print warning
    ATH_MSG_WARNING(
	   " ALFA ROB checksum verification failed." 
	<< " ALFA ROB id = 0x"             << std::setw(6)  << MSG::hex << robFrag.source_id() << MSG::dec 
	<< " checksum type = "             << std::setw(2)  << robFrag.checksum_type()
	<< " value = "                     << std::setw(12) << robFrag.checksum_value()
	<< " value (recalculated) = "      << std::setw(12) << current_value
	<< " check = "                     << std::setw(2)  << robFrag.checksum());

    // fill the histograms
    std::ostringstream ost;
    ost << "0x" << std::hex << robFrag.source_id();
    //if (m_hist_failedChecksumForALFAROB) {
      //oh_scoped_lock_histogram lock;
      //m_hist_failedChecksumForALFAROB->Fill((ost.str()).c_str(), 1.);
      //m_hist_failedChecksumForALFAROB->LabelsDeflate("X");
    //}
  }

  return failed_checksum;

}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
uint32_t TrigALFAROBMonitor::decodeALFA(const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment& robFrag, 
                                        std::vector<float> (&loc_pU) [8][10], std::vector<float> (&loc_pV) [8][10],
                                        bool FiberHitsODNeg[][3][30], bool FiberHitsODPos[][3][30],
                                        std::map<int,int>& triggerHitPattern,std::map<int,int>& triggerHitPatternReady) const {

  ATH_MSG_DEBUG(" decodeALFA: ROB id = 0x" << std::setw(6)  << MSG::hex << robFrag.source_id() << MSG::dec);

  uint32_t formatVersion = robFrag.rod_version();
  uint32_t evtNum        = robFrag.rod_lvl1_id();
  uint32_t robFragSize   = robFrag.fragment_size_word();
  uint32_t rodFragSize   = robFrag.rod_fragment_size_word();
  uint32_t robId         = robFrag.source_id();
  uint32_t rodId         = robFrag.rod_source_id();
  //const uint32_t bcId    = robFrag.rod_bc_id();


  const uint32_t* status;
  robFrag.rod_status( status );
  //uint32_t nstatus = robFrag.rod_nstatus();

  //uint32_t errorStat( 0 );
  //if( nstatus > 0 ) errorStat = static_cast< uint32_t >( *status );

  ATH_MSG_DEBUG( "ALFA ROB ID 0x" << MSG::hex << robId <<  " ROD ID 0x"
				     << rodId << MSG::dec << " ROB fragment size "
				     << robFragSize << " ROD fragment size " << rodFragSize << " evtNum " <<evtNum );

  // check if we have real data in the ROB - ALFA has fixed fragment size of 0x1e1 - skip such event and return if not real
  if (robFragSize != 0x1e1) return (1);

  if ((rodId == (uint32_t)m_lvl1ALFA1ROBid.value()) || (rodId == (uint32_t)m_lvl1ALFA2ROBid.value())) {
    ATH_MSG_DEBUG( "   Found ALFA ROB " << std::to_string(rodId) );
    ATH_MSG_DEBUG( "   Dumping ALFA ROB header - data - trailer words:" );
    /* Create header */
    ROIB::Header ALFAHead( rodId, evtNum, formatVersion);
    /* Create content body */
    const uint32_t* data;
    robFrag.rod_data( data );

    const uint32_t* lwcPtr = data + 1;
    const uint32_t* twcPtr = lwcPtr + ((*lwcPtr) & 0xffff) - 1;

     
    uint32_t ndata = robFrag.rod_ndata();
    for( uint32_t i = 0; i < ndata; ++i, ++data ) {
      ATH_MSG_DEBUG( MSG::dec<< " i: "<<i<<"       0x" << MSG::hex << std::setw( 8 )
	    << static_cast< uint32_t >( *data ) );
    } 

  {
    for (int layer = 0; layer < 10; layer++) {
        for (int station = 0; station < 8; station++) {
            loc_pV[station][layer].clear();
            loc_pU[station][layer].clear();
        }
    }
   triggerHitPatternReady.clear();
   triggerHitPattern.clear();

    for (int station=0; station<8; station++)
        for (int layer=0; layer<3; layer++)
            for (int fiber=0; fiber<30; fiber++) {
                FiberHitsODPos[station][layer][fiber] = false;
                FiberHitsODNeg[station][layer][fiber] = false;
            }

    
    while ( 1 ) {
	uint32_t mbNb = 0; // from 20.07.2010 MB numbers are as PMF0 15-0 bits - counting from 1 and coded as 1 from N 
        
        // check consistency of the ROD data - if data from LWC point to TWC
        if ((*lwcPtr & 0xff000000) != 0x81000000) {
    	    ATH_MSG_DEBUG("ROD "<< MSG::hex<<rodId<<" skipped - LWC(-1): "<< *(lwcPtr-1) <<" LWC: "<<*lwcPtr << " LWC+1: "<< *(lwcPtr+1) );
            return (1); //continue;
        }
        if ((*twcPtr & 0xff000000) != 0x8a000000) {
    	    ATH_MSG_DEBUG( "ROD "<< MSG::hex<<rodId<<" skipped - TWC(-1): "<< *(twcPtr-1)<< " TWC: "<< *twcPtr <<" TWC+1: " << *(twcPtr+1) 
                           <<" LWC: " << *lwcPtr << " mbNb: "<< mbNb);
            return (1); //continue;
        }


        const uint32_t* dataPtr = lwcPtr;
        dataPtr++; // bol

	dataPtr++; // tlp - always 5 TDCs (5 captons) - use it as mask for scanning TDC data
        uint32_t tlp = (*dataPtr) & 0xffff; 
        dataPtr++; // bot

        for ( ; tlp; tlp>>=1 ) {

            dataPtr++; // TDC data
	    uint32_t data16channels = *dataPtr;
    	    //ATH_MSG_DEBUG( "current tlp: " << tlp << "; data16channels: "<< MSG::hex << std::setw( 8 )<< data16channels );
            //ERS_INFO ("Decode: mrodNb: "<<mrodNb<<" tlp: "<<tlp<<" bot:" << bot <<" data16channels: "<<data16channels);
            while ((data16channels & 0xf0000000) == 0x30000000) {
                int pmf = (data16channels >> 19 ) & 0x1F;
                int quarter = (data16channels >> 24) & 0x3;

                if ((pmf >= 1)&&(pmf <= 23)){
                  if (mbNb > 0) {
                    if (data16channels & 0x20000) {
                        // check the PMF error bit; set when PMF BCX & EventNumber counters differ from MB's
                        //AlfaEventObj->synchMBvsPMF[mbNb-1] |= (0x1)<<(pmf-1);
                    } else {
                        if (data16channels & 0xffff) {
                              decodeRealPMT (data16channels, quarter, mbNb-1, pmf, loc_pU, loc_pV, FiberHitsODNeg, FiberHitsODPos);
                        }
                    }
                  }
                }
                else {
                  // PMF0 - control data: quarter=0: motherboard nb coded as 1 from N, quarter=2: "dead", quarter=3: "face"
                  // PMF24 - trigger mezzanine data: q=0: trigger pattern, q=1: QDC0, q=2: QDC1, q=3: rate_counter
                  if ( pmf == 0) {
                        if (quarter == 0){
                             mbNb =  decodePMT0 (data16channels);
                         //    AlfaEventObj->stationTrackData[mbNb-1] = 1;
                        }
                  }
                  else {
                      if ( pmf == 24) {// trigger mezzanine data
                        if (data16channels & 0x20000) {
                            // check the PMF error bit; set when PMF BCX & EventNumber counters differ from MB's
                          //  AlfaEventObj->synchMBvsPMF[mbNb-1] |= (0x1)<<(pmf-1);
                        } else {
                            //decodePMT24 (data16channels, quarter, mbNb);
                            if (quarter == 0) {
                                  triggerHitPattern[mbNb-1] = data16channels & 0xFFFF;
                                  triggerHitPatternReady[mbNb-1] = 1;
                            } else {
                            } 
                        }
                      }
                  }
                }
                dataPtr++;
                data16channels = *dataPtr;
            }

            // TDC trailer EOT ?
            if (( *dataPtr & 0xF0000000) != 0xC0000000) {
                break;
            }
            dataPtr++; //should point to BOT of next TDC or to TWC for the last TDC analysed
	}
        // dataPtr should point now to TWC. if next word is LWC continue, if EOB end otherwise error
        dataPtr++;
        if ((*dataPtr & 0xff000000) == 0x81000000)   {  // LWC
            lwcPtr = dataPtr;
            twcPtr = lwcPtr + ((*lwcPtr) & 0xffff) - 1;
            continue;
        }
        
        return (0); //break;  // end of data or data corrupted - break decoding
    } //ERS_INFO ("gnamDecode - end of decoding MROD:" << MrodId);
  }
 }
 return (0);
}

void TrigALFAROBMonitor::decodeRealPMT (uint32_t dataWord, uint32_t quarter, uint32_t mbNb, uint32_t pmf, 
                                        std::vector<float> (&loc_pU) [8][10], std::vector<float> (&loc_pV) [8][10], 
                                        bool FiberHitsODNeg[][3][30], bool FiberHitsODPos[][3][30]) const {

  // save input stream flags

    int layerNb = m_pmf2layer[pmf];
    int RPNumber = m_mbNb2RP[mbNb];
        RPNumber = RPNumber - 1;  // to access data from array in C - which starts with index 0
	
    int mask = 0x1;
	//ATH_MSG_DEBUG( "decodeRealPMT - dataWord: " << dataWord << " quarter: " << quarter << " mbNb: " << mbNb << " pmf: " << pmf );
    for (int offset = 0 ; offset <= 15; offset++) {
       if (dataWord & mask) {
           int channel = offset + quarter*16;

           {
               std::string stationName =  m_stationNames[mbNb] + "_"; 
               auto channelNb = Monitored::Scalar<double>(stationName, channel);
               auto pmfNb     = Monitored::Scalar<double>("PMF", pmf);
               auto monGroup = Monitored::Group ( *m_monTools["MonTool_detectors"], channelNb, pmfNb );
           }

           if (layerNb >= 0) {

	   	ATH_MSG_DEBUG( "ROD data "<< "mbNb [counts from 0]: " << mbNb << " layerNb: " << layerNb << " channel: " << channel << " maroc2fib: " << m_maroc2fiber[channel] );
		Float_t data = m_mm_a_f[mbNb][layerNb][m_maroc2fiber[channel]];

	   	if (layerNb &0x1) {
	   		loc_pV[mbNb][layerNb>>1].push_back(data);
	   	}else {
	   		loc_pU[mbNb][layerNb>>1].push_back(data);
	   	}
           } else {
               // OD data
               int od_offset = (4 - pmf) * 64 + m_maroc2mapmt[channel];
               int side = m_od_channel2side[RPNumber][od_offset];

               if (m_od_channel2fiber[RPNumber][od_offset]<35) {
                  if (side==1) {
                          FiberHitsODPos[mbNb][m_od_channel2layer[RPNumber][od_offset]-1][m_od_channel2fiber[RPNumber][od_offset]-1] = true;
                   } else { 
                          FiberHitsODNeg[mbNb][m_od_channel2layer[RPNumber][od_offset]-1][m_od_channel2fiber[RPNumber][od_offset]-1] = true;
                  }
	   	  //ATH_MSG_INFO( "OD hit "<< "mbNb [counts from 0]: " << mbNb << "side: "<< side << " RPNumber: " << RPNumber << " od_offset: " << od_offset << " chan2layer: " << m_od_channel2layer[RPNumber][od_offset]-1  << 
                                            //"chan2fiber: "<<m_od_channel2fiber[RPNumber][od_offset]-1) ;
            }

           }
       }
       mask <<= 1;
    }
    return;
 }

uint32_t  TrigALFAROBMonitor::decodePMT0 (uint32_t dataWord) const {
         uint32_t mbNb = 0;
         int mask = 1;
         for (int index = 1; index <= 8; index++) {
                 if ( mask & dataWord ) {
                         mbNb = index;
                         break;
                 }
                 mask = mask<<1;
         }
         return mbNb;
 }


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
void TrigALFAROBMonitor::dumpRoIBDataWord(uint32_t data_word) const {

  if (msgLvl(MSG::DEBUG)) {
    ROIB::MuCTPIRoI roI(data_word);

    std::string loc = "UNDEFINED"; 
    if( roI.getSectorLocation() == MuCTPI_RDO::ENDCAP )
      loc = "ENDCAP";
    else if( roI.getSectorLocation() == MuCTPI_RDO::FORWARD )
      loc = "FORWARD";
    else if( roI.getSectorLocation() == MuCTPI_RDO::BARREL )
      loc = "BARREL";

    ATH_MSG_DEBUG( "RoIB word               : 0x"
	<< MSG::hex << roI.roIWord() << MSG::dec );
    ATH_MSG_DEBUG( "Threshold               :  pt" << roI.pt() );
    ATH_MSG_DEBUG( "Sector location         :  " << loc );
    std::string sectorOffset("");  
    if ((roI.getSectorAddress() & LVL1::MuCTPIBits::SECTOR_HEMISPHERE_MASK) &&
	(roI.getSectorLocation() == MuCTPI_RDO::BARREL)) sectorOffset = " + 32 for Hemisphere = 1 "; 
    ATH_MSG_DEBUG( "Sector ID               :  " << roI.getSectorID() << sectorOffset );
    ATH_MSG_DEBUG( "Sector addr             :  0x" << MSG::hex
	<< roI.getSectorAddress() << MSG::dec );
    ATH_MSG_DEBUG( "Sector overflow         :  " << roI.getSectorOverflow() );
    ATH_MSG_DEBUG( "RoI overflow            :  " << roI.getRoiOverflow() );
    ATH_MSG_DEBUG( "RoI number              :  " << roI.getRoiNumber() );
    ATH_MSG_DEBUG( "IsHighestPt             :  " << roI.getCandidateIsHighestPt() );
    ATH_MSG_DEBUG( "Overlap                 :  " << roI.getOverlapBits() );
    ATH_MSG_DEBUG( "Hemisphere              :  " << (roI.getSectorAddress() & LVL1::MuCTPIBits::SECTOR_HEMISPHERE_MASK) );
    ATH_MSG_DEBUG( "=================================================" );
  }
  return;
}

void TrigALFAROBMonitor::findALFATracks( const ROIB::RoIBResult* roIBResult, 
                                         const int lumiBlockNb, 
                                         std::vector<float> (&loc_pU) [8][10], 
                                         std::vector<float> (&loc_pV) [8][10]) const {
	float x_Rec[8];
	float y_Rec[8];
	
	float MeanPos_U=0;
	float MeanPos_V=0;
	
	float MeanCutPos_U=0;
	float MeanCutPos_V=0;
	
	float RecPos_U=0;
	float RecPos_V=0;
	
	float Closest_Fib_U = 0;
	float Closest_Fib_V = 0;
	
	int cnt_fib_U=0;
	int cnt_fib_V=0;

	int cnt_lay_U=0;
	int cnt_lay_V=0;

        std::vector <int> u_hits;
        std::vector <int> v_hits;

	float sign;
        int nbOfTracksInDetectors[8];

	for (int iDet=0;iDet<8;iDet++) {
		MeanPos_U=0;
		MeanPos_V=0;

		MeanCutPos_U=0;
		MeanCutPos_V=0;

		RecPos_U=0;
		RecPos_V=0;
		
		cnt_fib_U=0;
		cnt_fib_V=0;

                u_hits.clear();
                v_hits.clear();

                nbOfTracksInDetectors[iDet] = 0;                

                ATH_MSG_DEBUG( "findALFATracks starts" );

                for (int iLay=0;iLay<10;iLay++) {

                        if (loc_pU[iDet][iLay].size()<=3 && loc_pU[iDet][iLay].size()>0) u_hits.push_back(iLay);
                        if (loc_pV[iDet][iLay].size()<=3 && loc_pV[iDet][iLay].size()>0) v_hits.push_back(iLay);

                }

                ATH_MSG_DEBUG( "findALFATracks 1" <<"; idet: " << iDet );
                if (u_hits.size()>=3 && v_hits.size()>=3) {
                        ATH_MSG_DEBUG( "findALFATracks 2" );
                        for (int iLay=0;iLay<(int)u_hits.size();iLay++) {
                                for (int iFib=0; iFib<(int)loc_pU[iDet][u_hits[iLay]].size();iFib++) {
                                        MeanPos_U+=loc_pU[iDet][u_hits[iLay]][iFib];
					cnt_fib_U++;
                                }
			}

                        for (int iLay=0;iLay<(int)v_hits.size();iLay++) {
                                for (int iFib=0; iFib<(int)loc_pV[iDet][v_hits[iLay]].size();iFib++) {
                                        MeanPos_V+=loc_pV[iDet][v_hits[iLay]][iFib];
					cnt_fib_V++;
                                }
                        }

                        if (cnt_fib_U > 0) MeanPos_U/=float(cnt_fib_U);
                        if (cnt_fib_V > 0) MeanPos_V/=float(cnt_fib_V);

                        cnt_fib_U=0;
                        cnt_fib_V=0;

                        for (int iLay=0;iLay<(int)u_hits.size();iLay++) {
                                for (int iFib=0; iFib<(int)loc_pU[iDet][u_hits[iLay]].size();iFib++) {
                                        if (fabs(loc_pU[iDet][u_hits[iLay]][iFib]-MeanPos_U)<2.) {
                                                MeanCutPos_U+=loc_pU[iDet][u_hits[iLay]][iFib];
                                                cnt_fib_U++;
                                        }
                                }
			}
                        for (int iLay=0;iLay<(int)v_hits.size();iLay++) {
                                for (int iFib=0; iFib<(int)loc_pV[iDet][v_hits[iLay]].size();iFib++) {
                                        if (fabs(loc_pV[iDet][v_hits[iLay]][iFib]-MeanPos_V)<2.) {
                                                MeanCutPos_V+=loc_pV[iDet][v_hits[iLay]][iFib];
                                                cnt_fib_V++;
                                        }
                                }
                        }

                        if (cnt_fib_U > 0) MeanCutPos_U/=float(cnt_fib_U);
                        if (cnt_fib_V > 0) MeanCutPos_V/=float(cnt_fib_V);

                        cnt_lay_U=0;
                        cnt_lay_V=0;

                        for (int iLay=0;iLay<(int)u_hits.size();iLay++) {

                                cnt_fib_U=0;
                                float minDist=2.;
                                for (int iFib=0; iFib<(int)loc_pU[iDet][u_hits[iLay]].size();iFib++) {
                                        if (fabs(loc_pU[iDet][u_hits[iLay]][iFib]-MeanCutPos_U)<minDist) {
                                                minDist=fabs(loc_pU[iDet][u_hits[iLay]][iFib]-MeanCutPos_U);
                                                Closest_Fib_U=loc_pU[iDet][u_hits[iLay]][iFib];
                                                if (cnt_fib_U==0) cnt_fib_U++;
                                        }
                                }
                                if (cnt_fib_U==1) {RecPos_U+=Closest_Fib_U; cnt_lay_U++;}
			}

                        for (int iLay=0;iLay<(int)v_hits.size();iLay++) {
                                cnt_fib_V=0;
                                float minDist=2.;
                                for (int iFib=0; iFib<(int)loc_pV[iDet][v_hits[iLay]].size();iFib++) {
                                        if (fabs(loc_pV[iDet][v_hits[iLay]][iFib]-MeanCutPos_V)<minDist) {
                                                minDist=fabs(loc_pV[iDet][v_hits[iLay]][iFib]-MeanCutPos_V);
                                                Closest_Fib_V=loc_pV[iDet][v_hits[iLay]][iFib];
                                                if (cnt_fib_V==0) cnt_fib_V++;
                                        }
                                }
                                if (cnt_fib_V==1) {RecPos_V+=Closest_Fib_V; cnt_lay_V++;}
                        }

                        if (cnt_lay_U>3 && cnt_lay_V>3) {

                                if (iDet%2==0) sign=1.;
                                else sign=-1.;

                                RecPos_U/=float(cnt_lay_U);
                                RecPos_V/=float(cnt_lay_V);

                                x_Rec[iDet] = (RecPos_U-RecPos_V)/2.;
                                y_Rec[iDet] = sign*(-(RecPos_V+RecPos_U)/2.-115.);

                                const std::vector<ROIB::CTPRoI> ctpRoIVecBP = roIBResult->cTPResult().TBP();
                                ATH_MSG_DEBUG( "findALFATracks TBP size: " <<ctpRoIVecBP.size()<<" m_map_TrgItemNumbersToHistGroups.size() = "<< m_map_TrgItemNumbersToHistGroups.size() );
       		                //const std::vector<uint32_t>& itemsBP = resultL1.itemsBeforePrescale();
       		                if ( ctpRoIVecBP.size() > 0) {
                                   for (std::map<int, int>::const_iterator it = m_map_TrgItemNumbersToHistGroups.begin(); it != m_map_TrgItemNumbersToHistGroups.end(); ++it) {
                                          int word = it->first>>5;
                                          int offset = (it->first)%32;
                                          ATH_MSG_DEBUG( "findALFATracks access TBP at: " <<word<<" with offset: "<<offset );
                                          if ((ctpRoIVecBP.at(word)).roIWord() & 1<<offset) {
                                                ATH_MSG_DEBUG( "filling findALFATracks histos " );
           					{
               						std::string stationName =  m_trigConditions[it->second]+ "_" + m_stationNames[iDet];
               						auto x_coord = Monitored::Scalar<double>(stationName, x_Rec[iDet]);
               						auto y_coord = Monitored::Scalar<double>("y", y_Rec[iDet]);
                                                        std::string current, trk1, trk10, trk60;
                                                        if (it->second == 0) {
                                                            current = "trackingElast";
                                                            trk1 =    "trackingElast_1LB";
                                                            trk10 =   "trackingElast_10LB";
                                                            trk60 =   "trackingElast_60LB";
                                                        } else {
                                                            current = "trackingAny";
                                                            trk1 =    "trackingAny_1LB";
                                                            trk10 =   "trackingAny_10LB";
                                                            trk60 =   "trackingAny_60LB";
                                                        }
               						    auto monGroup =   Monitored::Group (  *m_monTools["MonTool_" + current], x_coord, y_coord );
               						    auto monGroup1 =  Monitored::Group (  *m_monTools["MonTool_" + trk1],    x_coord, y_coord );
               						    auto monGroup10 = Monitored::Group (  *m_monTools["MonTool_" + trk10],   x_coord, y_coord );
               						    auto monGroup60 = Monitored::Group (  *m_monTools["MonTool_" + trk60],   x_coord, y_coord );
           					}

                    			   }
                		   }
                                }
                                nbOfTracksInDetectors[iDet]++;
                                //HitMapAggr[iDet]->Fill(x_Rec[iDet],y_Rec[iDet]);
                        }
                        else {
                                x_Rec[iDet] = -9999.;
                                y_Rec[iDet] = -9999.;
                        }
                }
                else {
                          x_Rec[iDet] = -9999.;
                          y_Rec[iDet] = -9999.;
                }

        }

        const std::vector<ROIB::CTPRoI> ctpRoIVecBP = roIBResult->cTPResult().TBP();
        //const std::vector<uint32_t>& itemsBP = resultL1.itemsBeforePrescale();
        if ( (ctpRoIVecBP.size() >0) && (m_elast15>0) && (m_elast18>0) ) {
           if ((ctpRoIVecBP.at(m_elast15>>5)).roIWord() & (1 <<(m_elast15%32))) {
              {
                  std::string stationName  = "goodDataAssessmentLB15";
                  std::string stationName1 = "com_goodDataAssessment";
                  auto one      = Monitored::Scalar<double>(stationName, 1.);
                  auto anotherOne = Monitored::Scalar<double>(stationName1, 1.);
                  auto lbNb     = Monitored::Scalar<double>("com_LB", lumiBlockNb);
                  auto monGroup = Monitored::Group (  *m_monTools["MonTool_common"], one, anotherOne, lbNb );
              }
              if ((nbOfTracksInDetectors[0] <=2) && (nbOfTracksInDetectors[2] <=2) && (nbOfTracksInDetectors[5]<=2) && (nbOfTracksInDetectors[7] <= 2) &&
                    (nbOfTracksInDetectors[0]>0) && (nbOfTracksInDetectors[2] >0) && (nbOfTracksInDetectors[5]>0) && (nbOfTracksInDetectors[7] > 0) ) {
                 {
                     std::string stationName  = "goodDataAssessmentLB15";
                     std::string stationName1 = "com_goodDataAssessment";
                     auto two      = Monitored::Scalar<double>(stationName, 2.);
                     auto anotherTwo = Monitored::Scalar<double>(stationName1, 2.);
                     auto lbNb     = Monitored::Scalar<double>("com_LB", lumiBlockNb);
                     auto monGroup = Monitored::Group (  *m_monTools["MonTool_common"], two, anotherTwo, lbNb );
                 }
              }
           }
           if ((ctpRoIVecBP.at(m_elast18>>5)).roIWord() & (1 <<(m_elast18%32))) {
                 {
                     std::string stationName  = "goodDataAssessmentLB18";
                     std::string stationName1 = "com_goodDataAssessment";
                     auto one      = Monitored::Scalar<double>(stationName, 1.);
                     auto four = Monitored::Scalar<double>(stationName1, 4.);
                     auto lbNb     = Monitored::Scalar<double>("com_LB", lumiBlockNb);
                     auto monGroup = Monitored::Group (  *m_monTools["MonTool_common"], one, four, lbNb );
                 }
              if ((nbOfTracksInDetectors[1] <=2) && (nbOfTracksInDetectors[3] <=2) && (nbOfTracksInDetectors[4]<=2) && (nbOfTracksInDetectors[6] <= 2) &&
                    (nbOfTracksInDetectors[1]>0) && (nbOfTracksInDetectors[3] >0) && (nbOfTracksInDetectors[4]>0) && (nbOfTracksInDetectors[6] > 0) ) {
                 {
                     std::string stationName  = "goodDataAssessmentLB18";
                     std::string stationName1 = "com_goodDataAssessment";
                     auto two      = Monitored::Scalar<double>(stationName, 2.);
                     auto five = Monitored::Scalar<double>(stationName1, 5.);
                     auto lbNb     = Monitored::Scalar<double>("com-LB", lumiBlockNb);
                     auto monGroup = Monitored::Group (  *m_monTools["MonTool_common"], two, five, lbNb );
                 }
              }
           }
        }

        for(int i=0; i<8; i++) {
           ATH_MSG_DEBUG( "det: "<<i<<" - "<<nbOfTracksInDetectors[i]<<"; ");
        }

        const float dist = 8264.;
        const std::vector <std::string> triggers={"elast15", "elast18", "syst17", "syst18"};
        const std::vector <std::string> armSets={"L1U", "L1D", "R1U","R1D"};
        if ((ctpRoIVecBP.at(m_elast15>>5)).roIWord() & (1 <<(m_elast15%32))) {
           if ( (nbOfTracksInDetectors[0] == 1) && (nbOfTracksInDetectors[2] == 1) && (nbOfTracksInDetectors[5] == 1) && (nbOfTracksInDetectors[7] == 1) &&
                 (nbOfTracksInDetectors[1] == 0) && (nbOfTracksInDetectors[3] == 0) && (nbOfTracksInDetectors[4] == 0) && (nbOfTracksInDetectors[6] == 0) ) {
               if ( (x_Rec[0] > -9000.) && (x_Rec[2] > -9000.) && (x_Rec[7] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[0] + "_x_" + armSets[0];
                         std::string name1 = triggers[0] + "_ax_" + armSets[0]; 
                         auto pcx_x = Monitored::Scalar<float>(name,-x_Rec[0]);
                         auto pcx_y = Monitored::Scalar<float>("y", -x_Rec[7]);
                         auto pax_x = Monitored::Scalar<float>(name1,-x_Rec[2]);
                         auto pax_y = Monitored::Scalar<float>("y", (1000000.*(x_Rec[2] - x_Rec[0]))/dist);
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundElast15"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_1LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_10LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_60LB"], pcx_x, pcx_y, pax_x, pax_y);
                       }
               
               if ( (y_Rec[0] > -9000.) && (y_Rec[2] > -9000.) && (y_Rec[7] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[0] + "_y_" + armSets[0];
                         std::string name1 = triggers[0] + "_ay_" + armSets[0]; 
                         auto pcy_x = Monitored::Scalar<float>(name,y_Rec[0]);
                         auto pcy_y = Monitored::Scalar<float>("y", y_Rec[7]);
                         auto pay_x = Monitored::Scalar<float>(name1,y_Rec[2]);
                         auto pay_y = Monitored::Scalar<float>("y", (1000000.*(y_Rec[0] - y_Rec[2]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundElast15"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_1LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_10LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_60LB"], pcy_x, pcy_y, pay_x, pay_y);
                       }
               }
               if ( (x_Rec[5] > -9000.) && (x_Rec[7] > -9000.) && (x_Rec[2] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[0] + "_x_" + armSets[3];
                         std::string name1 = triggers[0] + "_ax_" + armSets[3]; 
                         auto pcx_x = Monitored::Scalar<float>(name,-x_Rec[2]);
                         auto pcx_y = Monitored::Scalar<float>("y", -x_Rec[5]);
                         auto pax_x = Monitored::Scalar<float>(name1,-x_Rec[5]);
                         auto pax_y = Monitored::Scalar<float>("y", (1000000.*(x_Rec[5] - x_Rec[7]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundElast15"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_1LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_10LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_60LB"], pcx_x, pcx_y, pax_x, pax_y);
                       }
               }
               if ( (y_Rec[5] > -9000.) && (y_Rec[7] > -9000.) && (y_Rec[2] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[0] + "_y_" + armSets[3];
                         std::string name1 = triggers[0] + "_ay_" + armSets[3]; 
                         auto pcy_x = Monitored::Scalar<float>(name,y_Rec[2]);
                         auto pcy_y = Monitored::Scalar<float>("y", y_Rec[5]);
                         auto pay_x = Monitored::Scalar<float>(name1,y_Rec[5]);
                         auto pay_y = Monitored::Scalar<float>("y",  (1000000.*(y_Rec[7] - y_Rec[5]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundElast15"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_1LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_10LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast15_60LB"], pcy_x, pcy_y, pay_x, pay_y);
                       }
               }
           }
         }
        }

        if ((ctpRoIVecBP.at(m_elast18>>5)).roIWord() & (1 <<(m_elast18%32))) {
           if ( (nbOfTracksInDetectors[1] == 1) && (nbOfTracksInDetectors[3] == 1) && (nbOfTracksInDetectors[4] == 1) && (nbOfTracksInDetectors[6] == 1) &&
                 (nbOfTracksInDetectors[0] == 0) && (nbOfTracksInDetectors[2] == 0) && (nbOfTracksInDetectors[5] == 0) && (nbOfTracksInDetectors[7] == 0) ) {
               if ( (x_Rec[1] > -9000.) && (x_Rec[3] > -9000.) && (x_Rec[6] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[1] + "_x_" + armSets[1];
                         std::string name1 = triggers[1] + "_ax_" + armSets[1]; 
                         auto pcx_x = Monitored::Scalar<float>(name,-x_Rec[1]);
                         auto pcx_y = Monitored::Scalar<float>("y", -x_Rec[6]);
                         auto pax_x = Monitored::Scalar<float>(name1,-x_Rec[3]);
                         auto pax_y = Monitored::Scalar<float>("y",  (1000000.*(x_Rec[3] - x_Rec[1]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundElast18"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_1LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_10LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_60LB"], pcx_x, pcx_y, pax_x, pax_y);
                       }
               }
               if ( (y_Rec[1] > -9000.) && (y_Rec[3] > -9000.) && (y_Rec[6] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[1] + "_y_" + armSets[1];
                         std::string name1 = triggers[1] + "_ay_" + armSets[1]; 
                         auto pcy_x = Monitored::Scalar<float>(name,y_Rec[1]);
                         auto pcy_y = Monitored::Scalar<float>("y", y_Rec[6]);
                         auto pay_x = Monitored::Scalar<float>(name1,y_Rec[3]);
                         auto pay_y = Monitored::Scalar<float>("y",  (1000000.*(y_Rec[1] - y_Rec[3]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundElast18"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_1LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_10LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_60LB"], pcy_x, pcy_y, pay_x, pay_y);
                       }
               }
               if ( (x_Rec[4] > -9000.) && (x_Rec[6] > -9000.) && (x_Rec[3] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[1] + "_x_" + armSets[2];
                         std::string name1 = triggers[1] + "_ax_" + armSets[2]; 
                         auto pcx_x = Monitored::Scalar<float>(name,-x_Rec[3]);
                         auto pcx_y = Monitored::Scalar<float>("y", -x_Rec[4]);
                         auto pax_x = Monitored::Scalar<float>(name1,-x_Rec[4]);
                         auto pax_y = Monitored::Scalar<float>("y", (1000000.*(x_Rec[4] - x_Rec[6]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundElast18"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_1LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_10LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_60LB"], pcx_x, pcx_y, pax_x, pax_y);
                       }
               }
               if ( (y_Rec[4] > -9000.) && (y_Rec[6] > -9000.) && (y_Rec[3] > -9000.)  ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[1] + "_y_" + armSets[2];
                         std::string name1 = triggers[1] + "_ay_" + armSets[2]; 
                         auto pcy_x = Monitored::Scalar<float>(name,y_Rec[3]);
                         auto pcy_y = Monitored::Scalar<float>("y", y_Rec[4]);
                         auto pay_x = Monitored::Scalar<float>(name1,y_Rec[4]);
                         auto pay_y = Monitored::Scalar<float>("y",  (1000000.*(y_Rec[6] - y_Rec[4]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundElast18"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_1LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_10LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundElast18_60LB"], pcy_x, pcy_y, pay_x, pay_y);
                       }
               }
           }
         }

        if ((ctpRoIVecBP.at(m_syst17>>5)).roIWord() & (1 <<(m_syst17%32))) {
           //if ( (m_nbOfTracksInDetectors[0] >= 1) && (m_nbOfTracksInDetectors[2] >= 1) && (m_nbOfTracksInDetectors[4] >= 1) && (m_nbOfTracksInDetectors[6] >= 1) &&
             if (1) {  // (m_nbOfTracksInDetectors[1] == 0) && (m_nbOfTracksInDetectors[3] == 0) && (m_nbOfTracksInDetectors[5] == 0) && (m_nbOfTracksInDetectors[7] == 0) ) {
               //ATH_MSG_INFO(" m_syst17 fired: xrec0: "<<x_Rec[0]<<" xrec2: "<<x_Rec[2]<<" xrec4: "<<x_Rec[4]<<" xrec6: "<<x_Rec[6] );
               if ( (x_Rec[0] > -9000.) && (x_Rec[2] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[2] + "_x_" + armSets[0];
                         std::string name1 = triggers[2] + "_ax_" + armSets[0]; 
                         auto pcx_x = Monitored::Scalar<float>(name,-x_Rec[0]);
                         auto pcx_y = Monitored::Scalar<float>("y", -x_Rec[2]);
                         auto pax_x = Monitored::Scalar<float>(name1,-x_Rec[2]);
                         auto pax_y = Monitored::Scalar<float>("y", (1000000.*(x_Rec[2] - x_Rec[0]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_1LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_10LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_60LB"], pcx_x, pcx_y, pax_x, pax_y);
                       }
               }
               if ( (y_Rec[0] > -9000.) && (y_Rec[2] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[2] + "_y_" + armSets[0];
                         std::string name1 = triggers[2] + "_ay_" + armSets[0]; 
                         auto pcy_x = Monitored::Scalar<float>(name,y_Rec[0]);
                         auto pcy_y = Monitored::Scalar<float>("y", y_Rec[2]);
                         auto pay_x = Monitored::Scalar<float>(name1, y_Rec[2]);
                         auto pay_y = Monitored::Scalar<float>("y",  (1000000.*(y_Rec[0] - y_Rec[2]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_1LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_10LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_60LB"], pcy_x, pcy_y, pay_x, pay_y);
                       }
               }
               if ( (x_Rec[4] > -9000.) && (x_Rec[6] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[2] + "_x_" + armSets[2];
                         std::string name1 = triggers[2] + "_ax_" + armSets[2]; 
                         auto pcx_x = Monitored::Scalar<float>(name,-x_Rec[6]);
                         auto pcx_y = Monitored::Scalar<float>("y", -x_Rec[4]);
                         auto pax_x = Monitored::Scalar<float>(name1, -x_Rec[4]);
                         auto pax_y = Monitored::Scalar<float>("y", (1000000.*(x_Rec[4] - x_Rec[6]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_1LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_10LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_60LB"], pcx_x, pcx_y, pax_x, pax_y);
                       }
               }
               if ( (y_Rec[4] > -9000.) && (y_Rec[6] > -9000.)) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[2] + "_y_" + armSets[2];
                         std::string name1 = triggers[2] + "_ay_" + armSets[2]; 
                         auto pcy_x = Monitored::Scalar<float>(name,y_Rec[6]);
                         auto pcy_y = Monitored::Scalar<float>("y", y_Rec[4]);
                         auto pay_x = Monitored::Scalar<float>(name1, y_Rec[4]);
                         auto pay_y = Monitored::Scalar<float>("y",  (1000000.*(y_Rec[6] - y_Rec[4]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_1LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_10LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst17_60LB"], pcy_x, pcy_y, pay_x, pay_y);
                       }
               }
           }
         }

        if ((ctpRoIVecBP.at(m_syst18>>5)).roIWord() & (1 <<(m_syst18%32))) {
            //if ( (m_nbOfTracksInDetectors[1] >= 1) && (m_nbOfTracksInDetectors[3] >= 1) && (m_nbOfTracksInDetectors[5] >= 1) && (m_nbOfTracksInDetectors[7] >= 1) &&
              if (1) { //((m_nbOfTracksInDetectors[0] == 0) && (m_nbOfTracksInDetectors[2] == 0) && (m_nbOfTracksInDetectors[4] == 0) && (m_nbOfTracksInDetectors[6] == 0) ) {
               //ATH_MSG_INFO(" m_syst18 fired: xrec1: "<<x_Rec[1]<<" xrec3: "<<x_Rec[3]<<" xrec5: "<<x_Rec[5]<<" xrec7: "<<x_Rec[7] );
               if ( (x_Rec[1] > -9000.) && (x_Rec[3] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[3] + "_x_" + armSets[1];
                         std::string name1 = triggers[3] + "_ax_" + armSets[1]; 
                         auto pcx_x = Monitored::Scalar<float>(name,-x_Rec[1]);
                         auto pcx_y = Monitored::Scalar<float>("y", -x_Rec[3]);
                         auto pax_x = Monitored::Scalar<float>(name1, -x_Rec[3]);
                         auto pax_y = Monitored::Scalar<float>("y", (1000000.*(x_Rec[3] - x_Rec[1]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_1LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_10LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_60LB"], pcx_x, pcx_y, pax_x, pax_y);
                       }
               }
               if ( (y_Rec[1] > -9000.) && (y_Rec[3] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[3] + "_y_" + armSets[1];
                         std::string name1 = triggers[3] + "_ay_" + armSets[1]; 
                         auto pcy_x = Monitored::Scalar<float>(name,y_Rec[1]);
                         auto pcy_y = Monitored::Scalar<float>("y", y_Rec[3]);
                         auto pay_x = Monitored::Scalar<float>(name1, y_Rec[3]);
                         auto pay_y = Monitored::Scalar<float>("y",  (1000000.*(y_Rec[1] - y_Rec[3]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_1LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_10LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_60LB"], pcy_x, pcy_y, pay_x, pay_y);
                       }
               }
               if ( (x_Rec[5] > -9000.) && (x_Rec[7] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[3] + "_x_" + armSets[3];
                         std::string name1 = triggers[3] + "_ax_" + armSets[3]; 
                         auto pcx_x = Monitored::Scalar<float>(name,-x_Rec[7]);
                         auto pcx_y = Monitored::Scalar<float>("y", -x_Rec[5]);
                         auto pax_x = Monitored::Scalar<float>(name1, -x_Rec[5]);
                         auto pax_y = Monitored::Scalar<float>("y", (1000000.*(x_Rec[5] - x_Rec[7]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_1LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_10LB"], pcx_x, pcx_y, pax_x, pax_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_60LB"], pcx_x, pcx_y, pax_x, pax_y);
                       }
               }
               if ( (y_Rec[5] > -9000.)&& (y_Rec[7] > -9000.) ) {
                       {
                         ATH_MSG_DEBUG( "filling bckg histos " );
                         std::string name = triggers[3] + "_y_" + armSets[3];
                         std::string name1 = triggers[3] + "_ay_" + armSets[3]; 
                         auto pcy_x = Monitored::Scalar<float>(name,y_Rec[7]);
                         auto pcy_y = Monitored::Scalar<float>("y", y_Rec[5]);
                         auto pay_x = Monitored::Scalar<float>(name1, y_Rec[5]);
                         auto pay_y = Monitored::Scalar<float>("y", (1000000.*(y_Rec[7] - y_Rec[5]))/dist );
                         auto monGroup = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_1LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_1LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_10LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_10LB"], pcy_x, pcy_y, pay_x, pay_y);
                         auto monGroup_60LB = Monitored::Group ( *m_monTools["MonTool_backgroundSyst18_60LB"], pcy_x, pcy_y, pay_x, pay_y);
                       }
               }
           }
         }

   return;
}  

void TrigALFAROBMonitor::findODTracks( bool FiberHitsODNeg[][3][30], bool FiberHitsODPos[][3][30], std::map<int,int>& triggerHitPattern,std::map<int,int>& triggerHitPatternReady ) const{

    int Multiplicity[3], FibHit, Index=10;
    bool FoundTrack[2];
    double Pos[2];

    float ODtracks[8][2];

    // initialize to positive values arrays which will be used in distance measurements with ODs - data needed accross both ROBs (RODs).
    for (int detector=0; detector <8; detector++)
      for (int side=0; side<2; side++)
           ODtracks[detector][side] = 1.;

    for (int iStation=0;iStation<4;iStation++){
        for (int iSide=0;iSide<2;iSide++){
            FibHit=-1;

            FoundTrack[0] = false;
            FoundTrack[1] = false;
            if (iSide==0){//If we are in the positive side of the detectors

                //Loop over Upper and Lower detector of a station. Both detectors should have a track
                for (int iUL=0;iUL<2;iUL++){
                    Pos[iUL]=0;
                    if ( ! (triggerHitPatternReady[iStation*2+iUL])) {
                        continue;
                    } else {
                        if ( !(triggerHitPattern[iStation*2+iUL] &0x8000))
                            continue;
                    }
                    //ATH_MSG_INFO ("in DO search tracks : "<<m_triggerHitPattern[iStation*2+iUL] );
                    int MinMultipl = 60;

                    //Loop over OD layers to search for tracks where at least one layer has only one hit.
                    for (int iLay=0;iLay<3;iLay++){

                        Multiplicity[iLay]=0;
                        for (int iFib=0;iFib<30;iFib++){
                           if (FiberHitsODPos[iStation*2+iUL][iLay][iFib]){
                                FibHit = iFib;
                                Multiplicity[iLay] ++;
                           }
                        }
                        if (Multiplicity[iLay] < MinMultipl) MinMultipl = Multiplicity[iLay];
                           // hMultipl[iStation*2+iUL][iSide][iLay]->Fill(Multiplicity[iLay]);
                        if (FibHit==-1) continue;

                        //Go through the LookUpTable for this fiber hit: (the multiplicity for the layer MUST be equal to one.
                        //In all the following places where I have "sFiberHitsODPos" we need to rewrite it with the fiber_to_maroc variables
                        if (FoundTrack[iUL]) continue;
                        for (int iFib1 = (FibHit>0 ? FibHit-1 : 0); iFib1< (FibHit<29 ? FibHit+2 : 30) && Multiplicity[iLay]==1;iFib1++){
                            for (int iFib2 = (FibHit>0 ? FibHit-1 : 0); iFib2< (FibHit<29 ? FibHit+2 : 30);iFib2++){
                                if (abs(iFib1-iFib2)>1) continue;
                                if((FibHit-iFib1 == -1) && (FibHit-iFib2 == -1)) Index = 6;
                                if((FibHit-iFib1 == -1) && (FibHit-iFib2 == 0)) Index = 5;
                                if((FibHit-iFib1 == 0) && (FibHit-iFib2 == -1)) Index = 4;
                                if((FibHit-iFib1 == 0) && (FibHit-iFib2 == 0)) Index = 3;
                                if((FibHit-iFib1 == 0) && (FibHit-iFib2 == 1)) Index = 2;
                                if((FibHit-iFib1 == 1) && (FibHit-iFib2 == 0)) Index = 1;
                                if((FibHit-iFib1 == 1) && (FibHit-iFib2 == 1)) Index = 0;
                                if (FiberHitsODPos[iStation*2+iUL][(iLay+1)%3][iFib1] && FiberHitsODPos[iStation*2+iUL][(iLay+2)%3][iFib2]){
                                    if (iFib1<29 && FiberHitsODPos[iStation*2+iUL][(iLay+1)%3][iFib1+1]){
                                        if (iFib2<29 && FiberHitsODPos[iStation*2+iUL][(iLay+2)%3][iFib2+1]){
                                            Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+3*7+Index];
                                            if (Pos[iUL]>-10){
                                                Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+2*7+Index];
                                                if (Pos[iUL]>-10){
                                                    Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+1*7+Index];
                                                    if (Pos[iUL]>-10){
                                                        Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+0*7+Index];
                                                    }
                                                }
                                            }
                                        }
                                        else{
                                            Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+1*7+Index];
                                            if (Pos[iUL]>-10) Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+0*7+Index];
                                        }
                                    }
                                    else{
                                        if (iFib2<29 && FiberHitsODPos[iStation*2+iUL][(iLay+2)%3][iFib2+1]){
                                            Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+2*7+Index];
                                            if (Pos[iUL]>-10) Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+0*7+Index];
                                        }
                                        else Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+0*7+Index];
                                    }
                                }
                                if (Pos[iUL]<-10) { 
                                    FoundTrack[iUL]=true;
                                    break;
                                }
                                if (FoundTrack[iUL]) break;
                            }
                            if (FoundTrack[iUL]) break;
                        }
                    }//end of iLay-loop
                }//end of iUL-loop

            } else {//The same just for the negative side
                   
                for (int iUL=0;iUL<2;iUL++){
                    Pos[iUL]=0;
                 FoundTrack[iUL] = false;
                    if ( ! (triggerHitPatternReady[iStation*2+iUL])) {
                        continue;
                    } else {
                       if ( !(triggerHitPattern[iStation*2+iUL] &0x2000))
                        continue;
                       }
                    int MinMultipl = 60;
                    for (int iLay=0;iLay<3;iLay++){
                        Multiplicity[iLay]=0;
                        for (int iFib=0;iFib<30;iFib++){
                            if (FiberHitsODNeg[iStation*2+iUL][iLay][iFib]){
                                FibHit = iFib;
                                Multiplicity[iLay] ++;
                            }
                        }

                        if (Multiplicity[iLay] < MinMultipl) MinMultipl = Multiplicity[iLay];
                               //           hMultipl[iStation*2+iUL][iSide][iLay]->Fill(Multiplicity[iLay]);
                        if (FibHit==-1) continue;

                        if (FoundTrack[iUL]) continue;
                        for (int iFib1 = (FibHit>0 ? FibHit-1 : 0); iFib1< (FibHit<29 ? FibHit+2 : 30) && Multiplicity[iLay]==1;iFib1++){
                            for (int iFib2 = (FibHit>0 ? FibHit-1 : 0); iFib2< (FibHit<29 ? FibHit+2 : 30);iFib2++){
                                if (abs(iFib1-iFib2)>1) continue;
                                if((FibHit-iFib1 == -1) && (FibHit-iFib2 == -1)) Index = 6;
                                if((FibHit-iFib1 == -1) && (FibHit-iFib2 == 0)) Index = 5;
                                if((FibHit-iFib1 == 0) && (FibHit-iFib2 == -1)) Index = 4;
                                if((FibHit-iFib1 == 0) && (FibHit-iFib2 == 0)) Index = 3;
                                if((FibHit-iFib1 == 0) && (FibHit-iFib2 == 1)) Index = 2;
                                if((FibHit-iFib1 == 1) && (FibHit-iFib2 == 0)) Index = 1;
                                if((FibHit-iFib1 == 1) && (FibHit-iFib2 == 1)) Index = 0;
                                if (FiberHitsODNeg[iStation*2+iUL][(iLay+1)%3][iFib1] && FiberHitsODNeg[iStation*2+iUL][(iLay+2)%3][iFib2]){
                                    if (iFib1<29 && FiberHitsODNeg[iStation*2+iUL][(iLay+1)%3][iFib1+1]){
                                        if (iFib2<29 && FiberHitsODNeg[iStation*2+iUL][(iLay+2)%3][iFib2+1]){
                                            Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+3*7+Index];
                                            if (Pos[iUL]>-10){
                                                Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+2*7+Index];
                                                if (Pos[iUL]>-10){
                                                    Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+1*7+Index];
                                                    if (Pos[iUL]>-10){
                                                        Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+0*7+Index];
                                                    }
                                                }
                                            }
                                        }
                                        else{
                                            Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+1*7+Index];
                                            if (Pos[iUL]>-10) Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+0*7+Index];
                                        }
                                    }
                                    else{
                                        if (iFib2<29 && FiberHitsODNeg[iStation*2+iUL][(iLay+2)%3][iFib2+1]){
                                            Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+2*7+Index];
                                            if (Pos[iUL]>-10) Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+0*7+Index];
                                        }
                                        else Pos[iUL] = m_LUT[(iStation*2+iUL)*6+iSide*3+iLay][FibHit*28+0*7+Index];
                                    }
                                }
                                if (Pos[iUL]<-10) {
                                   FoundTrack[iUL]=true;
                                   break;
                                }
                            }
                            if (FoundTrack[iUL]) break;
                        }
                        if (FoundTrack[iUL]) break;
                    }//end of iLay-loop
                }//end of iUL-loop
            }

            //If we have a track in both upper and lower detector, we fill the histograms
            if (FoundTrack[0]) { 
                 {
                     std::string stationName  = "od_" + m_stationNames[iStation*2] + "_RP_" + std::to_string(iStation*2+1) + "_" + std::to_string(iSide) + " position";
                     auto pos    = Monitored::Scalar<double>(stationName, Pos[0]);
                     auto monGroup = Monitored::Group (  *m_monTools["MonTool_OD_" + m_stationNames[iStation*2]], pos );
                 }

                 ODtracks[iStation*2][iSide] = Pos[0];
            }
            if (FoundTrack[1]) {
                 {
                     std::string stationName  = "od_" + m_stationNames[iStation*2+1] + "_RP_" + std::to_string(iStation*2+2) + "_" + std::to_string(iSide) + " position";
                     auto pos    = Monitored::Scalar<double>(stationName, Pos[1]);
                     auto monGroup = Monitored::Group (  *m_monTools["MonTool_OD_" + m_stationNames[iStation*2+1]], pos );
                 }
                 ODtracks[iStation*2+1][iSide] = Pos[1];
            }
            //if (FoundTrack[0] && FoundTrack[1]){
            if( (ODtracks[iStation*2][iSide] < 0) && (ODtracks[iStation*2+1][iSide] < 0) ) {
              {
                  std::string stationName  = "od_" + m_stationNames[iStation*2] + "_distance_" + std::to_string(iStation*2+1) + "_" + std::to_string(iStation*2+2)+ "_side_" + std::to_string(iSide);
                  auto pos    = Monitored::Scalar<double>(stationName, -ODtracks[iStation*2][iSide] - ODtracks[iStation*2+1][iSide] + m_alfa_edge[iStation*2] + m_alfa_edge[iStation*2+1]);
                  auto monGroup = Monitored::Group (  *m_monTools["MonTool_OD_" +  m_stationNames[iStation*2]], pos );
              }

            }

        }//end of iSide-loop
    }//end of iStation-loop
    ATH_MSG_DEBUG ("end of findOD tracks");
}








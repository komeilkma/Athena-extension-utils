/*
  Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
*/

#include "AthenaBaseComps/AthReentrantAlgorithm.h"
#include "StoreGate/ReadHandleKey.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/ITHistSvc.h"
#include "GaudiKernel/HistoProperty.h"
#include "ByteStreamData/RawEvent.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "eformat/Status.h"
#include <stdint.h>

#include "TrigConfData/L1Menu.h"
#include "TrigT1Result/RoIBResult.h"

#include "AthenaMonitoringKernel/Monitored.h"

#include <initializer_list>

/////////////////////////////////////////////////////////////////////////////

namespace ROIB {
  class MuCTPIResult;
}

class MuCTPI_RDO;
class ITrigROBDataProviderSvc;

class TrigALFAROBMonitor:public AthReentrantAlgorithm {
public:
  TrigALFAROBMonitor(const std::string& name, ISvcLocator* pSvcLocator);

  virtual StatusCode initialize() override;
  virtual StatusCode execute(const EventContext& ctx) const override;
  virtual StatusCode start() override;

private:
	
  /**
   * @brief Accessor method for the message level variable.
   * @return value of the message level for this algorithm.
   */


  ServiceHandle<IROBDataProviderSvc>           m_robDataProviderSvc;
  SG::ReadHandleKey<TrigConf::L1Menu> m_L1MenuKey  { this, "L1TriggerMenu", "DetectorStore+L1TriggerMenu", "L1 Menu" };
  SG::ReadHandleKey<ROIB::RoIBResult> m_RBResultKey{ this, "RoIBBResultRHKey", "RoIBResult", "StoreGate key for reading RoIB results" };


  /// Source identifiers for ROB fragments
  IntegerProperty                  m_lvl1CTPROBid ;
  IntegerProperty                  m_lvl1ALFA1ROBid ;
  IntegerProperty                  m_lvl1ALFA2ROBid ;
  IntegerProperty                  m_daqCTPROBid ;

  UnsignedIntegerProperty          m_ctpModuleID;

  /// Switch for ROB checksum test
  BooleanProperty                  m_doROBChecksum;

  /// Switch for ALFA fast online tracking
  BooleanProperty                  m_doALFATracking;
  BooleanProperty                  m_doPMFMonitoring;
  BooleanProperty                  m_doDataGoodMonitoring;
  BooleanProperty                  m_doODDistance;

  /// pointers to the CTP and muCTPi result objects
  ROIB::MuCTPIResult*              m_lvl1muCTPIResult;  // RoIB muCTPi Result

  ToolHandleArray<GenericMonitoringTool> m_monTools{this, "MonTools", {}, "Monitoring tools"};

  std::vector<uint32_t> m_ALFARobIds;

  std::map<std::string, int> m_map_TrgNamesToHistGroups;
  std::map<int, int>         m_map_TrgItemNumbersToHistGroups;

  std::string m_pathHisto;

  bool m_inTDAQPart = false;

  int m_elast15 {0}, m_elast18 {0},  m_syst17 {0}, m_syst18 {0};     // ctp-items id numbers to select golden alfa trigger for data quality assesment

// ALFA extensions
// geometry data
  const int m_mbNb2RP[8]={2,1,8,3,5,6,7,4};
  const int m_maroc2mapmt[64]={56,32,40,41,24,33,16,25,48,17,57,18,26,43,59,51,35,49,60,58,52,61,44,54,62,63,50,53,34,45,36,42,46,55,27,37,31,47,28,38,39,30,23,10,29,15,22,14,20,9,7,21,19,4,12,6,13,5,11,3,2,1,8,0}; 
  const int m_maroc2fiber[64]={0,6,4,12,1,14,3,9,2,11,8,19,17,28,24,26,30,10,32,16,34,40,36,50,48,56,18,42,22,44,38,20,52,58,25,46,57,60,33,54,62,49,59,21,41,61,51,53,35,13,63,43,27,39,37,55,45,47,29,31,23,15,5,7};
  const int m_pmf2layer[24]={0,19,-1,-1,-1,15,17,18,14,16,11,13,10,12,7,9,1,6,8,3,5,0,2,4};

#include "../src/TrigALFAROBMon_geomTable.icc"

  const float m_y_min[2] = {0.,-35.};
  const float m_y_max[2] = {35.,0.};


  const std::vector<std::string> m_stationNames {"B7L1U", "B7L1L", "A7L1U", "A7L1L", "A7R1U", "A7R1L", "B7R1U", "B7R1L"};
  //const std::vector<std::string> m_trigConditions{"elastic", "elastic_ALFA_BG", "singleDiffr", "ALFA_MBTS_singleDiffr", "ALFA_LUCID_singleDiffr", "ALFA_EM3", "ALFA_J12", "ALFA_TRT", "ANY", "ANY_UNPAIRED_ISO", "ANY_ALFA_BG", "ALFA_EMPTY"};
  const std::vector<std::string> m_trigConditions{"elastic",  "ANY"};


  /// Helper for checksum test
  /// returns true if a ROB checksum failed
  bool verifyROBChecksum(const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment& robFrag) const;

  bool verifyALFAROBChecksum(const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment& robFrag) const;

  /// Helper for decoding the ALFA ROB 
  uint32_t  decodeALFA(const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment& robFrag, std::vector<float> (&loc_pU) [8][10], 
                       std::vector<float> (&loc_pV) [8][10],
                       bool FiberHitsODNeg[][3][30], bool FiberHitsODPos[][3][30],
                       std::map<int,int>& triggerHitPattern, std::map<int,int>& triggerHitPatternReady) const;

  void   decodeRealPMT(uint32_t dataWord, uint32_t quarter, uint32_t mbNb, uint32_t pmf, std::vector<float> (&loc_pU) [8][10], 
                       std::vector<float> (&loc_pV) [8][10],
                       bool FiberHitsODNeg[][3][30], bool FiberHitsODPos[][3][30]) const;

  uint32_t  decodePMT0(uint32_t dataWord) const;

  /// find tacks in ALFA detectors
  void findALFATracks(const ROIB::RoIBResult* roIBResult, const int lumiBlockNb, 
                      std::vector<float> (&loc_pU) [8][10], std::vector<float> (&loc_pV) [8][10]) const;

  // find OD tracks and calculate distance
  void findODTracks (bool FiberHitsODNeg[][3][30], bool FiberHitsODPos[][3][30], std::map<int,int>& triggerHitPattern,std::map<int,int>& triggerHitPatternReady) const;

  /// Helper to print contents of a muCTPi RoIB data word 
  void dumpRoIBDataWord(uint32_t data_word ) const;
};

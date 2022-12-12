/*
  Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration
*/

#include "TrigOpMonitor.h"

#include "AthenaInterprocess/Incidents.h"
#include "AthenaKernel/IOVRange.h"
#include "AthenaMonitoringKernel/OHLockedHist.h"
#include "ByteStreamData/ByteStreamMetadata.h"
#include "ByteStreamData/ByteStreamMetadataContainer.h"
#include "StoreGate/ReadCondHandle.h"
#include "GaudiKernel/DirSearchPath.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "Gaudi/Interfaces/IOptionsSvc.h"
#include "eformat/DetectorMask.h"
#include "eformat/SourceIdentifier.h"

#include <algorithm>
#include <fstream>
#include <list>

// Helpers
namespace {
  /// Sanitize COOL folder names
  std::string folder2HistName(const std::string& folder)
  {
    std::string s(folder);
    if (s.front() == '/') s.erase(0, 1);
    std::replace(s.begin(), s.end(), '/', '_');
    return s;
  }

  /// Read release metadata file from InstallArea/$BINARY_TAG/ReleaseData
  StatusCode readReleaseData(const std::string& file, std::map<std::string, std::string>& result)
  {
    std::ifstream f;
    f.open(file.c_str(), std::ifstream::in);
    if (!f.is_open()) return StatusCode::FAILURE;

    std::string line;
    while (getline(f, line)) {
      // Format: name:value
      std::string::size_type loc = line.find(':');
      if (loc == std::string::npos || loc == 0) continue;
      result[line.substr(0, loc)] = line.substr(loc + 1);
    }
    f.close();
    return StatusCode::SUCCESS;
  }

} // namespace

TrigOpMonitor::TrigOpMonitor(const std::string& name, ISvcLocator* pSvcLocator) :
  base_class(name, pSvcLocator),
  m_histPath("/EXPERT/HLTFramework/" + name + "/")
{}

StatusCode TrigOpMonitor::initialize()
{
  ATH_CHECK(m_histSvc.retrieve());
  ATH_CHECK(m_lumiDataKey.initialize(SG::AllowEmpty));
  ATH_CHECK(m_fieldMapKey.initialize(SG::AllowEmpty));

  ATH_CHECK( m_incidentSvc.retrieve() );
  m_incidentSvc->addListener(this, AthenaInterprocess::UpdateAfterFork::type());

  return StatusCode::SUCCESS;
}

void TrigOpMonitor::handle( const Incident& incident ) {
  // One time fills after fork
  if (incident.type() == AthenaInterprocess::UpdateAfterFork::type()) {
    if (!m_IOVDbSvc) service("IOVDbSvc", m_IOVDbSvc, /*createIf=*/false).ignore();
    fillIOVDbHist();
    fillSubDetHist();
    const AthenaInterprocess::UpdateAfterFork& updinc = dynamic_cast<const AthenaInterprocess::UpdateAfterFork&>(incident);
    fillProcThreadHist(updinc.workerID());
  }
}

StatusCode TrigOpMonitor::start()
{
  m_previousLB = 0;
  if (!m_IOVDbSvc) service("IOVDbSvc", m_IOVDbSvc, /*createIf=*/false).ignore();
  ATH_CHECK(bookHists());

  fillReleaseDataHist();
  return StatusCode::SUCCESS;
}

StatusCode TrigOpMonitor::execute()
{
  /* One-time fills */
  [[maybe_unused]] const static bool once = [&]() {
    fillMagFieldHist();
    return true;
  }();

  /* Per-LB fills */
  const EventContext& ctx = getContext();
  if (m_previousLB != ctx.eventID().lumi_block()) { // New LB
    m_previousLB = ctx.eventID().lumi_block();

    // Fill IOV diff histogram
    fillIOVDbChangeHist(ctx);

    // Fill lumi histogram
    fillLumiHist(ctx);
  }

  return StatusCode::SUCCESS;
}

StatusCode TrigOpMonitor::bookHists()
{
  m_iovChangeHist = nullptr;
  m_currentIOVs.clear();
  m_folderHist.clear();

  m_iovChangeHist = new TH2I("CoolFolderUpdate_LB", "COOL folder updates;Lumiblock;", 1, 0, 1, 1, 0, 1);

  // create histogram for magnetic field information
  m_magFieldHist = new TH2I("MagneticFieldSettings", "Magnetic field settings", 2, 0, 2, 2, 0, 2);
  m_magFieldHist->GetYaxis()->SetBinLabel(1, "off");
  m_magFieldHist->GetYaxis()->SetBinLabel(2, "on");
  m_magFieldHist->GetXaxis()->SetBinLabel(1, "Solenoid");
  m_magFieldHist->GetXaxis()->SetBinLabel(2, "Toroid");

  m_releaseHist = new TH1I("GeneralOpInfo", "General operational info;;Applications", 1, 0, 1);

  m_mtConfigHist = new TH2I("MTConfig", "Multi-threading configuration", 2, 0, 2, 10, 0, 10);
  m_mtConfigHist->SetCanExtend(TH1::kYaxis);
  m_mtConfigHist->GetXaxis()->SetBinLabel(1, "N threads");
  m_mtConfigHist->GetXaxis()->SetBinLabel(2, "N slots");

  m_workersHist = new TH1I("MPWorkers", "Worker IDs;Worker ID;Number of workers", 10, 0, 10);
  m_workersHist->SetCanExtend(TH1::kXaxis);

  m_subdetHist = new TH2I("Subdetectors", "State of subdetectors", 1, 0, 1, 3, 0, 3);
  m_subdetHist->SetCanExtend(TH1::kXaxis);
  m_subdetHist->GetYaxis()->SetBinLabel(1, "# ROB");
  m_subdetHist->GetYaxis()->SetBinLabel(2, "off");
  m_subdetHist->GetYaxis()->SetBinLabel(3, "on");

  m_lumiHist = new TProfile("Luminosity", "Luminosity;Lumiblock;Luminosity [10^{33} cm^{-2}s^{-1}]",
                            m_maxLB, 0, m_maxLB);

  m_muHist = new TProfile("Pileup", "Pileup;Lumiblock;Interactions per BX", m_maxLB, 0, m_maxLB);

  // Register histograms
  TH1* hist[] = {
    m_releaseHist, m_mtConfigHist, m_workersHist, m_subdetHist,
    m_iovChangeHist, m_magFieldHist, m_lumiHist, m_muHist
  };
  for (TH1* h : hist) {
    if (h) ATH_CHECK(m_histSvc->regHist(m_histPath + h->GetName(), h));
  }

  return StatusCode::SUCCESS;
}

void TrigOpMonitor::fillMagFieldHist()
{
  if (!m_fieldMapKey.empty()) {
    SG::ReadCondHandle<AtlasFieldMapCondObj> fieldMapHandle(m_fieldMapKey);
    const MagField::AtlasFieldMap* fieldMap = fieldMapHandle->fieldMap();
    // if there is no field map, the field is off
    m_magFieldHist->Fill("Solenoid", fieldMap && fieldMap->solenoidOn() ? "on" : "off", 1.0);
    m_magFieldHist->Fill("Toroid", fieldMap && fieldMap->toroidOn() ? "on" : "off", 1.0);
  }
}

void TrigOpMonitor::fillIOVDbHist()
{
  // create and fill histogram for IOVDb entries
  std::vector<std::string> keyList;
  if (m_IOVDbSvc) keyList = m_IOVDbSvc->getKeyList();

  // set up histograms
  TH2I* IOVDbRunHist = new TH2I("IOVDbRunRange", "IOVDb Run Range", 1, 0, 1, 1, 0, 1);
  TH2I* IOVDbTimeHist = new TH2I("IOVDbTimeRange", "IOVDb Time Range", 1, 0, 1, 1, 0, 1);
  TProfile* IOVDbBytesReadHist = new TProfile("IOVDbBytesRead", "Folder data read", 1, 0, 1);
  IOVDbBytesReadHist->SetYTitle("Data read [bytes]");
  TProfile* IOVDbReadTimeHist = new TProfile("IOVDbReadTime", "Folder update time", 1, 0, 1);
  IOVDbReadTimeHist->SetYTitle("Update time [ms]");

  // fill histograms
  IIOVDbSvc::KeyInfo info;
  for (const std::string& key : keyList) {

    if (m_IOVDbSvc->getKeyInfo(key, info) && info.retrieved) {

      m_currentIOVs[key] = info.range;

      IOVTime start(info.range.start());
      IOVTime stop(info.range.stop());

      // fill start condition (run number)
      if (start.isRunEvent()) {
        IOVDbRunHist->Fill(std::to_string(start.run()).c_str(), info.folderName.c_str(), 1.0);
      }

      // fill start condition (timestamp)
      if (start.isTimestamp()) {
        IOVDbTimeHist->Fill(std::to_string(start.timestamp()).c_str(), info.folderName.c_str(), 1.0);
      }

      // fill stop condition (run number)
      if (stop.isRunEvent()) {
        if (stop.run() == IOVTime::MAXRUN) {
          IOVDbRunHist->Fill("infinity", info.folderName.c_str(), 1.0);
        }
        else {
          IOVDbRunHist->Fill(std::to_string(stop.run()).c_str(), info.folderName.c_str(), 1.0);
        }
      }

      // fill stop condition (timestamp)
      if (stop.isTimestamp()) {
        if (stop.timestamp() == IOVTime::MAXTIMESTAMP) {
          IOVDbTimeHist->Fill("infinity", info.folderName.c_str(), 1.0);
        }
        else {
          IOVDbTimeHist->Fill(std::to_string(stop.timestamp()).c_str(), info.folderName.c_str(), 1.0);
        }
      }

      // fill read bytes and time read
      IOVDbBytesReadHist->Fill(info.folderName.c_str(), info.bytesRead, 1.0);
      IOVDbReadTimeHist->Fill(info.folderName.c_str(), info.readTime * 1000, 1.0);
    }
  }

  // final configuration of histograms
  IOVDbRunHist->LabelsDeflate("X");
  IOVDbRunHist->LabelsDeflate("Y");
  IOVDbRunHist->GetYaxis()->LabelsOption("a");

  IOVDbTimeHist->LabelsDeflate("X");
  IOVDbTimeHist->LabelsDeflate("Y");
  IOVDbTimeHist->GetYaxis()->LabelsOption("a");

  IOVDbBytesReadHist->LabelsDeflate("X");
  IOVDbReadTimeHist->LabelsDeflate("X");

  // Register histograms
  TH1* hist[] = {IOVDbRunHist, IOVDbTimeHist, IOVDbBytesReadHist, IOVDbReadTimeHist};
  for (TH1* h : hist) {
    m_histSvc->regHist(m_histPath + h->GetName(), h).ignore();
  }
}

void TrigOpMonitor::fillIOVDbChangeHist(const EventContext& ctx)
{
  std::vector<std::string> keys;
  if (m_IOVDbSvc) keys = m_IOVDbSvc->getKeyList();

  // Loop over all keys known to IOVDbSvc
  IIOVDbSvc::KeyInfo info;
  for (const std::string& k : keys) {
    if (not m_IOVDbSvc->getKeyInfo(k, info)) continue;
    if (not info.retrieved) continue;

    const auto curIOV = m_currentIOVs.find(k);
    if (curIOV == m_currentIOVs.end()) {
      m_currentIOVs[k] = info.range;
      continue;
    }

    // Print IOV changes and fill histogram
    if (info.range != curIOV->second) {
      ATH_MSG_INFO("IOV of " << k << " changed from " << curIOV->second << " to " << info.range
                             << " on event: " << ctx.eventID());

      if (m_iovChangeHist) {
        // Perform a locked fill and remove any empty bins to allow correct gathering
        oh_scoped_lock_histogram lock;
        m_iovChangeHist->Fill(std::to_string(ctx.eventID().lumi_block()).c_str(), info.folderName.c_str(),
                              1);
        m_iovChangeHist->LabelsDeflate("X");
        m_iovChangeHist->LabelsDeflate("Y");
      }

      // Create detailed histograms per changed folder
      if (m_detailedHists) {
        auto fh = m_folderHist.find(info.folderName);
        if (fh == m_folderHist.end()) {
          std::string name(folder2HistName(info.folderName));

          fh = m_folderHist.insert({info.folderName, FolderHist()}).first;
          fh->second.h_time = new TH1F((name + "_ReadTime").c_str(),
                                       ("Update time for " + info.folderName + ";Time [ms];Entries").c_str(),
                                       100, 0, 200);

          fh->second.h_bytes = new TH1F(
              (name + "_BytesRead").c_str(),
              ("Bytes read for " + info.folderName + ";Data [bytes];Entries").c_str(), 100, 0, 1000);

          for (TH1* h : {fh->second.h_time, fh->second.h_bytes}) {
            m_histSvc->regHist(m_histPath + h->GetName(), h).ignore();
          }
        }

        // Need to plot the difference because IOVDbSvc reports total time and bytes for entire job
        fh->second.h_time->Fill(info.readTime * 1000 - m_folderHist[info.folderName].total_time);
        fh->second.total_time += info.readTime * 1000;

        fh->second.h_bytes->Fill(info.bytesRead - m_folderHist[info.folderName].total_bytes);
        fh->second.total_bytes += info.bytesRead;
      }

      m_currentIOVs[k] = info.range;
    }
  }
}

void TrigOpMonitor::fillLumiHist(const EventContext& ctx)
{
  if (!m_lumiDataKey.empty()) {
    SG::ReadCondHandle<LuminosityCondData> lumiData(m_lumiDataKey, ctx);
    m_lumiHist->Fill(ctx.eventID().lumi_block(), lumiData->lbAverageLuminosity());
    m_muHist->Fill(ctx.eventID().lumi_block(), lumiData->lbAverageInteractionsPerCrossing());
  }
}

void TrigOpMonitor::fillReleaseDataHist()
{
  const char* ld_lib_path = getenv("LD_LIBRARY_PATH");
  if (ld_lib_path == nullptr) {
    ATH_MSG_WARNING("LD_LIBRARY_PATH is not defined. Will not fill release histogram.");
    return;
  }

  // Find all release metadata files
  std::list<DirSearchPath::path> file_list = DirSearchPath(ld_lib_path, ":").find_all(m_releaseData.value());

  if (file_list.empty()) {
    ATH_MSG_WARNING("Could not find release metadata file " << m_releaseData
                                                            << " in LD_LIBRARY_PATH");
    m_releaseHist->Fill("Release ?", 1);
    return;
  }

  for (const auto& f : file_list) {
    // Read metadata file
    std::map<std::string, std::string> result;
    if (readReleaseData(f.string(), result).isFailure()) {
      ATH_MSG_WARNING("Could not read release metadata from " << f);
      m_releaseHist->Fill("Release ?", 1);
      return;
    }

    // Check format
    if (result.find("project name") == result.end() || result.find("release") == result.end()) {
      ATH_MSG_WARNING("Invalid release metadata format in " << f);
      m_releaseHist->Fill("Release ?", 1);
      return;
    }

    m_releaseHist->Fill((result["project name"] + " " + result["release"]).c_str(), 1);
  }
}

void TrigOpMonitor::fillProcThreadHist(const int workerID)
{
  m_workersHist->Fill(workerID, 1);

  auto getIntProp = [this](std::string_view name, const std::string& prop) -> std::optional<int> {
    IProperty* svc = serviceLocator()->service(name).as<IProperty>();
    if (svc == nullptr) return std::nullopt;
    try {
      return std::stoi(svc->getProperty(prop).toString());
    }
    catch (...) {
      return std::nullopt;
    }
  };

  const std::optional<int> numThreads = getIntProp("AvalancheSchedulerSvc","ThreadPoolSize");
  if (numThreads.has_value()) {
    m_mtConfigHist->Fill("N threads", numThreads.value(), 1.0);
  }
  else {
    ATH_MSG_WARNING("Could not retrieve the number of threads to fill the monitoring histogram");
  }

  const std::optional<int> numSlots = getIntProp("EventDataSvc","NSlots");
  if (numSlots.has_value()) {
    m_mtConfigHist->Fill("N slots", numSlots.value(), 1.0);
  }
  else {
    ATH_MSG_WARNING("Could not retrieve the number of slots to fill the monitoring histogram");
  }
}

void TrigOpMonitor::fillSubDetHist()
{
  // Retrieve the enabled ROBs/SubDets list from DataFlowConfig which is a special object
  // used online to hold DF properties passed from TDAQ to HLT as run parameters
  auto jobOptionsSvc = service<Gaudi::Interfaces::IOptionsSvc>("JobOptionsSvc", /*createIf=*/ false);
  if (!jobOptionsSvc.isValid()) {
    ATH_MSG_WARNING("Could not retrieve JobOptionsSvc, will not fill SubDetectors histogram");
    return;
  }

  Gaudi::Property<std::vector<uint32_t>> enabledROBsProp("EnabledROBs",{});
  std::set<uint32_t> enabledROBs;
  if (enabledROBsProp.fromString(jobOptionsSvc->get("DataFlowConfig.DF_Enabled_ROB_IDs","[]")).isSuccess()) {
    enabledROBs.insert(enabledROBsProp.value().begin(), enabledROBsProp.value().end());
    ATH_MSG_DEBUG("Retrieved a list of " << enabledROBs.size()
                  << " ROBs from DataFlowConfig.DF_Enabled_ROB_IDs");
  }
  else {
    ATH_MSG_ERROR("Could not parse DataFlowConfig.DF_Enabled_ROB_IDs from JobOptionsSvc");
  }

  // Retrieve detector mask from detector store
  SmartIF<StoreGateSvc> inputMetaDataStore = service<StoreGateSvc>("InputMetaDataStore", /*createIf=*/ false);
  if (!inputMetaDataStore.isValid()) {
    ATH_MSG_WARNING("Could not retrieve InputMetaDataStore, will not fill SubDetectors histogram");
    return;
  }
  const ByteStreamMetadataContainer* metadatacont{nullptr};
  if (inputMetaDataStore->retrieve(metadatacont, "ByteStreamMetadata").isFailure()) {
    ATH_MSG_WARNING("Could not retrieve ByteStreamMetadata, will not fill SubDetectors histogram");
    return;
  }

  const ByteStreamMetadata* metadata = *(metadatacont->begin());
  uint64_t detMaskLeast = metadata->getDetectorMask();
  uint64_t detMaskMost = metadata->getDetectorMask2();

  // Decode subdetector masks
  std::vector<eformat::SubDetector> subDetOn;
  std::vector<eformat::SubDetector> subDetOff;
  std::vector<eformat::SubDetector> subDetAll;
  eformat::helper::DetectorMask(detMaskLeast, detMaskMost).sub_detectors(subDetOn);
  eformat::helper::DetectorMask(~detMaskLeast, ~detMaskMost).sub_detectors(subDetOff);
  eformat::helper::DetectorMask(~std::bitset<128>()).sub_detectors(subDetAll);

  // Add bins with labels for every subdetector name
  for (const eformat::SubDetector sd : subDetAll) {
    m_subdetHist->GetXaxis()->FindBin(eformat::helper::SubDetectorDictionary.string(sd).data());
  }
  m_subdetHist->LabelsDeflate("X");

  // Fill histogram with enabled subdetectors
  for (const eformat::SubDetector sd : subDetOn) {
    m_subdetHist->Fill(eformat::helper::SubDetectorDictionary.string(sd).data(), "on", 1.0);
  }

  // Fill histogram with disabled subdetectors
  for (const eformat::SubDetector sd : subDetOff) {
    m_subdetHist->Fill(eformat::helper::SubDetectorDictionary.string(sd).data(), "off", 1.0);
  }

  // Fill histogram with ROB counts
  for (const uint32_t robid : enabledROBs) {
    const std::string sdname = eformat::helper::SourceIdentifier(robid).human_detector();
    m_subdetHist->Fill(sdname.data(), "# ROB", 1.0);
  }
}

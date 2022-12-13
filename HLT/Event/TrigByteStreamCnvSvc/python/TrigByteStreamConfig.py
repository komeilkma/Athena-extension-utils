#
# Copyright (C) 2022 Komeilkma/leandrogardi for the benefit of the ATLAS collaboration
#

from AthenaConfiguration.ComponentAccumulator import ComponentAccumulator
from AthenaConfiguration.ComponentFactory import CompFactory
from AthenaMonitoringKernel.GenericMonitoringTool import GenericMonitoringTool
from SGComps.SGInputLoaderConfig import SGInputLoaderCfg

def getTrigByteStreamInputSvc(name='ByteStreamInputSvc'):
    svc = CompFactory.TrigByteStreamInputSvc(name)

    # Enable checking the CTP ROB with module ID 1 (the RoIB ROB), ATR-25217
    svc.CheckCTPFragmentModuleID = 1  # ROB ID 0x770001

    svc.MonTool = GenericMonitoringTool('MonTool', HistPath='HLTFramework/'+name)
    svc.MonTool.defineHistogram('L1Result_NumROBs', path='EXPERT', type='TH1F',
                                title='Number of ROBs received in L1 result;Number of ROBs;Events',
                                xbins=100, xmin=0, xmax=100)
    svc.MonTool.defineHistogram('L1Result_FullEvFragSize', path='EXPERT', type='TH1F',
                                title='Size of L1 result FullEventFragment;Size [kB];Events',
                                xbins=100, xmin=0, xmax=100)
    svc.MonTool.defineHistogram('L1Result_SubDets', path='EXPERT', type='TH1F',
                                title='Source of ROBs in L1 result passed to HLT;;Total number of ROBs',
                                xbins=4, xmin=0, xmax=4)
    svc.MonTool.defineHistogram('TIME_getNext', path='EXPERT', type='TH1F',
                                title='Time of DataCollector::getNext() calls;Time [ms];Calls',
                                xbins=400, xmin=0, xmax=200)
    svc.MonTool.defineHistogram('TIME_getNext;TIME_getNext_extRange', path='EXPERT', type='TH1F',
                                title='Time of DataCollector::getNext() calls;Time [ms];Calls',
                                xbins=400, xmin=0, xmax=2000, opt='kCanRebin')
    svc.MonTool.defineHistogram('getNext_LBN,getNext_noEvent;NoEventFraction', path='EXPERT', type='TProfile',
                                title='Fraction of getNext calls returning NO_EVENT;Lumi Block;Event fraction',
                                xbins=100, xmin=0, xmax=100, opt='kCanRebin')
    return svc

def getTrigByteStreamCnvSvc(name='ByteStreamCnvSvc'):
    svc = CompFactory.TrigByteStreamCnvSvc(name)
    svc.MonTool = GenericMonitoringTool('MonTool', HistPath='HLTFramework/'+name)
    svc.MonTool.defineHistogram('TIME_eventDone', path='EXPERT', type='TH1F',
                                title='Time of DataCollector::eventDone() calls;Time [ms];Calls',
                                xbins=400, xmin=0, xmax=2)
    svc.MonTool.defineHistogram('TIME_eventDone;TIME_eventDone_extRange', path='EXPERT', type='TH1F',
                                title='Time of DataCollector::eventDone() calls;Time [ms];Calls',
                                xbins=400, xmin=0, xmax=200, opt='kCanRebin')
    svc.MonTool.defineHistogram('StreamTags', path='EXPERT', type='TH1F',
                                title='Stream Tags produced by HLT;;Events',
                                xbins=10, xmin=0, xmax=10)
    svc.MonTool.defineHistogram('StreamTagCorrA,StreamTagCorrB;StreamTagsCorr', path='EXPERT', type='TH2F',
                                title='Stream Tags (produced by HLT) correlation',
                                xbins=10, xmin=0, xmax=10, ybins=10, ymin=0, ymax=10)
    svc.MonTool.defineHistogram('StreamTagsNum', path='EXPERT', type='TH1F',
                                title='Number of Stream Tags produced by HLT;Number of Stream Tags;Events',
                                xbins=20, xmin=0, xmax=20)
    svc.MonTool.defineHistogram('StreamTagsType', path='EXPERT', type='TH1F',
                                title='Type of Stream Tags produced by HLT;;Events',
                                xbins=7, xmin=0, xmax=7,
                                xlabels=["physics", "calibration", "express", "monitoring", "debug", "reserved", "unknown"])
    svc.MonTool.defineHistogram('StreamTagsPebRobsNum', path='EXPERT', type='TH1F',
                                title='Number of ROBs in PEB stream tags;Number of ROBs;Entries',
                                xbins=200, xmin=0, xmax=200,
                                cutmask="StreamTagIsPeb")
    svc.MonTool.defineHistogram('StreamTagsPebSubDetsNum', path='EXPERT', type='TH1F',
                                title='Number of SubDetectors in PEB stream tags;Number of SubDetectors;Entries',
                                xbins=100, xmin=0, xmax=100,
                                cutmask="StreamTagIsPeb")
    svc.MonTool.defineHistogram('StreamTagsPebSubDetsFromRobList', path='EXPERT', type='TH1F',
                                title='SubDetectors in PEB stream tags ROB list;;Entries',
                                xbins=20, xmin=0, xmax=20)
    svc.MonTool.defineHistogram('StreamTagsPebSubDetsFromSubDetList', path='EXPERT', type='TH1F',
                                title='SubDetectors in PEB stream tags SubDetector list;;Entries',
                                xbins=20, xmin=0, xmax=20)
    svc.MonTool.defineHistogram('ResultSizeTotal', path='EXPERT', type='TH1F',
                                title='HLT result total size (sum of all modules);Size [kB];Events',
                                xbins=200, xmin=0, xmax=2000, opt='kCanRebin')
    svc.MonTool.defineHistogram('ResultSizeFullEvFrag', path='EXPERT', type='TH1F',
                                title='HLT output FullEventFragment size;Size [kB];Events',
                                xbins=200, xmin=0, xmax=2000, opt='kCanRebin')
    svc.MonTool.defineHistogram('ResultModuleID,ResultModuleSize;ResultSizeByModule', path='EXPERT', type='TH2F',
                                title='HLT result size by module;Module ID;Size [kB]',
                                xbins=10, xmin=0, xmax=10, ybins=400, ymin=0, ymax=4000)
    svc.MonTool.defineHistogram('StreamTags,ResultSizeStream;ResultSizeByStream', path='EXPERT', type='TH2F',
                                title='HLT result size by stream;;Size [kB]',
                                xbins=10, xmin=0, xmax=10, ybins=400, ymin=0, ymax=4000)
    svc.MonTool.defineHistogram('OnlineErrorCode', path='EXPERT', type='TH1F',
                                title='Online error codes;;Events',
                                xbins=1, xmin=0, xmax=1)
    svc.MonTool.defineHistogram('TIME_monitorRawEvent', path='EXPERT', type='TH1F',
                                title='Time of monitorRawEvent() calls;Time [us];Calls',
                                xbins=1000, xmin=0, xmax=1000)
    return svc

def TrigByteStreamCfg(flags, type_names=[]):
    acc = ComponentAccumulator()

    bytestream_conversion = getTrigByteStreamCnvSvc()
    acc.addService(bytestream_conversion, primary=True)

    bytestream_input = getTrigByteStreamInputSvc()
    acc.addService(bytestream_input)

    event_selector = CompFactory.TrigEventSelectorByteStream(
        name='EventSelectorByteStream',
        ByteStreamInputSvc=bytestream_input.name)
    acc.addService(event_selector)
    acc.setAppProperty("EvtSel", event_selector.name)

    event_persistency = CompFactory.EvtPersistencySvc(
        name="EventPersistencySvc",
        CnvServices=[bytestream_conversion.name])
    acc.addService(event_persistency)

    address_provider = CompFactory.ByteStreamAddressProviderSvc(
        TypeNames=type_names)
    acc.addService(address_provider)

    proxy = CompFactory.ProxyProviderSvc()
    proxy.ProviderNames += [address_provider.name]
    acc.addService(proxy)

    acc.merge(SGInputLoaderCfg(flags, Load=address_provider.TypeNames, FailIfNoProxy=True))

    return acc

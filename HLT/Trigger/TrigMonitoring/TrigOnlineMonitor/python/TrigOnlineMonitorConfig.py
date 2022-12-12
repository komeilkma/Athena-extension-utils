# Copyright (C) 2022 Komeilkma for the benefit of the ATLAS collaboration

from TrigOnlineMonitor.TrigOnlineMonitorConf import TrigOpMonitor as _TrigOpMonitor
from AthenaCommon.AlgSequence import AthSequencer

class TrigOpMonitor(_TrigOpMonitor):
   __slots__ = ()

   def __init__(self, name='TrigOpMonitor', **kwargs):
      super(TrigOpMonitor, self).__init__(name, **kwargs)

      # Only monitor lumi and field if available
      condSeq = AthSequencer('AthCondSeq')
      for a in condSeq:
         if a.getType()=='LuminosityCondAlg':
            self.LuminosityCondDataKey = a.LuminosityOutputKey
         elif a.getType()=='MagField::AtlasFieldMapCondAlg':
            self.AtlasFieldMapCondDataKey = a.AtlasFieldMapCondObj


def TrigALFAROBMonitor():
   from AthenaConfiguration.ComponentFactory import CompFactory
   from AthenaMonitoringKernel.GenericMonitoringTool import GenericMonitoringTool

   monToolTrackingElast = GenericMonitoringTool('MonTool_trackingElast', HistPath='ALFAROBMonitor'+'/MTtracking/elast/current')
   monToolTrackingAny = GenericMonitoringTool('MonTool_trackingAny', HistPath='ALFAROBMonitor'+'/MTtracking/any/current')
   monToolTrackingElast_1LB = GenericMonitoringTool('MonTool_trackingElast_1LB', HistPath='ALFAROBMonitor'+'/MTtracking/elast/1LB')
   monToolTrackingAny_1LB = GenericMonitoringTool('MonTool_trackingAny_1LB', HistPath='ALFAROBMonitor'+'/MTtracking/any/1LB')
   monToolTrackingElast_10LB = GenericMonitoringTool('MonTool_trackingElast_10LB', HistPath='ALFAROBMonitor'+'/MTtracking/elast/10LB')
   monToolTrackingAny_10LB = GenericMonitoringTool('MonTool_trackingAny_10LB', HistPath='ALFAROBMonitor'+'/MTtracking/any/10LB')
   monToolTrackingElast_60LB = GenericMonitoringTool('MonTool_trackingElast_60LB', HistPath='ALFAROBMonitor'+'/MTtracking/elast/60LB')
   monToolTrackingAny_60LB = GenericMonitoringTool('MonTool_trackingAny_60LB', HistPath='ALFAROBMonitor'+'/MTtracking/any/60LB')
   monToolTrackingElast_SB = GenericMonitoringTool('MonTool_trackingElast_SB', HistPath='ALFAROBMonitor'+'/MTtracking/elast/SB')
   monToolTrackingAny_SB = GenericMonitoringTool('MonTool_trackingAny_SB', HistPath='ALFAROBMonitor'+'/MTtracking/any/SB')

   monToolCommon = GenericMonitoringTool('MonTool_common', HistPath='ALFAROBMonitor'+'/MTcommon')

   monToolOD_B7L1U = GenericMonitoringTool('MonTool_OD_B7L1U', HistPath='ALFAROBMonitor'+'/MTOD/B7L1U')
   monToolOD_B7L1L = GenericMonitoringTool('MonTool_OD_B7L1L', HistPath='ALFAROBMonitor'+'/MTOD/B7L1L')
   monToolOD_A7L1U = GenericMonitoringTool('MonTool_OD_A7L1U', HistPath='ALFAROBMonitor'+'/MTOD/A7L1U')
   monToolOD_A7L1L = GenericMonitoringTool('MonTool_OD_A7L1L', HistPath='ALFAROBMonitor'+'/MTOD/A7L1L')
   monToolOD_A7R1U = GenericMonitoringTool('MonTool_OD_A7R1U', HistPath='ALFAROBMonitor'+'/MTOD/A7R1U')
   monToolOD_A7R1L = GenericMonitoringTool('MonTool_OD_A7R1L', HistPath='ALFAROBMonitor'+'/MTOD/A7R1L')
   monToolOD_B7R1U = GenericMonitoringTool('MonTool_OD_B7R1U', HistPath='ALFAROBMonitor'+'/MTOD/B7R1U')
   monToolOD_B7R1L = GenericMonitoringTool('MonTool_OD_B7R1L', HistPath='ALFAROBMonitor'+'/MTOD/B7R1L')

   monToolDetectors = GenericMonitoringTool('MonTool_detectors', HistPath='ALFAROBMonitor'+'/MTdetectors')

   monToolBackgroundElast15 = GenericMonitoringTool('MonTool_backgroundElast15', HistPath='ALFAROBMonitor'+'/MTbackground/elast15/current')
   monToolBackgroundElast18 = GenericMonitoringTool('MonTool_backgroundElast18', HistPath='ALFAROBMonitor'+'/MTbackground/elast18/current')
   monToolBackgroundSyst17 = GenericMonitoringTool('MonTool_backgroundSyst17', HistPath='ALFAROBMonitor'+'/MTbackground/syst17/current')
   monToolBackgroundSyst18 = GenericMonitoringTool('MonTool_backgroundSyst18', HistPath='ALFAROBMonitor'+'/MTbackground/syst18/current')

   monToolBackgroundElast15_1LB = GenericMonitoringTool('MonTool_backgroundElast15_1LB', HistPath='ALFAROBMonitor'+'/MTbackground/elast15/1LB')
   monToolBackgroundElast18_1LB = GenericMonitoringTool('MonTool_backgroundElast18_1LB', HistPath='ALFAROBMonitor'+'/MTbackground/elast18/1LB')
   monToolBackgroundSyst17_1LB = GenericMonitoringTool('MonTool_backgroundSyst17_1LB', HistPath='ALFAROBMonitor'+'/MTbackground/syst17/1LB')
   monToolBackgroundSyst18_1LB = GenericMonitoringTool('MonTool_backgroundSyst18_1LB', HistPath='ALFAROBMonitor'+'/MTbackground/syst18/1LB')

   monToolBackgroundElast15_10LB = GenericMonitoringTool('MonTool_backgroundElast15_10LB', HistPath='ALFAROBMonitor'+'/MTbackground/elast15/10LB')
   monToolBackgroundElast18_10LB = GenericMonitoringTool('MonTool_backgroundElast18_10LB', HistPath='ALFAROBMonitor'+'/MTbackground/elast18/10LB')
   monToolBackgroundSyst17_10LB = GenericMonitoringTool('MonTool_backgroundSyst17_10LB', HistPath='ALFAROBMonitor'+'/MTbackground/syst17/10LB')
   monToolBackgroundSyst18_10LB = GenericMonitoringTool('MonTool_backgroundSyst18_10LB', HistPath='ALFAROBMonitor'+'/MTbackground/syst18/10LB')

   monToolBackgroundElast15_60LB = GenericMonitoringTool('MonTool_backgroundElast15_60LB', HistPath='ALFAROBMonitor'+'/MTbackground/elast15/60LB')
   monToolBackgroundElast18_60LB = GenericMonitoringTool('MonTool_backgroundElast18_60LB', HistPath='ALFAROBMonitor'+'/MTbackground/elast18/60LB')
   monToolBackgroundSyst17_60LB = GenericMonitoringTool('MonTool_backgroundSyst17_60LB', HistPath='ALFAROBMonitor'+'/MTbackground/syst17/60LB')
   monToolBackgroundSyst18_60LB = GenericMonitoringTool('MonTool_backgroundSyst18_60LB', HistPath='ALFAROBMonitor'+'/MTbackground/syst18/60LB')

   # common
   monToolCommon.defineHistogram("com_LB,goodDataAssessmentLB15", type='TH2F', path='EXPERT', title='goodDataAssessmentLB15', xbins=1000, xmin=-0.5, xmax=999.5, ybins=2, ymin=0.5, ymax=2.5 )
   monToolCommon.defineHistogram("com_LB,goodDataAssessmentLB18", type='TH2F', path='EXPERT', title='goodDataAssessmentLB18', xbins=1000, xmin=-0.5, xmax=999.5, ybins=2, ymin=0.5, ymax=2.5 )
   monToolCommon.defineHistogram("com_goodDataAssessment",        type='TH1F', path='EXPERT', title='goodDataAssessment',     xbins=10,   xmin=-0.5, xmax=9.5 )

   stations = ['B7L1U', 'B7L1L', 'A7L1U', 'A7L1L', 'A7R1U', 'A7R1L', 'B7R1U', 'B7R1L']
   for station in stations:
      #detector
      monToolDetectors.defineHistogram(station + "_,PMF", type='TH2F', path='EXPERT', title=station + " activity;channels;PMF", xbins=64, xmin=0., xmax=64., ybins=23, ymin=1.,ymax=24.)

      if station.find("U") != -1:
         monToolTrackingElast.defineHistogram("elastic_" + station + ",y", type='TH2F', path='EXPERT', title=station + " f elastic;x;y",              xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.)
         monToolTrackingAny.defineHistogram("ANY_" + station + ",y", type='TH2F', path='EXPERT', title=station + " f ANY;x;y",                        xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.)
         monToolTrackingElast_SB.defineHistogram("elastic_" + station + "_SB,y", type='TH2F', path='EXPERT', title=station + " SB elastic ;x;y",      xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.)
         monToolTrackingAny_SB.defineHistogram("ANY_" + station + "_SB,y", type='TH2F', path='EXPERT', title=station + " SB ANY;x;y",                 xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.)
         monToolTrackingElast_1LB.defineHistogram("elastic_" + station + "_1LB,y", type='TH2F', path='EXPERT', title=station + " 1LB elastic ;x;y",   xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.,opt='kLBNHistoryDepth=1')
         monToolTrackingAny_1LB.defineHistogram("ANY_" + station + "_1LB,y", type='TH2F', path='EXPERT', title=station + " 1LB ANY;x;y",              xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.,opt='kLBNHistoryDepth=1')
         monToolTrackingElast_10LB.defineHistogram("elastic_" + station + "_10LB,y", type='TH2F', path='EXPERT', title=station + " 10LB elastic;x;y", xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.,opt='kLBNHistoryDepth=10')
         monToolTrackingAny_10LB.defineHistogram("ANY_" + station + "_10LB,y", type='TH2F', path='EXPERT', title=station + " 10LB ANY;x;y",           xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.,opt='kLBNHistoryDepth=10')
         monToolTrackingElast_60LB.defineHistogram("elastic_" + station + "_60LB,y", type='TH2F', path='EXPERT', title=station + " 60LB elastic;x;y", xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.,opt='kLBNHistoryDepth=60')
         monToolTrackingAny_60LB.defineHistogram("ANY_" + station + "_60LB,y", type='TH2F', path='EXPERT', title=station + " 60LB ANY;x;y",           xbins=260, xmin=-23., xmax=23., ybins=175, ymin=0.,ymax=35.,opt='kLBNHistoryDepth=60')
      else:
         monToolTrackingElast.defineHistogram     ("elastic_" + station + ",y", type='TH2F', path='EXPERT', title=station + " f elastic;x;y",         xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.)
         monToolTrackingAny.defineHistogram       ("ANY_" + station + ",y", type='TH2F', path='EXPERT', title=station + " f ANY;x;y",                 xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.)
         monToolTrackingElast_SB.defineHistogram  ("elastic_" + station + "_SB,y", type='TH2F', path='EXPERT', title=station + " SB elastic ;x;y",    xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.)
         monToolTrackingAny_SB.defineHistogram    ("ANY_" + station + "_SB,y", type='TH2F', path='EXPERT', title=station + " SB ANY;x;y",             xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.)
         monToolTrackingElast_1LB.defineHistogram ("elastic_" + station + "_1LB,y", type='TH2F', path='EXPERT', title=station + " 1LB elastic ;x;y",  xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.,opt='kLBNHistoryDepth=1')
         monToolTrackingAny_1LB.defineHistogram   ("ANY_" + station + "_1LB,y", type='TH2F', path='EXPERT', title=station + " 1LB ANY;x;y",           xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.,opt='kLBNHistoryDepth=1')
         monToolTrackingElast_10LB.defineHistogram("elastic_" + station + "_10LB,y", type='TH2F', path='EXPERT', title=station + " 10LB elastic;x;y", xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.,opt='kLBNHistoryDepth=10')
         monToolTrackingAny_10LB.defineHistogram  ("ANY_" + station + "_10LB,y", type='TH2F', path='EXPERT', title=station + " 10LB ANY;x;y",         xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.,opt='kLBNHistoryDepth=10')
         monToolTrackingElast_60LB.defineHistogram("elastic_" + station + "_60LB,y", type='TH2F', path='EXPERT', title=station + " 60LB elastic;x;y", xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.,opt='kLBNHistoryDepth=60')
         monToolTrackingAny_60LB.defineHistogram  ("ANY_" + station + "_60LB,y", type='TH2F', path='EXPERT', title=station + " 60LB ANY;x;y",         xbins=260, xmin=-23., xmax=23., ybins=175, ymin=-35.,ymax=0.,opt='kLBNHistoryDepth=60')

   monToolOD_B7L1U.defineHistogram("od_B7L1U_RP_1_0 position", type='TH1F', path='EXPERT', title='B7L1U RP_1_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7L1U.defineHistogram("od_B7L1U_RP_1_1 position", type='TH1F', path='EXPERT', title='B7L1U RP_1_1 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7L1U.defineHistogram("od_B7L1U_distance_1_2_side_0", type='TH1F', path='EXPERT', title='B7L1U distance_1_2_side_0', xbins=401, xmin=-20.05, xmax=20.05)
   monToolOD_B7L1U.defineHistogram("od_B7L1U_distance_1_2_side_1", type='TH1F', path='EXPERT', title='B7L1U distance_1_2_side_1', xbins=401, xmin=-20.05, xmax=20.05)

   monToolOD_B7L1L.defineHistogram("od_B7L1L_RP_2_0 position", type='TH1F', path='EXPERT', title='B7L1L RP_2_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7L1L.defineHistogram("od_B7L1L_RP_2_1 position", type='TH1F', path='EXPERT', title='B7L1L RP_2_1 position', xbins=200, xmin=-145., xmax=-125.)

   monToolOD_B7L1U.defineHistogram("od_B7L1U_RP_1_0 position", type='TH1F', path='EXPERT', title='B7L1U RP_1_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7L1U.defineHistogram("od_B7L1U_RP_1_1 position", type='TH1F', path='EXPERT', title='B7L1U RP_1_1 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7L1U.defineHistogram("od_B7L1U_distance_1_2_side_0", type='TH1F', path='EXPERT', title='B7L1U distance_1_2_side_0', xbins=401, xmin=-20.05, xmax=20.05)
   monToolOD_B7L1U.defineHistogram("od_B7L1U_distance_1_2_side_1", type='TH1F', path='EXPERT', title='B7L1U distance_1_2_side_1', xbins=401, xmin=-20.05, xmax=20.05)

   monToolOD_B7L1L.defineHistogram("od_B7L1L_RP_2_0 position", type='TH1F', path='EXPERT', title='B7L1L RP_2_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7L1L.defineHistogram("od_B7L1L_RP_2_1 position", type='TH1F', path='EXPERT', title='B7L1L RP_2_1 position', xbins=200, xmin=-145., xmax=-125.)

   monToolOD_A7L1U.defineHistogram("od_A7L1U_RP_3_0 position", type='TH1F', path='EXPERT', title='A7L1U RP_3_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_A7L1U.defineHistogram("od_A7L1U_RP_3_1 position", type='TH1F', path='EXPERT', title='A7L1U RP_3_1 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_A7L1U.defineHistogram("od_A7L1U_distance_3_4_side_0", type='TH1F', path='EXPERT', title='A7L1U distance_3_4_side_0', xbins=401, xmin=-20.05, xmax=20.05)
   monToolOD_A7L1U.defineHistogram("od_A7L1U_distance_3_4_side_1", type='TH1F', path='EXPERT', title='A7L1U distance_3_4_side_1', xbins=401, xmin=-20.05, xmax=20.05)

   monToolOD_A7L1L.defineHistogram("od_A7L1L_RP_4_0 position", type='TH1F', path='EXPERT', title='A7L1L RP_4_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_A7L1L.defineHistogram("od_A7L1L_RP_4_1 position", type='TH1F', path='EXPERT', title='A7L1L RP_4_1 position', xbins=200, xmin=-145., xmax=-125.)

   monToolOD_A7R1U.defineHistogram("od_A7R1U_RP_5_0 position", type='TH1F', path='EXPERT', title='A7R1U RP_5_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_A7R1U.defineHistogram("od_A7R1U_RP_5_1 position", type='TH1F', path='EXPERT', title='A7R1U RP_5_1 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_A7R1U.defineHistogram("od_A7R1U_distance_5_6_side_0", type='TH1F', path='EXPERT', title='A7R1U distance_5_6_side_0', xbins=401, xmin=-20.05, xmax=20.05)
   monToolOD_A7R1U.defineHistogram("od_A7R1U_distance_5_6_side_1", type='TH1F', path='EXPERT', title='A7R1U distance_5_6_side_1', xbins=401, xmin=-20.05, xmax=20.05)

   monToolOD_A7R1L.defineHistogram("od_A7R1L_RP_6_0 position", type='TH1F', path='EXPERT', title='A7R1L RP_6_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_A7R1L.defineHistogram("od_A7R1L_RP_6_1 position", type='TH1F', path='EXPERT', title='A7R1L RP_6_1 position', xbins=200, xmin=-145., xmax=-125.)

   monToolOD_B7R1U.defineHistogram("od_B7R1U_RP_7_0 position", type='TH1F', path='EXPERT', title='B7R1U RP_7_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7R1U.defineHistogram("od_B7R1U_RP_7_1 position", type='TH1F', path='EXPERT', title='B7R1U RP_7_1 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7R1U.defineHistogram("od_B7R1U_distance_7_8_side_0", type='TH1F', path='EXPERT', title='B7R1U distance_7_8_side_0', xbins=401, xmin=-20.05, xmax=20.05)
   monToolOD_B7R1U.defineHistogram("od_B7R1U_distance_7_8_side_1", type='TH1F', path='EXPERT', title='B7R1U distance_7_8_side_1', xbins=401, xmin=-20.05, xmax=20.05)

   monToolOD_B7R1L.defineHistogram("od_B7R1L_RP_8_0 position", type='TH1F', path='EXPERT', title='B7R1L RP_8_0 position', xbins=200, xmin=-145., xmax=-125.)
   monToolOD_B7R1L.defineHistogram("od_B7R1L_RP_8_1 position", type='TH1F', path='EXPERT', title='B7R1L RP_8_1 position', xbins=200, xmin=-145., xmax=-125.)

   # background monitoring hstograms
   bckg_armSets = ['L1U', 'L1D', 'R1U','R1D']
   bckg_triggers = ['elast15', 'elast18', 'syst17', 'syst18']
   xcxAxis = ["x_{3} [mm]", "x_{4} [mm]", "x_{6} [mm]", "x_{5} [mm]"]
   ycxAxis = ["x_{1} [mm]", "x_{2} [mm]", "x_{8} [mm]", "x_{7} [mm]"]
   xcyAxis = ["y_{3} [mm]", "y_{4} [mm]", "y_{6} [mm]", "y_{5} [mm]"]
   ycyAxis = ["y_{1} [mm]", "y_{2} [mm]", "y_{8} [mm]", "y_{7} [mm]"]
   xaxAxis = ["x_{3} [mm]", "x_{4} [mm]", "x_{5} [mm]", "x_{6} [mm]"]
   yaxAxis = ["#theta_{x} [#murad]", "#theta_{x} [#murad]", "#theta_{x} [#murad]", "#theta_{x} [#murad]"]
   yayAxis = ["#theta_{y} [#murad]", "#theta_{y} [#murad]", "#theta_{y} [#murad]", "#theta_{y} [#murad]"]

   for trigger in bckg_triggers:
      for station in bckg_armSets:
         if (trigger.find('elast15') != -1) and ((station.find('L1D') != -1) or (station.find('R1U') != -1)):  # golden 1: exclude bottom left top righ
            continue
         if (trigger.find('elast18') != -1) and ((station.find('L1U') != -1) or (station.find('R1D') != -1)):  # golden 2: exclude top left bottom right
            continue
         if (trigger.find('syst17') != -1) and ((station.find('L1D') != -1) or (station.find('R1D') != -1)):   # antigolden 1: exclude both lowe
            continue
         if (trigger.find('syst18') != -1) and ((station.find('L1U') != -1) or (station.find('R1U') != -1)):   # antigolden 2: exclude both upper
            continue

         if (trigger.find('elast15') != -1):
            monToolBckg_1LB = monToolBackgroundElast15_1LB
            monToolBckg_10LB = monToolBackgroundElast15_10LB
            monToolBckg_60LB = monToolBackgroundElast15_60LB
            monToolBckg = monToolBackgroundElast15
         if (trigger.find('elast18') != -1):
            monToolBckg_10LB = monToolBackgroundElast18_10LB
            monToolBckg_60LB = monToolBackgroundElast18_60LB
            monToolBckg = monToolBackgroundElast18
            monToolBckg_1LB = monToolBackgroundElast18_1LB
         if (trigger.find('syst17') != -1):
            monToolBckg_60LB = monToolBackgroundSyst17_60LB
            monToolBckg = monToolBackgroundSyst17
            monToolBckg_1LB = monToolBackgroundSyst17_1LB
            monToolBckg_10LB = monToolBackgroundSyst17_10LB
         if (trigger.find('syst18') != -1):
            monToolBckg_1LB = monToolBackgroundSyst18_1LB
            monToolBckg_10LB = monToolBackgroundSyst18_10LB
            monToolBckg_60LB = monToolBackgroundSyst18_60LB
            monToolBckg = monToolBackgroundSyst18

         monToolBckg.defineHistogram(trigger+'_x_'+station+',y', type='TH2F', path='EXPERT', title=trigger+'_x_'+station+';'+xcxAxis[bckg_armSets.index(station)]+';'+ycxAxis[bckg_armSets.index(station)], xbins=200, xmin=-12., xmax=12., ybins=200, ymin=-12., ymax=12.)
         monToolBckg.defineHistogram(trigger+'_y_'+station+',y', type='TH2F', path='EXPERT', title=trigger+'_y_'+station+';'+xcyAxis[bckg_armSets.index(station)]+';'+ycyAxis[bckg_armSets.index(station)], xbins=200, xmin=-25., xmax=25., ybins=200, ymin=-25., ymax=25.)
         monToolBckg.defineHistogram(trigger+'_ax_'+station+',y', type='TH2F', path='EXPERT', title=trigger+'_ax_'+station+';'+xaxAxis[bckg_armSets.index(station)]+';'+yaxAxis[bckg_armSets.index(station)], xbins=200, xmin=-12., xmax=12., ybins=200, ymin=-1500., ymax=1500.)
         monToolBckg.defineHistogram(trigger+'_ay_'+station+',y', type='TH2F', path='EXPERT', title=trigger+'_ay_'+station+';'+xcyAxis[bckg_armSets.index(station)]+';'+yayAxis[bckg_armSets.index(station)], xbins=200, xmin=-25., xmax=25., ybins=200, ymin=-2000., ymax=2000.)

         monToolBckg_1LB.defineHistogram(trigger+'_x_'+station+'_1LB,y', type='TH2F', path='EXPERT', title=trigger+'_x_'+station+'_1LB;'+xcxAxis[bckg_armSets.index(station)]+';'+ycxAxis[bckg_armSets.index(station)], xbins=200, xmin=-12., xmax=12., ybins=200, ymin=-12., ymax=12. ,opt='kLBNHistoryDepth=1')
         monToolBckg_1LB.defineHistogram(trigger+'_y_'+station+'_1LB,y', type='TH2F', path='EXPERT', title=trigger+'_y_'+station+'_1LB;'+xcyAxis[bckg_armSets.index(station)]+';'+ycyAxis[bckg_armSets.index(station)], xbins=200, xmin=-25., xmax=25., ybins=200, ymin=-25., ymax=25. ,opt='kLBNHistoryDepth=1')
         monToolBckg_1LB.defineHistogram(trigger+'_ax_'+station+'_1LB,y', type='TH2F', path='EXPERT', title=trigger+'_ax_'+station+'_1LB;'+xaxAxis[bckg_armSets.index(station)]+';'+yaxAxis[bckg_armSets.index(station)], xbins=200, xmin=-12., xmax=12., ybins=200, ymin=-1500., ymax=1500. ,opt='kLBNHistoryDepth=1')
         monToolBckg_1LB.defineHistogram(trigger+'_ay_'+station+'_1LB,y', type='TH2F', path='EXPERT', title=trigger+'_ay_'+station+'_1LB;'+xcyAxis[bckg_armSets.index(station)]+';'+yayAxis[bckg_armSets.index(station)], xbins=200, xmin=-25., xmax=25., ybins=200, ymin=-2000., ymax=2000. ,opt='kLBNHistoryDepth=1')

         monToolBckg_10LB.defineHistogram(trigger+'_x_'+station+'_10LB,y', type='TH2F', path='EXPERT', title=trigger+'_x_'+station+'_10LB;'+xcxAxis[bckg_armSets.index(station)]+';'+ycxAxis[bckg_armSets.index(station)], xbins=200, xmin=-12., xmax=12., ybins=200, ymin=-12., ymax=12. ,opt='kLBNHistoryDepth=10') 
         monToolBckg_10LB.defineHistogram(trigger+'_y_'+station+'_10LB,y', type='TH2F', path='EXPERT', title=trigger+'_y_'+station+'_10LB;'+xcyAxis[bckg_armSets.index(station)]+';'+ycyAxis[bckg_armSets.index(station)], xbins=200, xmin=-25., xmax=25., ybins=200, ymin=-25., ymax=25. ,opt='kLBNHistoryDepth=10') 
         monToolBckg_10LB.defineHistogram(trigger+'_ax_'+station+'_10LB,y', type='TH2F', path='EXPERT', title=trigger+'_ax_'+station+'_10LB;'+xaxAxis[bckg_armSets.index(station)]+';'+yaxAxis[bckg_armSets.index(station)], xbins=200, xmin=-12., xmax=12., ybins=200, ymin=-1500., ymax=1500. ,opt='kLBNHistoryDepth=10')
         monToolBckg_10LB.defineHistogram(trigger+'_ay_'+station+'_10LB,y', type='TH2F', path='EXPERT', title=trigger+'_ay_'+station+'_10LB;'+xcyAxis[bckg_armSets.index(station)]+';'+yayAxis[bckg_armSets.index(station)], xbins=200, xmin=-25., xmax=25., ybins=200, ymin=-2000., ymax=2000. ,opt='kLBNHistoryDepth=10')

         monToolBckg_60LB.defineHistogram(trigger+'_x_'+station+'_60LB,y', type='TH2F', path='EXPERT', title=trigger+'_x_'+station+'_60LB;'+xcxAxis[bckg_armSets.index(station)]+';'+ycxAxis[bckg_armSets.index(station)], xbins=200, xmin=-12., xmax=12., ybins=200, ymin=-12., ymax=12. ,opt='kLBNHistoryDepth=60')
         monToolBckg_60LB.defineHistogram(trigger+'_y_'+station+'_60LB,y', type='TH2F', path='EXPERT', title=trigger+'_y_'+station+'_60LB;'+xcyAxis[bckg_armSets.index(station)]+';'+ycyAxis[bckg_armSets.index(station)], xbins=200, xmin=-25., xmax=25., ybins=200, ymin=-25., ymax=25. ,opt='kLBNHistoryDepth=60')
         monToolBckg_60LB.defineHistogram(trigger+'_ax_'+station+'_60LB,y', type='TH2F', path='EXPERT', title=trigger+'_ax_'+station+'_60LB;'+xaxAxis[bckg_armSets.index(station)]+';'+yaxAxis[bckg_armSets.index(station)], xbins=200, xmin=-12., xmax=12., ybins=200, ymin=-1500., ymax=1500. ,opt='kLBNHistoryDepth=60')
         monToolBckg_60LB.defineHistogram(trigger+'_ay_'+station+'_60LB,y', type='TH2F', path='EXPERT', title=trigger+'_ay_'+station+'_60LB;'+xcyAxis[bckg_armSets.index(station)]+';'+yayAxis[bckg_armSets.index(station)], xbins=200, xmin=-25., xmax=25., ybins=200, ymin=-2000., ymax=2000. ,opt='kLBNHistoryDepth=60')

   ALFAROBMonitor = CompFactory.TrigALFAROBMonitor("ALFAROBMonitor")
   ALFAROBMonitor.MonTools += [monToolCommon, monToolDetectors]
   ALFAROBMonitor.MonTools += [monToolBackgroundElast15,      monToolBackgroundElast18,      monToolBackgroundSyst17,      monToolBackgroundSyst18]
   ALFAROBMonitor.MonTools += [monToolBackgroundElast15_1LB,  monToolBackgroundElast18_1LB,  monToolBackgroundSyst17_1LB,  monToolBackgroundSyst18_1LB]
   ALFAROBMonitor.MonTools += [monToolBackgroundElast15_10LB, monToolBackgroundElast18_10LB, monToolBackgroundSyst17_10LB, monToolBackgroundSyst18_10LB]
   ALFAROBMonitor.MonTools += [monToolBackgroundElast15_60LB, monToolBackgroundElast18_60LB, monToolBackgroundSyst17_60LB, monToolBackgroundSyst18_60LB]
   ALFAROBMonitor.MonTools += [monToolTrackingElast, monToolTrackingAny, monToolTrackingElast_1LB, monToolTrackingAny_1LB]
   ALFAROBMonitor.MonTools += [monToolTrackingElast_10LB, monToolTrackingAny_10LB, monToolTrackingElast_60LB, monToolTrackingAny_60LB]
   ALFAROBMonitor.MonTools += [monToolOD_B7L1U, monToolOD_B7L1L, monToolOD_A7L1U, monToolOD_A7L1L, monToolOD_A7R1U, monToolOD_A7R1L, monToolOD_B7R1U, monToolOD_B7R1L]
   return ALFAROBMonitor
   

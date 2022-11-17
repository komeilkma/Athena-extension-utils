#!/usr/bin/env python
"""
Ratio Plot
==========

This module plots a set of histograms and their ratio using the ATLAS Style.

If branch names don't match between files, use TTree:SetAlias
See: https:#root.cern.ch/doc/master/classTTree.html

.. literalinclude:: ../examples/ratio_plot.py
    :lines: 15-
"""

from __future__ import absolute_import, division, print_function

import os
import argparse

import ROOT as root
from ROOT import gPad

from atlasplots import atlas_style as astyle
from atlasplots import utils
from atlasplots import config_reader as config


def make_outdir(params):
    """Make the output directory.

    Optionally place output directory in a parent directory indicating a version
    number if provided.

    Parameters
    ----------
    params : dict
        Dictionary of configuration parameters

    Returns
    -------
    str
        Output directory
    """
    if 'version' not in params:
        outdir = params['outdir']
        if not os.path.exists(outdir):
            os.makedirs(outdir)
    else:
        outdir = "{}/{}".format(params['outdir'], params['version'])
        if not os.path.exists(outdir):
            os.makedirs(outdir)

    return outdir


def FormatRatioPlot(h, ymin, ymax, yLabel):
    # Define range of y axis
    h.SetMinimum(ymin)
    h.SetMaximum(ymax)

    # Set marker style and draw
    h.SetMarkerStyle(20)
    h.Draw("same e")

    # Y axis ratio plot settings
    h.GetYaxis().SetTitle(yLabel)
    h.GetYaxis().SetTitleSize(26)
    h.GetYaxis().SetTitleFont(43)
    h.GetYaxis().SetTitleOffset(1.5)
    h.GetYaxis().SetLabelFont(43)
    h.GetYaxis().SetLabelSize(26)
    h.GetYaxis().SetNdivisions(505)

    # X axis ratio plot settings
    h.GetXaxis().SetTitleSize(26)
    h.GetXaxis().SetTitleFont(43)
    h.GetXaxis().SetTitleOffset(4.)
    h.GetXaxis().SetLabelFont(43)
    h.GetXaxis().SetLabelSize(26)
    h.GetXaxis().SetLabelOffset(0.04)


# Parse input arguments
parser = argparse.ArgumentParser()
parser.add_argument("-v", "--verbose", action="store_true",
                    help="Be verbose")
parser.add_argument("-c", "--config", default="ratio_plot.config.toml",
                    help="config file")
args = parser.parse_args()

# Get plotting configuration parameters
params = config.read(args.config)

# Create output directory
outdir = make_outdir(params)

# Need to keep track of open files to read from TTree
open_files = []

# Load TTrees
for file in params['file']:
    # Replace the tree name with the tree itself
    # You can always get the name back with tree.GetName()
    tmp_file, file['tree'] = utils.GetTTree(file['name'], file['tree'])
    open_files.append(tmp_file)


# ---------- Loop over the set of kinematic variables and plot ---------- #

astyle.SetAtlasStyle()

for plot in params['plot']:
    if args.verbose:
        print("Creating plot {}".format(plot['name']))

    # Create canvas
    canv = root.TCanvas(
        "canv_" + plot['name'], "", params['canvas']['w'], params['canvas']['h']
    )

    # List of histograms in plot
    hists = []

    # ----- Loop over the files and get trees ----- #
    for file in params['file']:
        if args.verbose:
            print("Reading in branch from " + file['name'])

        weight_str = "{}*{}".format(file['scale'], plot['cuts'])

        if args.verbose:
            print("Applying weight: " + weight_str)

        hist = utils.MakeHistogram(
            file['tree'], plot['name'],
            plot['nbins'], plot['xmin'], plot['xmax'],
            selections=weight_str,
            label=file['label']
        )

        # Aesthetics
        utils.SetHistogramLine(hist, root.TColor.GetColor(file['color']), 2)

        if file['fill'] is not None:
            utils.SetHistogramFill(hist, root.TColor.GetColor(file['fill']))

        hists.append(hist)

    # Normalizes histograms to unity
    if params['norm']:
        utils.NormalizeHistograms(hists)

    # Amount to scale the y-axis to allow room for labels/legends
    max_factor = 12 if params['logy'] else 1.2

    hist_min = utils.GetMinimum(hists)
    hist_max = utils.GetMaximum(hists)

    # Format histogram
    if params['norm']:
        utils.FormatHistograms(
            hists,
            xtitle=plot['label'],
            ytitle="Normalized to unity",
            max=max_factor * hist_max,
            min=0.5*hist_min if params['logy'] else 0
        )
    else:
        utils.FormatHistograms(
            hists,
            xtitle=plot['label'],
            ytitle="Events",
            max=max_factor * hist_max,
            min=0.5*hist_min + 0.0001 if params['logy'] else 0
        )

    # Build upper pad for histograms
    hist_pad = utils.MakePad("histpad", "", 0, 0.3, 1, 1)
    hist_pad.SetBottomMargin(0.03)

    hist_pad.Draw()
    hist_pad.cd()

    utils.DrawHistograms(hists, "hist")

    # Remove x-axis labels
    for hist in hists:
        hist.GetXaxis().SetLabelSize(0)

    # Set logarithmic scale on y-axis
    if params['logy']:
        if args.verbose:
            print("Set histogram y-axis to logarithmic scale")
        gPad.SetLogy()

    # Draw labels and legend on the canvas
    astyle.ATLASLabel(0.2, 0.86, "Internal")

    for label in params['label']:
        utils.DrawText(label['x'], label['y'], label['text'], 1, 0.05)

    if 'legend' in params:
        legend = utils.MakeLegend(
            hists, xmin=params['legend']['x'], ymin=params['legend']['y']
        )
        legend.Draw()

    # Go back to the main canvas before defining lower pad
    canv.cd()

    # Build lower pad for ratio plots
    ratio_pad = utils.MakePad("ratiopad", "", 0, 0, 1, 0.3)
    ratio_pad.SetTopMargin(0.01)
    ratio_pad.SetBottomMargin(0.35)
    ratio_pad.SetGridy()  # horizontal grid

    ratio_pad.Draw()
    ratio_pad.cd()

    # Draw the ratio plots
    ratio_hist = root.TH1F()
    for i, hist in enumerate(hists):
        if i == 0:
            continue
        else:
            ratio_hist = hist.Clone("ratio_hist")
            ratio_hist.Divide(hists[0])
            ratio_hist.SetLineColor(1)
            ratio_hist.SetMarkerColor(hist.GetLineColor())

            FormatRatioPlot(ratio_hist, 0.4, 2.1, "Ratio")

    # Draw line where ratio = 1
    utils.DrawLineAt1(ratio_hist, color=2)

    gPad.RedrawAxis()

    canv.Print("{}/{}_{}.{}".format(
        outdir, params['title'], plot['name'], params['ext'])
    )

    if args.verbose:
        print("")

# Cleanup
for file in open_files:
    file.Close()

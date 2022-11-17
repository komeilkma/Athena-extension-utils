"""
Plotting Utils
==============

The main suite of utility functions.
"""

from __future__ import absolute_import, division, print_function

import sys
import datetime
import re

from atlasplots.console import bcolor

import ROOT as root
from ROOT import gDirectory


def DrawText(x, y, text, color=1, size=0.05):
    """Draw text.

    Parameters
    ----------
    x : float
        x position in NDC coordinates
    y : float
        y position in NDC coordinates
    text : string, optional
        The text
    color : int, optional
        Text colour (the default is 1, i.e. black).
        See https://root.cern.ch/doc/master/classTColor.html.
        If you know the hex code, rgb values, etc., use ``ROOT.TColor.GetColor()``
    size : float, optional
        Text size
        See https://root.cern.ch/doc/master/classTLatex.html
    """
    l = root.TLatex()
    # l.SetTextAlign(12)
    l.SetTextSize(size)
    l.SetNDC()
    l.SetTextColor(color)
    l.DrawLatex(x, y, text)


def GetTTree(filename, treename):
    """Get TTree from file(s).

    Returns the TTree if reading a single file or a TChain if reading multiple
    files from a directory. Exits if file or tree cannot be read.

    Parameters
    ----------
    filename : str
        Name of ROOT file or the containing directory
    treename : str
        Name of TTree

    Returns
    -------
    TTree or TChain
        The TTree or TChain
    """
    if filename.endswith(".root"):
        file = root.TFile.Open(filename)
        print("Reading in tree {} from {}... "
              .format(bcolor.bold + treename + bcolor.end, filename), end="")

        if not file:
            # ROOT prints its own "file not found" message
            print("{}".format(bcolor.red + "Failed" + bcolor.end))
            sys.exit(1)

        tree = file.Get(treename)

        if not tree:
            print("{}".format(bcolor.red + "Failed" + bcolor.end))
            print("{}  Tree '{}' not found in {}"
                  .format(bcolor.warning(), treename, filename))

            # Look into file to see what else is there
            items_in_file = file.GetListOfKeys()

            if items_in_file is not None:
                print("\nDid you mean:")
                for item in items_in_file:
                    print("  * {}".format(item.GetName()))

            sys.exit(1)

        print("{}".format(bcolor.ok()))

    else:
        # TODO: Exception handling
        chain = root.TChain(treename)
        chain.Add(filename + "/*.root")
        tree = chain

    # tree.SetDirectory(0)

    return file, tree


def GetTChain(filenames, treename):
    """Get TChain (list of Root files containing the same tree)

    Parameters
    ----------
    filenames : [str]
        Name(s) of ROOT file(s)
    treename : str
        Name of TTree

    Returns
    -------
    TTree or TChain
        The TTree or TChain
    """
    if type(filenames) is not list:
        filenames = [filenames]

    chain = root.TChain(treename)

    for file in filenames:
        chain.Add(file)

    return chain


def MakeHistogram(tree, plotname, nbins, xmin, xmax, selections="", shift="", label=""):
    """Make histogram from a TTree.

    Parameters
    ----------
    tree : TTree
        The tree from which the histogram is made
    plotname : str
        The plot name; i.e. the name of the branch (or variable) being plotted
    nbins : int
        Number of bins
    xmin : float
        Lower edge of first bin
    xmax : float
        Upper edge of last bin (not included in last bin)
    selections : str, optional
        Apply selections.
        See ``TTree::Draw()`` at https://root.cern.ch/doc/master/classTTree.html
        for more information
    shift : str, optional
        Shift histogram by this amount; subtacts this value from the variable
        being plotted
    label : str, optional
        The histogram's label; i.e. the entry that will appear in the legend

    Returns
    -------
    TH1
        The histogram
    """
    histname = plotname

    # Use current time to uniquely identify histograms
    unique_id = datetime.datetime.now().isoformat()
    histname += "_{}".format(unique_id)

    # Remove reserved characters
    histname = histname.replace("/", "")
    histname = histname.replace("*", "")
    histname = histname.replace("(", "")
    histname = histname.replace("-", "_")
    histname = histname.replace(")", "")
    histname = histname.replace(":", "")
    histname = histname.replace(".", "")

    indices = "({},{},{})".format(nbins, xmin, xmax)

    if shift:
        plotname += "-{}".format(shift)

    tree.Draw(plotname + ">>" + histname + indices, selections, "e")

    hist = gDirectory.Get(histname)
    hist.SetDirectory(0)

    if hist.GetEntries() == 0:
        print("{}  Histogram is empty!".format(bcolor.warning()))

    if label:
        hist.label = label

    return hist


def SetHistogramLine(hist, color=1, width=1, style=1, alpha=1):
    """Set the histogram line properties.

    See https://root.cern.ch/doc/master/classTAttLine.html for more information
    on line properties.

    Parameters
    ----------
    hist : TH1
        The histogram
    color : int, optional
        Line colour (the default is 1, i.e. black).
        See https://root.cern.ch/doc/master/classTColor.html.
        If you know the hex code, rgb values, etc., use ``ROOT.TColor.GetColor()``
    width : int, optional
        Line width in pixels; between 1 and 10 (the default is 1)
    style : int, optional
        Line style; between 1 and 10 (the default is 1, i.e. solid line)
    alpha : float, optional
        Line transparency; between 0 and 1 (the default is 1, i.e. opaque)
    """
    hist.SetLineColor(color)
    hist.SetLineWidth(width)
    hist.SetLineStyle(style)
    hist.SetLineStyle(alpha)


def SetHistogramFill(hist, color=None, style=None, alpha=1):
    """Set the histogram fill properties.

    If ``SetHistogramFill()`` is called with no colour specified, the fill colour
    is set to the same as the histogram's line colour.

    See https://root.cern.ch/doc/master/classTAttFill.html for more information
    on fill properties.

    Parameters
    ----------
    hist : TH1
        The histogram
    color : int, optional
        Fill colour.
        See https://root.cern.ch/doc/master/classTColor.html.
        If you know the hex code, rgb values, etc., use ``ROOT.TColor.GetColor()``
    style : int, optional
        Fill style; this one's complicated so best to see the ROOT documentation
    alpha : float, optional
        Fill transparency; between 0 and 1 (the default is 1, i.e. opaque)
    """
    if style is not None:
        hist.SetFillStyle(style)

    if color is not None:
        hist.SetFillColor(color)
    else:
        hist.SetFillColor(hist.GetLineColor())

    if alpha != 1:
        hist.SetFillColorAlpha(color, alpha)


def FormatHistograms(hists, title="", xtitle="", ytitle="", xtitle_offset=None,
                     ytitle_offset=None, units="", max=None, min=0):
    """Format histograms and add axis labels.

    Typically the y-axis label contains the bin width with units, for example,
    "Events / 10 GeV". The preferred way to get the bin width is at run time
    rather than computing it by hand and including it in the config file. So, if
    no units are specified, the function will try to parse the units from the
    x-axis label and apply it to the y-axis.

    Parameters
    ----------
    hists : [TH1]
        List of histograms
    title : str, optional
        Histogram title; typically empty (the default is "")
    xtitle : str, optional
        x-axis label (the default is "")
    ytitle : str, optional
        y-axis label (the default is "")
    xtitle_offset : float, optional
        Label offset from x-axis (the default is None, i.e. use ROOT's default)
    ytitle_offset : float, optional
        Label offset from y-axis (the default is None, i.e. use ROOT's default)
    units : str, optional
        Units (the default is "")
    max : float, optional
        Histogram maximum value (the default is None)
    min : float, optional
        Histogram minimum value (the default is 0)
    """
    # If hists is a single value, convert to list
    if type(hists) is not list:
        hists = [hists]

    SetYRange(hists, max, min)

    # Try to parse units from xtitle
    if not units and xtitle:
        pattern = re.compile(r".*\[(.*\w.*)\]\s*$")
        match = pattern.search(xtitle)

        if match:
            units = match.group(1)
        else:
            units = ""

    for hist in hists:
        if title:
            hist.SetTitle(title)

        if xtitle:
            hist.GetXaxis().SetTitle(xtitle)

        if ytitle:
            ytitle += " / {:g} {}".format(hist.GetXaxis().GetBinWidth(1), units)
            hist.GetYaxis().SetTitle(ytitle)

        if xtitle_offset:
            hist.GetXaxis().SetTitleOffset(xtitle_offset)

        if ytitle_offset:
            hist.GetXaxis().SetTitleOffset(ytitle_offset)


def DrawHistograms(hists, options=""):
    """Draw the histograms.

    The histograms should already have their formatting applied at this point

    Parameters
    ----------
    hists : [TH1]
        List of histograms
    options : str, optional
        Drawing options (the default is "")
    """
    # If hists is a single value, convert to list
    if type(hists) is not list:
        hists = [hists]

    for i, hist in enumerate(hists):
        if i == 0:
            hist.Draw(options)
        else:
            hist.Draw("same " + options)


def NormalizeHistograms(hists, width=False):
    """Normalize a list of histograms to unity.

    Parameters
    ----------
    hists : [TH1]
        List of histograms
    width : bool, optional
        If true, the bin contents and errors are divided by the bin width
        (the default is False)
    """
    # If hists is a single value, convert to list
    if type(hists) is not list:
        hists = [hists]

    option = "width" if width else ""

    for hist in hists:
        hist.Scale(1.0 / hist.Integral(), option)


def GetMinimum(hists):
    """Get minimum value (i.e. value of 'shortest' bin) of a list of histograms.

    Parameters
    ----------
    hists : [TH1]
        List of histograms

    Returns
    -------
    float
        Minimum value
    """
    # If hists is a single value, convert to list
    if type(hists) is not list:
        hists = [hists]

    histmin = hists[0].GetMinimum()

    for hist in hists:
        tmpmin = hist.GetMinimum()

        if hist.GetMinimum() < histmin:
            histmin = tmpmin

    return histmin


def GetMaximum(hists):
    """Get maximum value (i.e. value of 'tallest' bin) of a list of histograms.

    Parameters
    ----------
    hists : [TH1]
        List of histograms

    Returns
    -------
    float
        Maximum value
    """
    # If hists is a single value, convert to list
    if type(hists) is not list:
        hists = [hists]

    histmin = hists[0].GetMaximum()

    for hist in hists:
        tmpmin = hist.GetMaximum()

        if hist.GetMaximum() > histmin:
            histmin = tmpmin

    return histmin


def SetYRange(hists, max=None, min=0):
    """Set the y-axis range (max and min) on a list of histograms.

    If the max value is not provided, it calls :py:func:`GetMaximum` to get the
    maximum value from the list of histograms

    Parameters
    ----------
    hists : [TH1]
        List of histograms
    max : float, optional
        Max value (the default is None)
    min : float, optional
        Min value (the default is 0)
    """
    # If hists is a single value, convert to list
    if type(hists) is not list:
        hists = [hists]

    if max is None:
        plotmax = GetMaximum(hists)
    else:
        plotmax = max

    for hist in hists:
        hist.GetYaxis().SetRangeUser(min, plotmax)


def MakePad(name, title, xlow, ylow, xup, yup):
    """Make a pad.

    This function replaces the typical TPad constructor because of an
    unfortunate quirk of PyROOT that forces you to set the ownership of the Pad
    when creating it, otherwise it gives a segmentation fault.

    See https://root.cern.ch/doc/master/classTPad.html.

    Parameters
    ----------
    name : str
        Pad name
    title : str
        Pad title
    xlow : float
        The x position of the bottom left point of the pad
    ylow : float
        The y position of the bottom left point of the pad
    xup : float
        The x position of the top right point of the pad
    yup : float
        The y position of the top right point of the pad
    """
    pad = root.TPad(name, title, xlow, ylow, xup, yup)
    root.SetOwnership(pad, False)

    return pad


def MakeLegend(hists, xmin=0.65, ymin=0.65, options="LF"):
    """Draw the legend.

    Legend drawing options are:

    * L: draw line associated with hists' lines
    * P: draw polymarker associated with hists' marker
    * F: draw a box with fill associated with hists' fill
    * E: draw vertical error bar if option "L" is also specified

    See https://root.cern.ch/doc/master/classTLegend.html for full details.

    Parameters
    ----------
    hists : [TH1]
        List of histograms
    xmin : float, optional
        The x position of the bottom left point of the legend
        (the default is 0.65)
    ymin : float, optional
        The y position of the bottom left point of the legend
        (the default is 0.65)
    options : string, optional
        Pass these options to TLegend::AddEntry().
        Default is "LF"
    """
    # If hists is a single value, convert to list
    if type(hists) is not list:
        hists = [hists]

    xmax = xmin + 0.10
    ymax = ymin + 0.05 * (len(hists) + 1)

    legend = root.TLegend(xmin, ymin, xmax, ymax)
    legend.SetTextSize(0.03)
    legend.SetFillColor(0)
    legend.SetLineColor(0)
    legend.SetBorderSize(0)

    for hist in hists:
        if hasattr(hist, 'label'):
            legend.AddEntry(hist, hist.label, options)
        else:
            legend.AddEntry(hist)
            print("{}  Making legend but histogram '{}' has no label"
                  .format(bcolor.warning(), hist.GetTitle()))

    return legend


def DrawLine(x1, y1, x2, y2, color=1, width=1, style=1, alpha=1):
    """Draw a line on a histogram.

    See https://root.cern.ch/doc/master/classTAttLine.html for more information
    on line properties.

    Parameters
    ----------
    x1, y1, x2, y2 : float
        Line coordinates
    color : int, optional
        Line colour (the default is 1, i.e. black).
        See https://root.cern.ch/doc/master/classTColor.html.
        If you know the hex code, rgb values, etc., use ``ROOT.TColor.GetColor()``
    width : int, optional
        Line width in pixels; between 1 and 10 (the default is 1)
    style : int, optional
        Line style; between 1 and 10 (the default is 1, i.e. solid line)
    alpha : float, optional
        Line transparency; between 0 and 1 (the default is 1, i.e. opaque)
    """
    line = root.TLine()

    if color != 1:
        line.SetLineColor(color)

    if width != 1:
        line.SetLineWidth(width)

    if style != 1:
        line.SetLineStyle(style)

    if alpha != 1:
        line.SetLineColorAlpha(alpha)

    line.DrawLine(x1, y1, x2, y2)


def DrawLineAt1(hist, color=1, width=1, style=1, alpha=1):
    """Draw a horizontal line at y=1 on a histogram.

    This is particularly useful for ratio plots.

    See https://root.cern.ch/doc/master/classTAttLine.html for more information
    on line properties.

    Parameters
    ----------
    hist : TH1
        The histogram on which to draw the line
    color : int, optional
        Line colour (the default is 1, i.e. black).
        See https://root.cern.ch/doc/master/classTColor.html.
        If you know the hex code, rgb values, etc., use ``ROOT.TColor.GetColor()``
    width : int, optional
        Line width in pixels; between 1 and 10 (the default is 1)
    style : int, optional
        Line style; between 1 and 10 (the default is 1, i.e. solid line)
    alpha : float, optional
        Line transparency; between 0 and 1 (the default is 1, i.e. opaque)
    """
    line = root.TLine()

    if color != 1:
        line.SetLineColor(color)

    if width != 1:
        line.SetLineWidth(width)

    if style != 1:
        line.SetLineStyle(style)

    if alpha != 1:
        line.SetLineColorAlpha(alpha)

    line.DrawLine(
        hist.GetXaxis().GetXmin(), 1,
        hist.GetXaxis().GetXmax(), 1
    )

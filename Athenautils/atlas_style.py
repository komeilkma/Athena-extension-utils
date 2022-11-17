"""
ATLAS Style
===========

This module contains the ATLAS Style definition for plotting, as well as methods
for adding the ubiquitous *ATLAS* label and a few other deprecated methods
for adding text to plots.

The ATLAS Style will automatically take care of setting up the ATLAS default
plot formatting, and namely it will:

* Remove the ROOT default grey background, and set all canvas backgrounds to
  white (except for TLegend objects);
* Set the pad margins to decent values, so that your axis labels will not
  overlap on the axis figures;
* Select the ATLAS default font (Helvetica) at its default size;
* Select the default marker type (full circle, black);
* Increase the default line thicknesses (figures in paper are usually greatly
  reduced: this improves visibility in both articles and conference
  presentations);
* Avoid the display of any of the standard histogram decorations (title,
  statistics box, ...);
* Put tick marks on top and right hand side of your plots.

See https://twiki.cern.ch/twiki/bin/view/AtlasProtected/PubComPlotStyle for the
full documentation on style guidelines for ATLAS plots (requires CERN login).

Examples
--------

Import the module:

>>> import ROOT
>>> from atlasplots import atlas_style as astyle

Set the ATLAS Style globally:

>>> astyle.SetAtlasStyle()

Add the "*ATLAS* Internal" label:

>>> astyle.ATLASLabel(0.2, 0.87, "Internal")

For a collection of complete examples, see the :ref:`examples` section.
"""

from __future__ import absolute_import, division, print_function

from atlasplots.console import bcolor

import ROOT as root
from ROOT import gPad, gROOT


def SetAtlasStyle():
    """Applying ATLAS style settings globally.
    """
    print("\n{}\n".format(bcolor.green + "Applying ATLAS style settings..." + bcolor.end))

    AtlasStyle()
    gROOT.SetStyle("ATLAS")
    gROOT.ForceStyle()


def AtlasStyle():
    """Define the ATLAS style.

    Returns
    -------
    TStyle
        The style object with ATLAS style settings
    """
    atlasStyle = root.TStyle("ATLAS", "Atlas style")

    # Use plain black on white colors
    color = 0  # white
    atlasStyle.SetFrameBorderMode(color)
    atlasStyle.SetFrameFillColor(color)
    atlasStyle.SetCanvasBorderMode(color)
    atlasStyle.SetCanvasColor(color)
    atlasStyle.SetPadBorderMode(color)
    atlasStyle.SetPadColor(color)
    atlasStyle.SetStatColor(color)

    # Set the paper and margin sizes
    atlasStyle.SetPaperSize(20, 26)

    # Set margin sizes
    atlasStyle.SetPadTopMargin(0.05)
    atlasStyle.SetPadRightMargin(0.05)
    atlasStyle.SetPadBottomMargin(0.16)
    atlasStyle.SetPadLeftMargin(0.16)

    # Set title offsets (for axis label)
    atlasStyle.SetTitleXOffset(1.4)
    atlasStyle.SetTitleYOffset(1.4)

    # Use large fonts
    # See https://root.cern.ch/doc/master/classTAttText.html#T53
    font = 42  # Helvetica
    tsize = 0.05
    atlasStyle.SetTextFont(font)

    atlasStyle.SetTextSize(tsize)
    atlasStyle.SetLabelFont(font, "x")
    atlasStyle.SetTitleFont(font, "x")
    atlasStyle.SetLabelFont(font, "y")
    atlasStyle.SetTitleFont(font, "y")
    atlasStyle.SetLabelFont(font, "z")
    atlasStyle.SetTitleFont(font, "z")

    atlasStyle.SetLabelSize(tsize, "x")
    atlasStyle.SetTitleSize(tsize, "x")
    atlasStyle.SetLabelSize(tsize, "y")
    atlasStyle.SetTitleSize(tsize, "y")
    atlasStyle.SetLabelSize(tsize, "z")
    atlasStyle.SetTitleSize(tsize, "z")

    # Use bold lines and markers
    atlasStyle.SetMarkerStyle(20)
    atlasStyle.SetMarkerSize(1.2)
    atlasStyle.SetHistLineWidth(2)
    atlasStyle.SetLineStyleString(2, "[12 12]")  # postscript dashes

    # Get rid of X error bars (as recommended in ATLAS figure guidelines)
    atlasStyle.SetErrorX(0.0001)
    # Get rid of error bar caps
    atlasStyle.SetEndErrorSize(0.)

    # Do not display any of the standard histogram decorations
    atlasStyle.SetOptTitle(0)
    # atlasStyle.SetOptStat(1111)
    atlasStyle.SetOptStat(0)
    # atlasStyle.SetOptFit(1111)
    atlasStyle.SetOptFit(0)

    # Put tick marks on top and RHS of plots
    atlasStyle.SetPadTickX(1)
    atlasStyle.SetPadTickY(1)

    return atlasStyle


def ATLASLabel(x, y, text=None, color=1):
    """Draw the ATLAS Label.

    Parameters
    ----------
    x : float
        x position in NDC coordinates
    y : float
        y position in NDC coordinates
    text : string, optional
        Text displayed next to label (the default is None)
    color : TColor, optional
        Text colour (the default is 1, i.e. black).
        See https://root.cern.ch/doc/master/classTColor.html
    """
    l = root.TLatex()
    l.SetNDC()
    l.SetTextColor(color)
    l.DrawLatex(x, y, r"#bf{#it{ATLAS}} " + text)


def ATLASLabelOld(x, y, preliminary=False, color=1):
    """Draw the ATLAS Label (old version).

    Note
    ----
    Use :py:func:`ATLASLabel` instead.

    Parameters
    ----------
    x : float
        x position in NDC coordinates
    y : float
        y position in NDC coordinates
    preliminary : bool, optional
        If True, write "Preliminary" next to label (the default is False)
    color : TColor, optional
        Text colour (the default is 1, i.e. black).
        See https://root.cern.ch/doc/master/classTColor.html
    """
    l = root.TLatex  # l.SetTextAlign(12); l.SetTextSize(tsize)
    l.SetNDC()
    l.SetTextFont(72)
    l.SetTextColor(color)
    l.DrawLatex(x, y, "ATLAS")

    if preliminary:
        p = root.TLatex
        p.SetNDC()
        p.SetTextFont(42)
        p.SetTextColor(color)
        p.DrawLatex(x + 0.115, y, "Preliminary")


def ATLASVersion(version, x=0.88, y=0.975, color=1):
    """Draw the version number.

    Parameters
    ----------
    version : string
        Version number
    x : float, optional
        x position in NDC coordinates (the default is 0.88)
    y : float, optional
        y position in NDC coordinates (the default is 0.975)
    color : TColor, optional
        Text colour (the default is 1, i.e. black).
        See https://root.cern.ch/doc/master/classTColor.html
    """
    l = root.TLatex
    l.SetTextAlign(22)
    l.SetTextSize(0.04)
    l.SetNDC()
    l.SetTextFont(72)
    l.SetTextColor(color)
    l.DrawLatex(x, y, "Version {}".format(version))

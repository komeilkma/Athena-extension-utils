#!/usr/bin/env python
"""
Random Histogram
================

This module plots a random histogram using the ATLAS Style.

.. literalinclude:: ../examples/random_hist.py
    :lines: 12-
"""

from __future__ import absolute_import, division, print_function

import ROOT as root

from atlasplots import atlas_style as astyle


def main():
    # Set the ATLAS Style
    astyle.SetAtlasStyle()

    # Construct the canvas
    c1 = root.TCanvas("c1", "The FillRandom example", 0, 0, 800, 600)

    # Define a distribution
    form1 = root.TFormula("form1", "abs(sin(x)/x)")
    sqroot = root.TF1("sqroot", "x*gaus(0) + [3]*form1", 0, 10)
    sqroot.SetParameters(10, 4, 1, 20)

    # Randomly fill the histrogram according to the above distribution
    hist = root.TH1F("hist", "Test random numbers", 100, 0, 10)
    hist.FillRandom("sqroot", 10000)
    hist.Draw()

    # Set axis titles
    hist.GetXaxis().SetTitle("x axis")
    hist.GetYaxis().SetTitle("y axis")

    # Add the ATLAS Label
    astyle.ATLASLabel(0.2, 0.87, "Internal")

    # Save the plot as a PDF
    c1.Update()
    c1.Print("random_hist.pdf")


if __name__ == '__main__':
    main()

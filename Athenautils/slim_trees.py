#!/usr/bin/env python3

"""
Slim Trees
==========

A quick and dirty script for reducing the file size of a ROOT file by removing
unwanted branches from a TTree.

Examples
--------

The following examples assume the branches you want included in the slimmed tree
are listed in ``slimbranches.txt``.

Slim the tree ``tree`` in ``data.root`` and save to ``data.slim.root``:

.. code:: bash

    $ slim-trees tree -i data.root

Slim the tree ``tree`` in ``data.root`` and save to ``newdata.root``:

.. code:: bash

    $ slim-trees tree -i data.root -o newdata.root

Slim the tree ``tree`` in ``data.root``, save to ``data.slim.root``, and only
keep the first 100 events:

.. code:: bash

    $ slim-trees tree -i data.root -t 100

Note
----

A standard ROOT installation provides a program called ``rootslimtree`` which
attempts to do much the same that this script does, but is more feature-rich.
The source code is available in the main `ROOT git repository
<https://github.com/root-project/root/blob/master/main/python/rootslimtree.py>`_.

There are some quirks about ``rootslimtree`` that I don't understand, for
example I've seen it *increase* the file size of ROOT file after stripping away
unwanted branches. This is obviously not ideal.
"""

from __future__ import absolute_import, division, print_function

import sys
import argparse

from atlasplots.console import bcolor

import ROOT as root


def main():
    # ----- Parse input arguments ----- #
    parser = argparse.ArgumentParser()
    parser.add_argument("tree", help="name of TTree")
    parser.add_argument("-i", "--input", help="input file")
    parser.add_argument("-o", "--output", help="output file")
    parser.add_argument("-b", "--branches", default="slimbranches.txt",
                        help="file with list of branches to keep (default is "
                        "%(default)s)")
    parser.add_argument("-t", "--trim", type=int,
                        help="trim the tree and keep this many events")
    args = parser.parse_args()

    # Generate output file name
    if args.output is not None:
        outname = args.output
    elif args.input[len(args.input)-4:] == "root":
        outname = args.input[:len(args.input)-5] + '.slim.root'
    else:
        print("{}  Problem getting output file name. Exiting..."
              .format(bcolor.error()))
        sys.exit(1)


    # ----- Slim the tree ----- #

    print("Reading in {}... ".format(args.input), end="")

    # Get old file and old tree
    oldfile = root.TFile.Open(args.input)
    oldtree = oldfile.Get(args.tree)

    # Set branches in old tree to copy over to new, slimmed tree
    oldtree.SetBranchStatus("*", 0)

    with open(args.branches, 'r') as f:
        for line in f:
            br = line.rstrip()  # Strip newline character at end of line
            oldtree.SetBranchStatus(br, 1)

    print("{}".format(bcolor.ok()))

    # Create a new file and a clone of old tree in new file
    newfile = root.TFile(outname, "RECREATE")

    if args.trim is None:
        # Don't trim; add all the entries to the new tree
        newtree = oldtree.CloneTree()
        newfile.Write()
    else:
        # Check the number of entries
        nentry = oldtree.GetEntries()
        if nentry < args.trim:
            print("{}  Attempting to trim {} entries when {} only has {}"
                  .format(bcolor.warning(), args.trim, args.tree, nentry))
            print("Inlcuding all entries in new tree")
            args.trim = nentry

        newtree = oldtree.CloneTree(0)

        for i in range(args.trim):
            oldtree.GetEntry(i)
            newtree.Fill()

        newfile.Write()

    print("Writing new tree to {}:\n".format(outname))

    newtree.Print()


if __name__ == '__main__':
    main()

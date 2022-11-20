#!/usr/bin/env bash
#
# Copyright (C) 2002-2018 CERN for the benefit of the ATLAS collaboration
#
# Testing script for asetup_wrapper
#
export AtlasSetup="/cvmfs/atlas.cern.ch/repo/ATLASLocalRootBase/x86_64/AtlasSetup/current/AtlasSetup"
export ASETUP_ARGS="$AtlasProject --releasebase=$AtlasBaseDir"
export ASETUP_PRELOAD="${TCMALLOCDIR}/libtcmalloc.so"
export ASETUP_EXTRA_SW_PATH="$PWD"

# Run asetup_wrapper and save output
out=`asetup_wrapper printenv`

# Check for correct setting of LD_PRELOAD
echo "$out" | grep "^LD_PRELOAD" | grep "${ASETUP_PRELOAD}" &> /dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: LD_PRELOAD not correctly set by asetup_wrapper"
    echo "ASETUP_PRELOAD=${ASETUP_PRELOAD}"
    echo "$out"
    exit 1
else
    echo "LD_PRELOAD set OK"
fi

# Check prepending of extra path
echo "$out" | grep "^LD_LIBRARY_PATH" | tr ':' '\n' | head -1 | grep "${ASETUP_EXTRA_SW_PATH}" &> /dev/null
if [ $? -ne 0 ]; then
    echo "ERROR: EXTRA_SW_PATH not correctly prepended by asetup_wrapper"
    echo "ASETUP_EXTRA_SW_PATH=${ASETUP_EXTRA_SW_PATH}"
    echo "$out"
    exit 1
else
    echo "EXTRA_SW_PATH set OK"
fi

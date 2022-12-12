#!/bin/bash


if test -z "${TDAQ_APPLICATION_NAME}" ; then echo "TDAQ_APPLICATION_NAME is undefined. Not sleeping." ; exit 0 ; fi

DELAY=5 

case "${TDAQ_APPLICATION_NAME}" in
    GM-Application:*:1 )        DELAY=5 ;;
    GM-Application:*:2 )        DELAY=180 ;;
    GM-Application:*:3 )        DELAY=360 ;;
    GM-Application:*:4 )        DELAY=540 ;;
    GM-Application:*:5 )        DELAY=720 ;;
    GM-Application:*:6 )        DELAY=900 ;;
    GM-Application:*:7 )        DELAY=1080 ;;

    TRT-Monitoring-Noise )      DELAY=5 ;;
    TRT-Monitoring-Beam )       DELAY=5 ;;
    TRT-Monitoring-01 )         DELAY=120 ;;
    TRT-Monitoring-02 )         DELAY=120 ;;
    TRT-Monitoring-03 )         DELAY=240 ;;
    TRT-Monitoring-04 )         DELAY=240 ;;
    TRT-Monitoring-05 )         DELAY=360 ;;
    TRT-Monitoring-06 )         DELAY=360 ;;
esac

echo -n "Sleeping for ${DELAY} seconds ..."
sleep "${DELAY}" && echo " Done."

#!/bin/bash

export PATH="/usr/bin:/bin"
echo "#### Starting Athena job ######################################################"
echo
date "+%a %F, %T %z (%Z)"
echo "$(whoami)@$(hostname):$(pwd)\$" "$0" "$@"
"${0%/*}/athena_job_delay.sh"
echo
echo "#### Input file and additional command-line tags for AtlasSetup ###############"
echo
echo "ATHENA_JOB_SETUP=${ATHENA_JOB_SETUP:-(none)}"
echo "ATHENA_JOB_TAGS=${ATHENA_JOB_TAGS:-(none)}"
if test -z "${ATHENA_JOB_SETUP}" -a -z "${ATHENA_JOB_TAGS}" ; then echo "Neither ATHENA_JOB_SETUP nor ATHENA_JOB_TAGS is defined. Exiting." ; exit 1 ; fi
if test -n "${ATHENA_JOB_SETUP}" ; then
    if test ! -f "${ATHENA_JOB_SETUP}" ; then echo "ATHENA_JOB_SETUP is defined, but the file could not be found. Exiting." ; exit 1 ; fi
    echo
    echo "Contents of AtlasSetup input file ${ATHENA_JOB_SETUP}:"
    echo "==== snip ====================================================================="
    cat "${ATHENA_JOB_SETUP}"
    echo "==== snap ====================================================================="
fi
echo
echo "#### Environment before AtlasSetup ############################################"
echo
env | sort
echo
echo "#### Running AtlasSetup #######################################################"
echo
export AtlasSetup="/sw/atlas/AtlasSetup"
echo   source "${AtlasSetup}/scripts/asetup.sh" "${ATHENA_JOB_SETUP:+--input}" "${ATHENA_JOB_SETUP}" "${ATHENA_JOB_TAGS}"
echo ; source "${AtlasSetup}/scripts/asetup.sh" "${ATHENA_JOB_SETUP:+--input}" "${ATHENA_JOB_SETUP}" "${ATHENA_JOB_TAGS}"
if test "$?" -ne 0 ; then echo "AtlasSetup failed. Exiting." ; exit 1 ; fi
echo "Done."
echo
echo "#### Environment after AtlasSetup #############################################"
echo
env | sort        
echo
echo "#### Executing Athena #########################################################"
echo
echo "$(which athena.py)" "$@"
echo
athena.py "$@" &
trap "
    kill -TERM $! 2> /dev/null && echo 'Sending Athena the TERM signal ...'
    sleep 3
    kill -KILL $! 2> /dev/null && echo 'Sending Athena the KILL signal ...'
    TRAPPED=1
" HUP INT TERM
wait $!
STATUS=$?
trap - HUP INT TERM
if test -n "${TRAPPED}" ; then exit 0 ; fi
if test "${STATUS}" -gt 128 ; then kill -$((STATUS-128)) $$ ; fi
exit "${STATUS}"

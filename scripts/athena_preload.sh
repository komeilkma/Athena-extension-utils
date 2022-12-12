#!/bin/sh

ld_preload()
{
    export LD_PRELOAD="$1${LD_PRELOAD:+:$LD_PRELOAD}"  # set or prepend
}

# possibly, allow user to run with full tcmalloc by setting this variable
export USETCMALLOCMINIMAL
if [ -z "$USETCMALLOCMINIMAL" ]; then
    USETCMALLOCMINIMAL=1
fi

# for profiling we need the full tcmalloc library
if [ -n "$CPUPROFILE" ] || [ -n "$HEAPPROFILE" ] || [ -n "$HEAPCHECK" ]; then
    USETCMALLOCMINIMAL=0
fi

#
# tcmalloc
#
if [ "$USETCMALLOC" = "1" ] || [ "$USETCMALLOC" = "true" ] ; then
    if [ -z $TCMALLOCDIR ]; then
        echo "ERROR: TCMALLOCDIR not defined"
        exit 1
    fi
    # test, if minimal tcmalloc is available. fallback to full library, if not
    if [ "$USETCMALLOCMINIMAL" = "1" ] || [ "$USETCMALLOCMINIMAL" = "true" ] ; then
        if [ ! -e "$TCMALLOCDIR/libtcmalloc_minimal.so" ]; then
            echo "WARNING: $TCMALLOCDIR/libtcmalloc_minimal.so does not exist. Falling back to libtcmalloc"
            USETCMALLOCMINIMAL=0
        fi
    fi
    # preload and run with correct tcmalloc, if requested
    if [ "$USETCMALLOCMINIMAL" = "0" ]; then
        if [ ! -e "$TCMALLOCDIR/libtcmalloc.so" ]; then
            echo "ERROR: $TCMALLOCDIR/libtcmalloc.so does not exist"
            exit 1
        fi
        echo "Preloading tcmalloc.so"
        ld_preload "$TCMALLOCDIR/libtcmalloc.so"
    else
        echo "Preloading tcmalloc_minimal.so"
        ld_preload "$TCMALLOCDIR/libtcmalloc_minimal.so"
    fi
fi

#
# Intel math library
#
if [ "$USEIMF" = "1" ] || [ "$USEIMF" = "true" ]; then
    if [ -z "${ATLASMKLLIBDIR_PRELOAD}" ]; then
        # Be resilient for platforms without Intel math library (ATLINFR-4503)
        echo "WARNING: Intel math library (ATLASMKLLIBDIR_PRELOAD) is not available for"\
             "platform ${BINARY_TAG}. Skipping preload."
    else
        IMF_LIB1="$ATLASMKLLIBDIR_PRELOAD/libimf.so"
        IMF_LIB2="$ATLASMKLLIBDIR_PRELOAD/libintlc.so.5"
        if [ ! -e "$IMF_LIB1" ]; then
            echo "ERROR: $IMF_LIB1 does not exit"
            exit 1
        elif [ ! -e "$IMF_LIB2" ]; then
            echo "ERROR: $IMF_LIB2 does not exit"
            exit 1
        else
            echo "Preloading `basename $IMF_LIB1`"
            echo "Preloading `basename $IMF_LIB2`"
            ld_preload "$IMF_LIB1"
            ld_preload "$IMF_LIB2"
        fi
    fi
fi

#
# Exception trace collector
#
if [ "$USEEXCTRACE" = "1" ] || [ "$USEEXCTRACE" = "true" ]; then
    # Abuse which to search for the library.
    EXCTRACE_LIB=`PATH=$LD_LIBRARY_PATH /usr/bin/which libexctrace_collector.so`
    if [ "$EXCTRACE_LIB" = "" ]; then
        error "ERROR: Cannot find libexctrace_collector.so"
        exit 1
    fi
    echo "Preloading `basename $EXCTRACE_LIB`"
    ld_preload "$EXCTRACE_LIB"
fi

#
# User specific preload
#
if [ -n "$ATHENA_ADD_PRELOAD" ] && [ "$ATHENA_DROP_RELOAD" != "1" ]; then
    echo "Preloading $ATHENA_ADD_PRELOAD"
    ld_preload "$ATHENA_ADD_PRELOAD"
else
    unset ATHENA_ADD_PRELOAD
fi

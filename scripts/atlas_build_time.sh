atlas_build_time () {
   # Declare all the variables to be used, grabbing the first argument as the
   # "stamp"/name of the build step being run.
   local stamp="$1" c= t1= t2= status=
   shift
   # Collect all the remaining arguments given to the function.
   while test "X$1" != "X"; do
      c+=" \"$1\""
      shift
   done
   # Remember the start time.
   t1=`date +%s`
   # Run the command that's being timed, and remember its result.
   eval "$c"
   status=$?
   # Remember the stop time.
   t2=`date +%s`
   # Print a message that would be easy to parse for in the build logs,
   echo TIMESTAMP::${stamp}:: `expr $t2 - $t1` ::
   return $status
}

# Specification

AC-1: connected→unexpectedloss→reconnect invokes WeMacro connect again, no stalecount.
AC-2: MJKZZ rejects short/wrongaddress/nonresponse/checksumcorrupt notifications before applying data; valid frames retained.
AC-3: Manual Temma replies stn-on/off with coherent standby/motorsstate; v1/v2 return versionprefix and select speed.
AC-4: NexStar GOTO has300s monotonic overall bound matching parkdefault, cancelsmotion and ALERT onexpiry; healthymove preserved.
AC-5: Lakeside no-progress bound30s independent reporting cadence, renewed by positionprogress; missing/malformed/frozen stream stops CH# and ALERT.
AC-6: AO simulator never sends beyond fixedreplybuffer; oversizedfirmware rejected early.

Baseline9557ca85e. Portable drivers generatedfrom.driver, macBT handwrittenObjectiveC. Hardwaredecision:no. Relevantexisting serialsim tests and exactsource callback/syntheticArduino fixtures; no migration/broadrecertification. Complexityhigh time/lifecycle, medium consequence bounded reversible changes.

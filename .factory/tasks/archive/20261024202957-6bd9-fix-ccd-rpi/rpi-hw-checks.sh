#!/bin/bash
# Manual hardware checks of indigo_ccd_rpi against a running server on port 7700
S=${RPI_TEST_DIR:?set RPI_TEST_DIR to a writable directory containing fitsinfo.py}
B=${INDIGO_BIN:-build/bin}
D="RPI Camera ov5647@36"
P="$B/indigo_prop_tool"
FAIL=0
check() {
	if [ "$1" = "$2" ]; then
		echo "PASS $3"
	else
		echo "FAIL $3: expected '$2', got '$1'"
		FAIL=1
	fi
}
state() {
	"$P" list_state -p 7700 "$D.$1" 2>/dev/null | head -1 | sed 's/^.* = //;s/ .*//'
}
item() {
	"$P" get -p 7700 "$D.$1" 2>/dev/null
}
expose() {
	"$P" set -p 7700 -t "$2" -w OK "$D.CCD_EXPOSURE.EXPOSURE=$1" >/dev/null 2>&1
	state CCD_EXPOSURE
}
last_file() {
	ls -t "$S"/img_*.fits 2>/dev/null | head -1
}
info() {
	python3 -I "$S/fitsinfo.py" "$(last_file)"
}

"$P" set -p 7700 "$D.CCD_LOCAL_MODE.DIR=$S/;PREFIX=img_XXX" >/dev/null
"$P" set -p 7700 "$D.CCD_UPLOAD_MODE.LOCAL=ON" >/dev/null
"$P" set -p 7700 "$D.CCD_IMAGE_FORMAT.FITS=ON" >/dev/null

echo "== full frame BGR24 1 s"
check "$(expose 1 20)" OK "exposure state"
info
check "$(info | cut -d' ' -f2)" 2592x1944x3 "full sensor size"
BGR_MEANS=$(info | sed 's/.*means \([0-9. ]*\) rg.*/\1/')

echo "== RGB24 has the same channel order as BGR24"
"$P" set -p 7700 -w OK "$D.CCD_MODE.RGB24_1=ON" >/dev/null
check "$(expose 1 20)" OK "exposure state"
info
RGB_MEANS=$(info | sed 's/.*means \([0-9. ]*\) rg.*/\1/')
echo "BGR24 means $BGR_MEANS / RGB24 means $RGB_MEANS"
"$P" set -p 7700 -w OK "$D.CCD_MODE.BGR24_1=ON" >/dev/null

echo "== binning 2 through CCD_BIN"
"$P" set -p 7700 -w OK "$D.CCD_BIN.HORIZONTAL=2;VERTICAL=2" >/dev/null
check "$(item CCD_MODE.BGR24_2)" ON "mode follows binning"
check "$(expose 1 20)" OK "exposure state"
check "$(info | cut -d' ' -f2)" 1296x972x3 "binned size"
"$P" set -p 7700 -w OK "$D.CCD_MODE.BGR24_1=ON" >/dev/null

echo "== subframe 800x600 at 200,100"
"$P" set -p 7700 -w OK "$D.CCD_FRAME.LEFT=200;TOP=100;WIDTH=800;HEIGHT=600;BITS_PER_PIXEL=24" >/dev/null
check "$(state CCD_FRAME)" OK "frame state with BITS_PER_PIXEL=24"
check "$(expose 1 20)" OK "exposure state"
check "$(info | cut -d' ' -f2)" 800x600x3 "subframe size"
echo "== subframe binned 2"
"$P" set -p 7700 -w OK "$D.CCD_BIN.HORIZONTAL=2;VERTICAL=2" >/dev/null
check "$(expose 1 20)" OK "exposure state"
check "$(info | cut -d' ' -f2)" 400x300x3 "binned subframe size"
"$P" set -p 7700 -w OK "$D.CCD_BIN.HORIZONTAL=1;VERTICAL=1" >/dev/null
"$P" set -p 7700 -w OK "$D.CCD_FRAME.LEFT=0;TOP=0;WIDTH=2592;HEIGHT=1944" >/dev/null

echo "== bits per pixel outside the modes is clamped by the framework"
"$P" set -p 7700 -w ANY "$D.CCD_FRAME.BITS_PER_PIXEL=8" >/dev/null 2>&1
check "$(item CCD_FRAME.BITS_PER_PIXEL)" 24 "bits per pixel kept"
check "$(item CCD_MODE.BGR24_1)" ON "mode kept"

echo "== saturation 0 persists over two exposures"
"$P" set -p 7700 -w OK "$D.X_CCD_ADVANCED.SATURATION=0" >/dev/null
check "$(expose 0.5 20)" OK "exposure 1 state"
info
check "$(info | sed 's/.*rg_diff \([0-9]*\).*/\1/')" 0 "exposure 1 is grey"
check "$(expose 0.5 20)" OK "exposure 2 state"
info
check "$(info | sed 's/.*rg_diff \([0-9]*\).*/\1/')" 0 "exposure 2 is grey"
"$P" set -p 7700 -w OK "$D.X_CCD_ADVANCED.SATURATION=1" >/dev/null

echo "== countdown during a 3 s exposure"
"$P" set -p 7700 -t 20 -w OK -s "$D.CCD_EXPOSURE.EXPOSURE=3" 2>&1 | tr '\n' ' '
echo

echo "== abort during a 3 s exposure, then a new exposure"
T0=$(date +%s.%N)
"$P" set -p 7700 "$D.CCD_EXPOSURE.EXPOSURE=3" >/dev/null 2>&1 &
sleep 1
"$P" set -p 7700 "$D.CCD_ABORT_EXPOSURE.ABORT_EXPOSURE=ON" >/dev/null 2>&1 &
echo "abort sent after $(echo "$(date +%s.%N) - $T0" | bc) s"
sleep 0.5
check "$(state CCD_EXPOSURE)" ALERT "aborted exposure state"
sleep 3
check "$(state CCD_EXPOSURE)" ALERT "no late completion after abort"
check "$(expose 1 20)" OK "exposure after abort"

echo "== abort while waiting for readout"
"$P" set -p 7700 "$D.CCD_EXPOSURE.EXPOSURE=1" >/dev/null 2>&1 &
sleep 1.05
"$P" set -p 7700 "$D.CCD_ABORT_EXPOSURE.ABORT_EXPOSURE=ON" >/dev/null 2>&1 &
sleep 2
check "$(state CCD_EXPOSURE)" ALERT "exposure aborted during readout wait"
check "$(expose 1 20)" OK "exposure after early abort"

echo "== disconnect during exposure, reconnect and expose"
"$P" set -p 7700 "$D.CCD_EXPOSURE.EXPOSURE=3" >/dev/null 2>&1 &
sleep 1
"$P" set -p 7700 -t 20 -w OK "$D.CONNECTION.DISCONNECTED=ON" >/dev/null 2>&1
check "$(item CONNECTION.DISCONNECTED)" ON "disconnected"
"$P" set -p 7700 -t 20 -w OK "$D.CONNECTION.CONNECTED=ON" >/dev/null 2>&1
check "$(item CONNECTION.CONNECTED)" ON "reconnected"
[ "$(state CCD_EXPOSURE)" != BUSY ] && echo "PASS exposure not left busy ($(state CCD_EXPOSURE))" || { echo "FAIL exposure left busy"; FAIL=1; }
"$P" set -p 7700 "$D.CCD_UPLOAD_MODE.LOCAL=ON" >/dev/null
check "$(expose 1 20)" OK "exposure after reconnect"

echo "== three connect cycles"
for i in 1 2 3; do
	"$P" set -p 7700 -t 20 -w OK "$D.CONNECTION.DISCONNECTED=ON" >/dev/null 2>&1
	"$P" set -p 7700 -t 20 -w OK "$D.CONNECTION.CONNECTED=ON" >/dev/null 2>&1
	check "$(item CONNECTION.CONNECTED)" ON "cycle $i connected"
	check "$(item X_CCD_ADVANCED.SATURATION)" 1 "cycle $i advanced property defined"
done
"$P" set -p 7700 "$D.CCD_UPLOAD_MODE.LOCAL=ON" >/dev/null
check "$(expose 0.1 20)" OK "short exposure"
info

echo "RESULT $([ $FAIL = 0 ] && echo PASS || echo FAIL)"

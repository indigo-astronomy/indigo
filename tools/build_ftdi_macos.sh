#!/bin/sh
# Copyright (c) 2026 INDIGO initiative
# All rights reserved.
#
# You may use this software under the terms of 'INDIGO Astronomy
# open-source license' (see LICENSE.md).
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Build privately and publish only a complete, detached FTDI dependency.
set -eu
if [ "$#" -lt 4 ]; then
	printf 'Usage: %s IMAGE INCLUDE_DIR ARCHIVE LIBUSB_OBJECT...\n' "$0" >&2
	exit 2
fi
image=$1
include_dir=$2
archive=$3
shift 3
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/indigo_d2xx.XXXXXX")
candidate=
mounted=false
detach_attempted=false

detach_image() {
	detach_attempted=true
	if ! hdiutil detach "$work_dir/mount"; then
		printf 'FTDI: normal detach failed; retrying this read-only mount with -force\n' >&2
		if ! hdiutil detach -force "$work_dir/mount"; then
			return 1
		fi
	fi
	mounted=false
}

cleanup() {
	status=$?
	trap - 0 HUP INT TERM
	if [ "$mounted" = true ] && [ "$detach_attempted" = false ]; then
		if ! detach_image; then
			status=1
		fi
	fi
	if [ "$mounted" = false ]; then
		rm -rf "$work_dir"
	else
		printf 'FTDI: mount workspace retained at %s; detach it before removing it\n' "$work_dir" >&2
	fi
	if [ -n "$candidate" ]; then
		rm -f "$candidate"
	fi
	exit "$status"
}

trap cleanup 0
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
candidate=$(mktemp "$(dirname "$archive")/.libftd2xx.XXXXXX")
mounted=true
hdiutil attach -readonly -noverify -noautoopen "$image" -mountpoint "$work_dir/mount"
architectures=$(lipo -archs "$work_dir/mount/release/build/libftd2xx.a")
[ -n "$architectures" ]
mkdir "$work_dir/thin"
for arch in $architectures; do
	lipo -thin "$arch" "$work_dir/mount/release/build/libftd2xx.a" -output "$work_dir/thin/$arch.a"
	ar -d "$work_dir/thin/$arch.a" "$@"
	ranlib "$work_dir/thin/$arch.a"
done
lipo -create "$work_dir/thin/"*.a -output "$candidate"
for arch in $architectures; do
	lipo "$candidate" -verify_arch "$arch"
done
install -m 0644 "$work_dir/mount/release/ftd2xx.h" "$work_dir/mount/release/WinTypes.h" "$include_dir"
detach_image
chmod 0644 "$candidate"
mv -f "$candidate" "$archive"
candidate=

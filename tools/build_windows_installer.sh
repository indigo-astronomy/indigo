#!/bin/bash

# Copyright (c) 2026 Rumen Bogdanovski
# All rights reserved.
#
# You can use this software under the terms of 'INDIGO Astronomy
# open-source license'
# (see https://github.com/indigo-astronomy/indigo/blob/master/LICENSE.md).
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
#
# Builds the Windows INDIGO binaries with MSBuild (Visual Studio) and
# packages the whole build output (indigo_server, tools, indigo.dll and
# every driver/agent DLL) into a Windows installer
# (indigo_windows/installer/indigo.iss) using Inno Setup 6.
#
# Run this from a bash shell that has the usual POSIX build tools on PATH
# (e.g. Git for Windows' bash, or MSYS2).
#
# Usage:
#   ./tools/build_windows_installer.sh [Release|Debug] [x64|ARM64]
#
# Environment variables:
#   INDIGO_BUILD_CONFIGURATION  Configuration to build/package (default: Release)
#   INDIGO_BUILD_PLATFORM       Platform to build/package (default: x64)
#   INDIGO_VERSION               overrides the version read from ../Makefile
#   INDIGO_BUILD                  overrides the build number read from ../Makefile
#   INDIGO_PRERELEASE             overrides the prerelease marker read from
#                                  ../Makefile (e.g. "a1"); appended to the
#                                  build number as "<build>~<prerelease>" in
#                                  the installer's version and file name if
#                                  non-empty
#   MSBUILD                     full path to MSBuild.exe, if it isn't already
#                                on PATH and isn't found via vswhere
#   ISCC                        full path to ISCC.exe (Inno Setup 6 compiler),
#                                if it isn't already on PATH and isn't found in
#                                one of the usual "Program Files" locations
#   SKIP_BUILD                  set to "1" to skip the MSBuild step and only
#                                package an already built output directory

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

INDIGO_BUILD_CONFIGURATION="${1:-${INDIGO_BUILD_CONFIGURATION:-Release}}"
INDIGO_BUILD_PLATFORM="${2:-${INDIGO_BUILD_PLATFORM:-x64}}"
export INDIGO_BUILD_CONFIGURATION
export INDIGO_BUILD_PLATFORM

# Pick up the INDIGO version/build number from the top-level Makefile so the
# installer file name and "Add/Remove Programs" entry stay in sync.
export INDIGO_VERSION="${INDIGO_VERSION:-$(grep -m1 '^INDIGO_VERSION' "$REPO_ROOT/Makefile" | sed 's/^INDIGO_VERSION *= *//')}"
export INDIGO_BUILD="${INDIGO_BUILD:-$(grep -m1 '^INDIGO_BUILD' "$REPO_ROOT/Makefile" | sed 's/^INDIGO_BUILD *= *//')}"
export INDIGO_PRERELEASE="${INDIGO_PRERELEASE:-$(grep -m1 '^INDIGO_PRERELEASE' "$REPO_ROOT/Makefile" | sed 's/^INDIGO_PRERELEASE *= *//')}"

if [ -n "$INDIGO_PRERELEASE" ]; then
    INDIGO_PACKAGE_BUILD="$INDIGO_BUILD~$INDIGO_PRERELEASE"
else
    INDIGO_PACKAGE_BUILD="$INDIGO_BUILD"
fi

echo "==> Packaging INDIGO $INDIGO_VERSION-$INDIGO_PACKAGE_BUILD ($INDIGO_BUILD_CONFIGURATION|$INDIGO_BUILD_PLATFORM)"

if [ "${SKIP_BUILD:-0}" != "1" ]; then
    echo "==> Locating MSBuild"
    if [ -z "$MSBUILD" ]; then
        VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
        if [ -x "$VSWHERE" ]; then
            VS_PATH="$("$VSWHERE" -latest -requires Microsoft.Component.MSBuild -property installationPath | tr -d '\r')"
            if [ -n "$VS_PATH" ]; then
                MSBUILD="$VS_PATH/MSBuild/Current/Bin/MSBuild.exe"
            fi
        fi
    fi
    if [ -z "$MSBUILD" ] && command -v msbuild >/dev/null 2>&1; then
        MSBUILD="$(command -v msbuild)"
    fi
    if [ -z "$MSBUILD" ] || [ ! -f "$MSBUILD" ]; then
        echo "error: couldn't find MSBuild.exe." >&2
        echo "       Run this from a 'Developer Command Prompt/PowerShell', or set" >&2
        echo "       MSBUILD=/path/to/MSBuild.exe and re-run this script." >&2
        exit 1
    fi

    echo "==> Building indigo_windows.sln with $MSBUILD"
    "$MSBUILD" "$REPO_ROOT/indigo_windows.sln" \
        "/p:Configuration=$INDIGO_BUILD_CONFIGURATION" \
        "/p:Platform=$INDIGO_BUILD_PLATFORM" \
        "/m"
else
    echo "==> SKIP_BUILD=1, not (re)building the solution"
fi

BUILD_DIR="$REPO_ROOT/build/$INDIGO_BUILD_CONFIGURATION/$INDIGO_BUILD_PLATFORM"
if [ ! -d "$BUILD_DIR" ]; then
    echo "error: build output directory not found: $BUILD_DIR" >&2
    exit 1
fi

# Generate the "indigo_drivers" metadata file (name/description of every
# indigo_*.dll driver/agent next to indigo_server.exe). indigo_server looks
# for this file next to its own executable on Windows (the equivalent of
# /usr/share/indigo/indigo_drivers on Linux), and uses it to populate the
# list of available drivers without having to load every DLL up front.
if [ -x "$BUILD_DIR/make_indigo_drivers.exe" ]; then
    echo "==> Generating indigo_drivers metadata file"
    (cd "$BUILD_DIR" && ./make_indigo_drivers.exe)
else
    echo "warning: $BUILD_DIR/make_indigo_drivers.exe not found, skipping" >&2
    echo "         driver metadata generation (indigo_server won't be able" >&2
    echo "         to list available drivers)." >&2
fi

echo "==> Locating Inno Setup compiler (ISCC.exe)"
if [ -z "$ISCC" ]; then
    for candidate in \
        "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" \
        "/c/Program Files/Inno Setup 6/ISCC.exe"; do
        if [ -x "$candidate" ]; then
            ISCC="$candidate"
            break
        fi
    done
fi
if [ -z "$ISCC" ] && command -v iscc >/dev/null 2>&1; then
    ISCC="$(command -v iscc)"
fi
if [ -z "$ISCC" ]; then
    echo "error: couldn't find ISCC.exe (Inno Setup 6)." >&2
    echo "       Install it from https://jrsoftware.org/isinfo.php, or set" >&2
    echo "       ISCC=/path/to/ISCC.exe and re-run this script." >&2
    exit 1
fi

mkdir -p "$REPO_ROOT/dist"

echo "==> Building installer with $ISCC"
"$ISCC" "$REPO_ROOT/indigo_windows/installer/indigo.iss"

echo "==> Done. Installer written to $REPO_ROOT/dist/"

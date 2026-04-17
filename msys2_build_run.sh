#!/bin/bash
#
# Build k_edit via MSYS2 UCRT64 toolchain, from any bash shell.
#
# Usage:
#   ./msys2_build_run.sh              # equivalent to `make -j`
#   ./msys2_build_run.sh zap          # or any other make target/args
#   ./msys2_build_run.sh -j clean all # args forwarded to make
#
# Inside a real MSYS2 UCRT64 shell, invoke `make` directly and skip this
# script. It exists to make the build work from non-MSYS2 bash shells
# (e.g. Git-for-Windows bash) by:
#   1. Re-executing itself under MSYS2 UCRT64 bash so MSYSTEM=UCRT64.
#   2. Setting TMP/TEMP/TMPDIR to a Windows-style path - native gcc.exe
#      doesn't understand MSYS-style /tmp.

set -e

if [ "$MSYSTEM" != "UCRT64" ]; then
    # Resolve to absolute path before re-exec: MSYS2 login bash cd's to $HOME.
    exec env MSYSTEM=UCRT64 /c/msys64/usr/bin/bash -l "$(readlink -f "$0")" "$@"
fi

cd "$(dirname "$(readlink -f "$0")")"

# Native Windows gcc.exe can't resolve MSYS-style /tmp, so resolve to a
# Windows-style forward-slash path via cygpath -m (MSYS2's login profile
# strips $LOCALAPPDATA, so we can't rely on that).
WIN_TMP="$(cygpath -m /tmp)"
export TMP="$WIN_TMP"
export TEMP="$WIN_TMP"
export TMPDIR="$WIN_TMP"

if [ $# -eq 0 ]; then
    exec make -j
else
    exec make "$@"
fi

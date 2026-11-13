#!/bin/sh
target="${DESTDIR}$1"
if ! command -v setcap >/dev/null 2>&1 || ! setcap cap_sys_nice+ep "$target" 2>/dev/null; then
    echo "WARNING: could not give $target cap_sys_nice; as root, run: setcap cap_sys_nice+ep $1" >&2
fi

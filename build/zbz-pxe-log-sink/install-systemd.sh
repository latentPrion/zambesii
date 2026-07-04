#!/bin/sh
set -e

BINDIR="${1:-/usr/local/bin}"
UNITDIR="${2:-/etc/systemd/system}"
SRCDIR="$(cd "$(dirname "$0")" && pwd)"

install -d -m 755 /var/log/zbz-kernel
install -m 755 "$SRCDIR/zbz-pxe-log-sink" "$BINDIR/zbz-pxe-log-sink"
sed "s|/usr/local/bin/zbz-pxe-log-sink|$BINDIR/zbz-pxe-log-sink|g" \
	"$SRCDIR/zbz-pxe-log-sink.service" > /tmp/zbz-pxe-log-sink.service
install -m 644 /tmp/zbz-pxe-log-sink.service "$UNITDIR/zbz-pxe-log-sink.service"
rm -f /tmp/zbz-pxe-log-sink.service
systemctl daemon-reload
systemctl enable --now zbz-pxe-log-sink
systemctl status --no-pager zbz-pxe-log-sink

#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

VM_NAME="zbz-i586-ibmPc-ibmPcBios"
OUT_FILE="${REPO_ROOT}/b/vbox.out.tmp"
RUN_SECONDS=15
TAIL_LINES=120
WAIT_OUTPUT=4
VBOX_WAIT=10

STARTED_VM=0
RESTORE_UART=0
ORIG_UART1=""
ORIG_UARTMODE1=""

usage()
{
	cat <<EOF
Usage: $0 [options]

Options:
  --vm NAME         VirtualBox VM name. Default: ${VM_NAME}
  --out PATH        Serial output file. Default: ${OUT_FILE}
  --seconds N       Runtime before forced poweroff. Default: ${RUN_SECONDS}
  --tail N          Number of log lines to tail after capture. Default: ${TAIL_LINES}
  --wait-output N   Seconds to wait for serial bytes after poweroff. Default: ${WAIT_OUTPUT}
  --vbox-wait N     Seconds to wait for VBoxManage IPC to become ready. Default: ${VBOX_WAIT}
  -h, --help        Show this help.
EOF
}

get_vm_value()
{
	local key="$1"

	VBoxManage showvminfo "${VM_NAME}" --machinereadable \
		| awk -F= -v key="${key}" '$1 == key { gsub(/"/, "", $2); print $2; exit; }'
}

wait_for_vm_state()
{
	local desired_state="$1"
	local timeout_seconds="$2"
	local state=""

	for ((i=0; i < timeout_seconds * 2; i++))
	do
		state="$(get_vm_value VMState)"
		if [[ "${state}" == "${desired_state}" ]]; then
			return 0
		fi

		sleep 0.5
	done

	return 1
}

restore_uart_settings()
{
	local io_addr irq_no mode_kind mode_arg

	if (( ! RESTORE_UART )); then
		return
	fi

	if [[ -z "${ORIG_UART1}" || "${ORIG_UART1}" == "off" ]]; then
		VBoxManage modifyvm "${VM_NAME}" --uart1 off >/dev/null
		return
	fi

	IFS=, read -r io_addr irq_no <<< "${ORIG_UART1}"
	VBoxManage modifyvm "${VM_NAME}" --uart1 "${io_addr}" "${irq_no}" >/dev/null

	if [[ -z "${ORIG_UARTMODE1}" ]]; then
		return
	fi

	mode_kind="${ORIG_UARTMODE1%%,*}"
	if [[ "${ORIG_UARTMODE1}" == *,* ]]; then
		mode_arg="${ORIG_UARTMODE1#*,}"
	else
		mode_arg=""
	fi

	case "${mode_kind}" in
	file)
		VBoxManage modifyvm "${VM_NAME}" --uartmode1 file "${mode_arg}" >/dev/null
		;;
	disconnected)
		VBoxManage modifyvm "${VM_NAME}" --uartmode1 disconnected >/dev/null
		;;
	server)
		VBoxManage modifyvm "${VM_NAME}" --uartmode1 server "${mode_arg}" >/dev/null
		;;
	client)
		VBoxManage modifyvm "${VM_NAME}" --uartmode1 client "${mode_arg}" >/dev/null
		;;
	tcpserver)
		VBoxManage modifyvm "${VM_NAME}" --uartmode1 tcpserver "${mode_arg}" >/dev/null
		;;
	tcpclient)
		VBoxManage modifyvm "${VM_NAME}" --uartmode1 tcpclient "${mode_arg}" >/dev/null
		;;
	*)
		echo "warning: unhandled original uartmode1 '${ORIG_UARTMODE1}', leaving current serial mode in place." >&2
		;;
	esac
}

cleanup()
{
	if (( STARTED_VM )); then
		VBoxManage controlvm "${VM_NAME}" poweroff >/dev/null 2>&1 || true
		wait_for_vm_state poweroff 20 || true
	fi

	restore_uart_settings || true
}

while [[ $# -gt 0 ]]
do
	case "$1" in
		--vm)
			VM_NAME="$2"
			shift 2
			;;
		--out)
			OUT_FILE="$2"
			shift 2
			;;
		--seconds)
			RUN_SECONDS="$2"
			shift 2
			;;
		--tail)
			TAIL_LINES="$2"
			shift 2
			;;
		--wait-output)
			WAIT_OUTPUT="$2"
			shift 2
			;;
		--vbox-wait)
			VBOX_WAIT="$2"
			shift 2
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown argument: $1" >&2
			usage >&2
			exit 1
			;;
	esac
done

trap cleanup EXIT INT TERM

if ! command -v VBoxManage >/dev/null 2>&1; then
	echo "VBoxManage not found." >&2
	exit 1
fi

ensure_vbox_ready()
{
	local waited=0

	while (( waited < VBOX_WAIT )); do
		if VBoxManage list vms >/dev/null 2>&1; then
			return 0
		fi
		sleep 1
		waited=$(( waited + 1 ))
	done

	return 1
}

if ! ensure_vbox_ready; then
	echo "VBoxManage IPC not ready after ${VBOX_WAIT}s." >&2
	exit 1
fi

if [[ "$(get_vm_value VMState)" != "poweroff" ]]; then
	echo "VM '${VM_NAME}' must be powered off before running this harness." >&2
	exit 1
fi

ORIG_UART1="$(get_vm_value uart1)"
ORIG_UARTMODE1="$(get_vm_value uartmode1)"

mkdir -p "$(dirname "${OUT_FILE}")"
: > "${OUT_FILE}"

if [[ "${ORIG_UART1}" != "0x03f8,4" || "${ORIG_UARTMODE1}" != "file,${OUT_FILE}" ]]; then
	VBoxManage modifyvm "${VM_NAME}" \
		--uart1 0x3F8 4 \
		--uartmode1 file "${OUT_FILE}" >/dev/null
	RESTORE_UART=1
fi

echo "Starting VM '${VM_NAME}' headlessly for ${RUN_SECONDS}s..."
if ! ensure_vbox_ready; then
	echo "VBoxManage IPC not ready after ${VBOX_WAIT}s." >&2
	exit 1
fi
VBoxManage startvm "${VM_NAME}" --type headless >/dev/null
STARTED_VM=1

sleep "${RUN_SECONDS}"

VBoxManage controlvm "${VM_NAME}" poweroff >/dev/null
wait_for_vm_state poweroff 20 || {
	echo "VM '${VM_NAME}' did not power off within timeout." >&2
	exit 1
}
STARTED_VM=0

restore_uart_settings
RESTORE_UART=0

size=0
waited=0
while (( waited < WAIT_OUTPUT )); do
	size="$(stat -c %s "${OUT_FILE}" 2>/dev/null || echo 0)"
	if [[ "${size}" != "0" ]]; then
		break
	fi
	sleep 1
	waited=$(( waited + 1 ))
done

echo "Serial log: ${OUT_FILE} (bytes=${size})"
if [[ "${size}" == "0" ]]; then
	echo "warning: serial log still empty after ${WAIT_OUTPUT}s; UART settings:"
	VBoxManage showvminfo "${VM_NAME}" --machinereadable | rg '^uart1=|^uartmode1=' || true
else
	tail -n "${TAIL_LINES}" "${OUT_FILE}"
fi

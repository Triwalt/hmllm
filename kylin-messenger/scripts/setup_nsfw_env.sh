#!/usr/bin/env bash
set -euo pipefail

PY_VER="${PY_VER:-python3}"
VENV_PATH="${VENV_PATH:-$(cd "$(dirname "$0")/../.." && pwd)/.venv-nsfw}"
MODEL_DIR="${MODEL_DIR:-$(cd "$(dirname "$0")/.." && pwd)/models}"
MODEL_NAME="${MODEL_NAME:-nsfw_mobilenet2.224x224.h5}"

echo "[NSFW] Creating venv at ${VENV_PATH}"
"${PY_VER}" -m venv "${VENV_PATH}"

VENV_PY="${VENV_PATH}/bin/python"
"${VENV_PY}" -m pip install --upgrade pip

REQ_FILE="$(cd "$(dirname "$0")/.." && pwd)/python/requirements.linux.txt"
echo "[NSFW] Installing requirements (Linux) from ${REQ_FILE}"
"${VENV_PY}" -m pip install -r "${REQ_FILE}"

mkdir -p "${MODEL_DIR}"
MODEL_PATH="${MODEL_DIR}/${MODEL_NAME}"
if [[ ! -f "${MODEL_PATH}" ]]; then
  echo "[NSFW] Downloading model to ${MODEL_PATH}"
  curl -L -o "${MODEL_PATH}" \
    "https://github.com/GantMan/nsfw_model/releases/download/1.1.0/${MODEL_NAME}"
fi

echo "[NSFW] Export environment variables (add to your shell profile as needed)"
echo "export KYLIN_NSFW_PYTHON='${VENV_PY}'"
echo "export KYLIN_NSFW_MODEL='${MODEL_PATH}'"
echo "export KYLIN_NSFW_BACKEND='python'"

echo "[NSFW] Done. Restart the app to take effect."



#!/usr/bin/env bash
set -euo pipefail

# Windows installer builder via Docker (cross-compile with MinGW and NSIS)

# Defaults
PRODUCT_NAME="Xpad"
COMPANY_NAME="Xpad Project"
OUT_DIR="dist-win"
DOCKERFILE="Dockerfile.win"
BUILD_TARGET="artifact"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${SCRIPT_DIR}"

usage() {
  cat <<EOF
Usage: $0 [options]

Options:
  -n, --name <name>        Product name (default: ${PRODUCT_NAME})
  -c, --company <name>     Company name (default: ${COMPANY_NAME})
  -v, --version <ver>      Version override (default: parsed from configure.ac)
  -o, --out <dir>          Output directory (default: ${OUT_DIR})
  -f, --dockerfile <file>  Dockerfile path (default: ${DOCKERFILE})
  -h, --help               Show this help

Produces: <out>/out/<Product>-<Version>-Setup.exe
EOF
}

VERSION=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    -n|--name)
      PRODUCT_NAME="$2"; shift 2;;
    -c|--company)
      COMPANY_NAME="$2"; shift 2;;
    -v|--version)
      VERSION="$2"; shift 2;;
    -o|--out)
      OUT_DIR="$2"; shift 2;;
    -f|--dockerfile)
      DOCKERFILE="$2"; shift 2;;
    -h|--help)
      usage; exit 0;;
    *)
      echo "Unknown option: $1" >&2; usage; exit 1;;
  esac
done

if ! command -v docker >/dev/null 2>&1; then
  echo "Error: docker not found in PATH" >&2
  exit 1
fi

if [[ -z "${VERSION}" ]]; then
  if [[ -f "${REPO_ROOT}/configure.ac" ]]; then
    VERSION=$(sed -n 's/AC_INIT(\[Xpad\],\[\([^]]*\)\].*/\1/p' "${REPO_ROOT}/configure.ac" | head -n1 || true)
  fi
  if [[ -z "${VERSION}" ]]; then
    # Fallback to git tag if available
    if command -v git >/dev/null 2>&1 && git -C "${REPO_ROOT}" rev-parse --git-dir >/dev/null 2>&1; then
      VERSION=$(git -C "${REPO_ROOT}" describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || true)
    fi
  fi
fi

if [[ -z "${VERSION}" ]]; then
  echo "Error: could not determine version. Pass --version or ensure configure.ac has AC_INIT with a version." >&2
  exit 1
fi

OUT_ABS="${REPO_ROOT}/${OUT_DIR}"
mkdir -p "${OUT_ABS}"

echo "Building Windows installer via Docker"
echo " - Product     : ${PRODUCT_NAME}"
echo " - Company     : ${COMPANY_NAME}"
echo " - Version     : ${VERSION}"
echo " - Output dir  : ${OUT_ABS}"
echo " - Dockerfile  : ${DOCKERFILE}"

export DOCKER_BUILDKIT=1

docker build \
  -f "${REPO_ROOT}/${DOCKERFILE}" \
  --target "${BUILD_TARGET}" \
  --build-arg PRODUCT_NAME="${PRODUCT_NAME}" \
  --build-arg PRODUCT_VERSION="${VERSION}" \
  --build-arg COMPANY_NAME="${COMPANY_NAME}" \
  --output type=local,dest="${OUT_ABS}" \
  "${REPO_ROOT}"

echo
echo "Artifacts generated in: ${OUT_ABS}/out"
find "${OUT_ABS}/out" -maxdepth 1 -type f -name "*Setup.exe" -print || true
echo "Done."



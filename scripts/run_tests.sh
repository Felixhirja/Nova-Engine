#!/usr/bin/env bash
set -euo pipefail

# Determine the repository root relative to this script and switch to it.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

if ! command -v make >/dev/null 2>&1; then
  echo "Error: make is required to run the test suite." >&2
  exit 1
fi

# Build and run the automated tests using the existing Makefile target.
make test

#!/usr/bin/env bash
set -euo pipefail

script_dir="$(dirname "$0")"
compose_file="$script_dir/../deploy/docker-compose.yml"
env_file="$script_dir/../deploy/.env"

env_args=()
if [ -f "$env_file" ]; then
  env_args+=(--env-file "$env_file")
fi

echo "[+] Building containers"
docker compose "${env_args[@]}" -f "$compose_file" build

echo "[+] Starting stack"
docker compose "${env_args[@]}" -f "$compose_file" up -d

docker compose "${env_args[@]}" -f "$compose_file" ps

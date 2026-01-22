#!/usr/bin/env bash
set -euo pipefail
echo "[1/3] Backend: mvn verify -DskipTests"
(cd backend && mvn -q verify -DskipTests)
echo "[2/3] Frontend: npm ci"
cd frontend && npm ci >/dev/null
echo "[3/3] Frontend lint + type-check"
npm run lint -- --quiet
npm run type-check
echo "CI checks passed."

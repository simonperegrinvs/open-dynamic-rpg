#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

ue_root="${UE_ROOT:-/Volumes/UE_5_8_APFS}"
if [[ ! -x "$ue_root/Engine/Build/BatchFiles/Mac/Build.sh" ]]; then
  echo "Unreal Engine 5.8 Mac build tools not found at $ue_root" >&2
  exit 2
fi

cmake --preset ue-release
cmake --build --preset ue-release -j 4
python3 tools/stage_content.py
python3 tools/stage_content.py --check

"$ue_root/Engine/Build/BatchFiles/Mac/Build.sh" \
  OpenDynamicRPGEditor Mac Development \
  -project="$repo_root/Unreal/OpenDynamicRPG.uproject" \
  -waitmutex -NoHotReload

"$ue_root/Engine/Binaries/Mac/UnrealEditor-Cmd" \
  "$repo_root/Unreal/OpenDynamicRPG.uproject" \
  -unattended -nullrhi -nosplash \
  -ExecCmds='Automation RunTests ODR.BlacksmithMine;Quit' \
  -ReportExportPath="$repo_root/build/reports/unreal"
python3 tools/check_unreal_report.py

"$ue_root/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$repo_root/Unreal/OpenDynamicRPG.uproject" \
  -platform=Mac -clientconfig=Development \
  -build -cook -stage -pak -package -archive \
  -archivedirectory="$repo_root/build/package" \
  -unattended -nop4

"$repo_root/build/package/Mac/OpenDynamicRPG.app/Contents/MacOS/OpenDynamicRPG" \
  -odrsmoke -unattended -nullrhi -nosplash -stdout -FullStdOutLogOutput \
  > "$repo_root/build/packaged-smoke.log" 2>&1
rg 'ODR_SMOKE_PASS' "$repo_root/build/packaged-smoke.log"

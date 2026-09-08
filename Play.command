#!/bin/zsh
project_dir="${0:A:h}"
engine_root="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
engine_app="$engine_root/Engine/Binaries/Mac/UnrealEditor.app"
if [[ ! -d "$engine_app" ]]; then
  print 'Unreal Engine не найден. Задайте UE_ROOT — путь к папке UE_5.8.'
  exit 1
fi
open -n -a "$engine_app" --args "$project_dir/Altai.uproject" -game -windowed -ResX=1280 -ResY=720 -NoSplash "-abslog=$project_dir/Saved/Logs/AltaiStandalone.log"

#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: bindings/GenerateBindings.sh <language> [+xfeat ...] [--library NAME] [--output DIR]

Generates SWIG bindings from bindings/On1x.i. The default library name is
"on1x"; enabled XFeats must be listed in bindings/XFeats.yaml and support the
selected language.
EOF
}

if [[ $# -lt 1 ]]; then usage >&2; exit 2; fi

language=$1
shift
library=on1x
output=
features=()
while [[ $# -gt 0 ]]; do
  case $1 in
    +*) features+=("${1#+}") ;;
    --library) shift; [[ $# -gt 0 ]] || { usage >&2; exit 2; }; library=$1 ;;
    --output) shift; [[ $# -gt 0 ]] || { usage >&2; exit 2; }; output=$1 ;;
    --help|-h) usage; exit 0 ;;
    *) printf 'Unknown argument: %s\n' "$1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

case $language in
  Python) swig_language=python ;;
  Java) swig_language=java ;;
  CSharp|C#) swig_language=csharp ;;
  Ruby) swig_language=ruby ;;
  Lua) swig_language=lua ;;
  Go) swig_language=go ;;
  PHP) swig_language=php ;;
  Perl) swig_language=perl5 ;;
  R) swig_language=r ;;
  D) swig_language=d ;;
  JavaScript) swig_language=javascript ;;
  *) printf 'Unsupported SWIG language: %s\n' "$language" >&2; exit 2 ;;
esac

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
manifest="$root/bindings/XFeats.yaml"
interface="$root/bindings/On1x.i"
command -v swig >/dev/null || { echo "swig is required" >&2; exit 127; }

for feature in "${features[@]}"; do
  if ! awk -v wanted="$feature" -v language="$language" '
    $1 == "-" && $2 == "id:" {
      current = $3; gsub(/"/, "", current)
    }
    current == wanted && $1 == "languages:" {
      line = $0
      if (line ~ ("\"" language "\"")) found = 1
    }
    END { exit found ? 0 : 1 }
  ' "$manifest"; then
    printf 'XFeat "%s" is unknown or unsupported for %s\n' "$feature" "$language" >&2
    exit 2
  fi
done

if [[ -z $output ]]; then output="$root/bindings/generated/$language"; fi
mkdir -p "$output"
defines=()
for feature in "${features[@]}"; do
  macro=$(printf '%s' "$feature" | tr '[:lower:]-' '[:upper:]_')
  defines+=("-DSWIGON1X_XFEAT_${macro}")
done
extra=()
for feature in "${features[@]}"; do
  [[ $feature == py-builtin && $language == Python ]] && extra+=("-builtin")
  [[ $feature == directors ]] && extra+=("-directors")
done

swig "-$swig_language" -I"$root/include" -outdir "$output" -o "$output/${library}_wrap.cxx" \
  -module "$library" "${defines[@]}" "${extra[@]}" "$interface"
printf 'Generated %s bindings in %s\n' "$language" "$output"

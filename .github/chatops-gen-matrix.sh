#!/usr/bin/env bash
set -euo pipefail
shopt -s lastpipe

echowarn(){
   echo >&2 '::warning::'"$@"
}
echoerr(){
   echo >&2 '::error::'"$@"
}

JSON_full='{}'
JSON_ubuntu='{}'
JSON_macos='{}'
json_edit(){
    declare -n json=JSON_$1
    shift
    json="$(echo "$json" | jq "$@" || echo "$json")"
}
json_include(){
   local os=$2 c=$3 b=$4 o=$5 use=
   local use_from_o
   json_edit $1 ".include+=[{
         os:\"$os\",
         cc:\"$c\",
         build_type:\"$b\",
         CMAKE_USE:\"$use\",
         dockerpush: \"${dockerpush:-yes}\"
   }]"
}
show_human_json(){
    declare -n json=JSON_$1
    echo "$1:"
    echo "$json" | jq
}

readonly DEFAULT_os="ubuntu macos" DEFAULT_build_type="Debug" DEFAULT_cmake_opts="default burrow ursa"
readonly DEFAULT_ubuntu_compiler="gcc-9" AVAILABLE_ubuntu_compiler="gcc-9 gcc-10 clang-10"
readonly DEFAULT_macos_compiler="clang"  AVAILABLE_macos_compiler="clang llvm gcc-10"
readonly ALL_os="ubuntu macos" ALL_build_type="Debug Release" ALL_cmake_opts="default burrow ursa" ALL_compiler="gcc-9 gcc-10 clang-10 clang llvm"

use_from_o(){
   if [[ $o = default ]] ;then
      use=''
   else
      use=-DUSE_${o^^}=ON
   fi
}

generate(){
   declare -rn DEFAULT_compiler=DEFAULT_$1_compiler
   declare -rn AVAILABLE_compiler=AVAILABLE_$1_compiler
   declare -n MATRIX=MATRIX_$1

   if [[ " $os " = *" $1 "* ]] ;then
      cc=${compiler:-$DEFAULT_compiler}
      local c b o
      for c in $cc ;do
         if ! [[ " $AVAILABLE_compiler " = *" $c "* ]] ;then
            c=
            continue
         fi
         for b in $build_type ;do
            for o in $cmake_opts ;do
               MATRIX+="$1 $c $b $o"$'\n'
            done
         done
      done
      if test "${c:-}" = "" -a "${MATRIX:-}" = ""; then
         echoerr "No available compiler for '$1' among '$cc', available: '$AVAILABLE_compiler'"
      fi
   fi
}
# gen_json(){
#    local use o=$4; use_from_o
#    json_edit $1 ".include+=[{
#          cc:\"$2\",
#          build_type:\"$3\",
#          CMAKE_USE:\"$use\",
#          dockerpush: \"${dockerpush:-yes}\"
#       }]"
# }

handle_line(){
   # echo ----------- "$@"
   if [[ "${1:-}" != '/build' ]] ;then
      echowarn "Line skipped, should start with '/build' or '/test'"
      return
   fi
   shift
   local os compiler cmake_opts build_type dockerpush=yes
   while [[ $# > 0 ]] ;do
      case "$1" in
         macos)                     os+=" $1 " ;;
         ubuntu|linux)              os+=" ubuntu " ;;
         windows)                   os+=" $1 " ;;
         default)                   cmake_opts+=" $1 "  ;;
         burrow)                    cmake_opts+=" $1 "  ;;
         ursa)                      cmake_opts+=" $1 "  ;;
         release|Release)           build_type+=" Release " ;;
         debug|Debug)               build_type+=" Debug"  ;;
         gcc-9|gcc9)                compiler+=" gcc-9 " ;;
         gcc-10|gcc10)              compiler+=" gcc-10 " ;;
         clang-10|clang10)          compiler+=" clang-10"  ;;
         llvm)                      compiler+=" $1 " ;;
         clang)                     compiler+=" $1 " ;;
         # msvc)                      compiler+=" $1 " ;;
         # mingw)                     compiler+=" $1 " ;;
         # notest)  ;;
         # test)  ;;
         dockerpush)                dockerpush=yes ;;
         nodockerpush)              dockerpush=no ;;
         all|everything|before_merge|before-merge)
            os="$ALL_os" build_type="$ALL_build_type" cmake_opts="$ALL_cmake_opts" compiler="$ALL_compiler"
            ;;
         *)
            echoerr "Unknown /build argument '$1'"
            return 1
            ;;
      esac
      shift
   done

   os=${os:-$DEFAULT_os}
   build_type=${build_type:-$DEFAULT_build_type}
   cmake_opts=${cmake_opts:-$DEFAULT_cmake_opts}

   generate ubuntu
   generate macos
}

while read comment_line;do
   #  if [[ "$comment_line" =~ ^/build\ .* ]] ;then
        handle_line $comment_line #|| continue
   #  fi
done

MATRIX_ubuntu=${MATRIX_ubuntu:=}
MATRIX_macos=${MATRIX_macos:=}
echo >&2 "$MATRIX_ubuntu$MATRIX_macos"----

echo "$MATRIX_ubuntu$MATRIX_macos" | sort -uV |
   while read line ;do
      if [[ "" = "$line" ]] ;then continue ;fi
      # gen_json $line
      json_include full $line
   done

# show_human_json ubuntu
# show_human_json macos
show_human_json full

echo "$JSON_full"   | jq -c >matrix_full
echo "$JSON_full"   | jq -c >matrix_ubuntu_release  '.include | map(select (.os=="ubuntu" and .build_type=="Release") )'
echo "$JSON_full"   | jq -c >matrix_ubuntu_debug    '.include | map(select (.os=="ubuntu" and .build_type=="Debug") )'
echo "$JSON_ubuntu" | jq -c >matrix_ubuntu
echo "$JSON_macos"  | jq -c >matrix_macos

#ifndef __CREATEUTILS_SH_H
#define __CREATEUTILS_SH_H

#include "enums.h"
#include "params.h"
#include "utils.h"
#include "generate.h"

void generate_utils_sh(const std::string & supportpath)
{
   std::string vdata = R"EOF(#!/bin/bash
# --- some useful utility functions
# --- generally should be okay if used with 'set -e'.

# Formatting for comamnds - standardised.
[ -v ECODE ] || readonly ECODE=$(printf "\e")
[ -v CODE_S ] || readonly CODE_S="$ECODE[32m"
[ -v CODE_E ] || readonly CODE_E="$ECODE[0m"

#------------------------------------------------------------------------------------
# die MSG [EXITCODE] - show the message (red) and exit exitcode.

function die {
   echo " ">&2 ; echo -e "\e[31m\e[1m${1}\e[0m">&2  ; echo " ">&2
   EXITCODE=${2:-1}
   exit "$EXITCODE"
}

#------------------------------------------------------------------------------------
# See if a container is exists. Return value of docker inspect is correct (0 if running)
# https://gist.github.com/ekristen/11254304

function container_exists {
   [ $# -eq 1 ] || die "container_exists requires containername parameter."
   # exit code of docker inspect says if it's running. Don't let the line fail though (in case we have set -e)
   docker inspect --format="{{ .State.Running }}" "$1" >/dev/null 2>&1 || return 1
}

#------------------------------------------------------------------------------------
# See if a container is running

function container_running {
   [ $# -eq 1 ] || die "container_running requires containername parameter."
   container_exists "$1" && [ $(docker inspect --format="{{ .State.Running }}" "$1" 2>/dev/null) = "true" ]
}

#------------------------------------------------------------------------------------
# See if a container is paused

function container_paused {
   [ $# -eq 1 ] || die "container_paused requires containername parameter."
   container_exists "$1" && [ $(docker inspect --format="{{ .State.Paused }}" "$1" 2>/dev/null) = "true" ]
}

#------------------------------------------------------------------------------------
# Import directory into container. utils_import source-localpath dest-containerpath

function utils_import_DEPRECATED {
   [ "$#" -eq 2 ] || die "utils_import -- requires two arguments (the path to be imported and the container's destination path)."
   [ -d "$1" ] || die "utils_import -- source path does not exist: $1"
   local SOURCEPATH=$(realpath "$1" | tr -d '\r\n')

         # docker run --name="${SERVICENAME}-importfn" "${DOCKEROPTS[@]}" "${IMAGENAME}" bash -c "rm -rf $2/*"
   tar cf - -C "$SOURCEPATH" . | docker run -i --name="${SERVICENAME}-importfn" "${DOCKEROPTS[@]}" "${IMAGENAME}" tar -xv -C "$2"
   RVAL=$?
   docker rm "${SERVICENAME}-importfn" >/dev/null
   [ $RVAL -eq 0 ] || die "utils_import failed to transfer the files."
}

#------------------------------------------------------------------------------------
# Export directory from container. utils_export source-containerpath dest-localpath

function utils_export_DEPRECATED {
   [ "$#" -eq 2 ] || die "utils_export -- requires two arguments (the container's source path and the path to be exported to)."
   [ -d "$2" ] || die "utils_export -- destination path does not exist."
   local DESTPATH=$(realpath "$2" | tr -d '\r\n')

         docker run -i --name="${SERVICENAME}-exportfn" "${DOCKEROPTS[@]}" "${IMAGENAME}" tar cf - -C "$1" . | tar -xv -C "$DESTPATH"
   RVAL=$?
   docker rm "${SERVICENAME}-exportfn" >/dev/null
   [ $RVAL -eq 0 ] || die "utils_export failed to transfer the files."
}


#------------------------------------------------------------------------------------
# Element is contained in an array

# elementIn element array
# if elementIn "a string" "${array[@]}" ; then ...
function elementIn {
  local e
  for e in "${@:2}"; do [[ "$e" == "$1" ]] && return 0; done
  return 1
}

#------------------------------------------------------------------------------------
# Command is one of the standard hooks provided by dRunner
# If you don't implement these make sure there is no output and you return 127.

function isHook {
   local HOOKS=("install_end" "backup_start" "backup_end" \
      "restore_start" "restore_end" "uninstall_start" "obliterate_start" \
      "servicecmd_start" "servicecmd_end" "update_start" "update_end" \
      "enter_start" "enter_end" "status_start" "status_end" )
   
   if elementIn "$1" "${HOOKS[@]}"; then
      return 0;
   fi
   return 1
}

#------------------------------------------------------------------------------------
# check whether a docker volume exists on the host.
# if volumeexists "$VOLNAME" ; then

function volumeexists {
  docker volume ls | grep "$1" > /dev/null
}

#------------------------------------------------------------------------------------
# Save an environment variable (configuration)
# Example:   save_environment "PORT" "25565"

function save_environment {
   [ "$#" -eq 2 ] || die "save_environment -- requires two arguments (the environment variable name and the content)."

   drunner __save-environment "$SERVICENAME" "$1" "$2" || die "${SERVICENAME} save_environment failed."

   # set the environment variable to the new value now.
   eval $1=\$2
}


)EOF";

   generate(supportpath+"/utils.sh",S_ALLREAD,vdata);
}

#endif

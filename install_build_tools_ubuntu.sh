#!/bin/bash

#
# Copyright 2015-2021 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
#
# This file is part of K.
#
# K is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# K is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along
# with K.  If not, see <http:#www.gnu.org/licenses/>.
#

# run to unilaterally install prereqs for building & running K

hdr="$ID K-prereq-install: "
complete() {
   local rv="$?"
   [ "$rv" -gt "0" ] && { echo "${hdr}FAILED" ; exit 1 ; }
   echo "to build & install universal-ctags: run ./ubuntu_univ-ctags_build_install"
   echo "${hdr}SUCCEEDED"
   exit 0
   }

. /etc/os-release

if [ "$ID" = "ubuntu" ] ; then
   echo "${hdr}STARTING"
   sudo apt-get install -yq   \
      build-essential         \
      g++                     \
      libboost-dev            \
      libboost-filesystem-dev \
      libboost-system-dev     \
      libncurses5-dev         \
      libpcre3-dev            \
      ncurses-term            \
      libreadline-dev         \
      xclip
   complete
fi

if [ "$ID" = "centos" ] ; then
   echo "${hdr}STARTING"
   yum -yq groupinstall "Development Tools"   &&
   yum -yq install boost-devel ncurses-devel pcre-devel ncurses-term readline-devel &&
   yum -yq install exuberant-ctags &&
   yum -yq install xclip
   complete
fi

echo "${hdr}FAILED: '$ID' unknown" ;
exit 1

# notes (not used): alternative yum version
yum groupinstall "Development Tools"
yum install libboost-devel
yum install libboost-filesystem-devel
yum install libboost-system-devel
yum install libncurses5-devel
yum install libpcre3-devel
yum install exuberant-ctags
yum install ncurses-term
yum install xclip

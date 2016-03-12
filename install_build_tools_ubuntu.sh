#!/bin/bash

#
# Copyright 2015 by Kevin L. Goodwin [fwmechanic@gmail.com]; All rights reserved
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

# gcc 4.9.x is the dflt version on *ubuntu 14.10, and seems to be
#           a major memory hog (I've observed this w/MinGW too)
# gcc 4.8.x is sufficient for our needs, and is much less of a
#           memory hog (thus runs faster on <= 2GB VMs)

hdr="$ID K-prereq-install: "
complete() {
   local rv="$?"
   [ "$rv" -gt "0" ] && { echo "${hdr}FAILED"    ; exit 1 ; }
                        { echo "${hdr}SUCCEEDED" ; exit 0 ; }
   }

. /etc/os-release

if [ "$ID" = "ubuntu" ] ; then
   echo "${hdr}STARTING"
   sudo apt-get install -yq   \
      build-essential         \
      libboost-dev            \
      libboost-filesystem-dev \
      libboost-system-dev     \
      libncurses5-dev         \
      libpcre3-dev            \
      exuberant-ctags         \
      ncurses-term            \
      xclip
   complete
fi

if [ "$ID" = "centos" ] ; then
   echo "${hdr}STARTING"
   yum groupinstall "Development Tools"   &&
   yum install libboost-devel             &&
   yum install libboost-filesystem-devel  &&
   yum install libboost-system-devel      &&
   yum install libncurses5-devel          &&
   yum install libpcre3-devel             &&
   yum install exuberant-ctags            &&
   yum install ncurses-term               &&
   yum install xclip
   complete
fi

echo "${hdr}FAILED: '$ID' unknown" ; exit 1

yum groupinstall "Development Tools"
yum install libboost-devel
yum install libboost-filesystem-devel
yum install libboost-system-devel
yum install libncurses5-devel
yum install libpcre3-devel
yum install exuberant-ctags
yum install ncurses-term
yum install xclip

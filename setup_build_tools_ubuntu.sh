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

# intended to be sourced: . install_build_tools.sh

# gcc 4.9.x is the dflt version on *ubuntu 14.10, and seems to be
#           a major memory hog (I've observed this w/MinGW too)
# gcc 4.8.x is sufficient for our needs, and is much less of a
#           memory hog (thus runs faster on <= 2GB VMs)

# modify the make rules to use these versions of gcc/g++
export CC=gcc-4.8
export CXX=g++-4.8

which $CC && which $CXX && return 0

install_build_tools_ubuntu.sh

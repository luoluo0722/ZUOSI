# Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

AM_CPPFLAGS      = @ZLIB_INCLUDES@ -I$(top_builddir)/include 
AM_CPPFLAGS     += -I$(top_srcdir)/include -I$(top_srcdir)/unittest/mytap
AM_CPPFLAGS     += -I$(top_srcdir)/sql

LDADD 		= $(top_builddir)/unittest/mytap/libmytap.a \
		  $(top_builddir)/mysys/libmysys.a \
		  $(top_builddir)/strings/libmystrings.a \
		  $(top_builddir)/dbug/libdbug.a \
		  $(top_builddir)/mysys/libmysys.a \
		  $(top_builddir)/strings/libmystrings.a

my_decimal_t_SOURCES = my_decimal-t.cc

noinst_PROGRAMS  = my_decimal-t

# Don't update the files from bitkeeper
%::SCCS/s.%

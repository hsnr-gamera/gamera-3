/*
 *
 * Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
  Many compilers (including gcc) don't have the standard header <limits> that
  (amoung other things) defines numeric_limits.  This header will include the
  standard header unless GAMERA_USE_BOOST_LIMITS macro is defined; if it is
  defined it will include the boost version that the boost developers created
  because they were so tired of compilers not including <limits>.

 */

#ifndef kwm03182002_gamera_limits
#define kwm03182002_gamera_limits

#ifdef __GNUC__
#if __GNUC__ < 3
#define GAMERA_USE_BOOST_LIMITS
#endif
#endif

#ifdef GAMERA_USE_BOOST_LIMITS
#include "boost_limits.hpp"
#else
#include <limits>
#endif

#endif

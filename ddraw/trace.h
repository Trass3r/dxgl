// DXGL
// Copyright (C) 2013 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#pragma once
#ifndef _TRACE_H
#define _TRACE_H


#ifdef _TRACE
void TRACE_ENTER(const char *function, int argtype, void *arg, int end);
void TRACE_ARG(int argtype, void *arg, int end);
void TRACE_EXIT(const char *function, int argtype, void *arg);
#else
#define TRACE_ENTER(a,b,c,d)
#define TRACE_ARG(a,b,c)
#define TRACE_EXIT(a,b,c)
#endif

#endif //_TRACE_H
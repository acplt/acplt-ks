/* -*-plt-c++-*- */
/* $Header: /home/david/cvs/acplt/ks/include/ks/hostent.h,v 1.2 1998-01-12 07:49:24 harald Exp $ */

#ifndef KSC_HOSTENT_INCLUDED
#define KSC_HOSTENT_INCLUDED

/*
 * Copyright (c) 1996, 1997, 1998
 * Chair of Process Control Engineering,
 * Aachen University of Technology.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must print or display the above
 *    copyright notice either during startup or must have a means for
 *    the user to view the copyright notice.
 * 3. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the Chair of Process Control Engineering nor the
 *    name of the Aachen University of Technology may be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CHAIR OF PROCESS CONTROL ENGINEERING
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE CHAIR OF PROCESS CONTROL
 * ENGINEERING BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Author: Markus Juergens <markusj@plt.rwth-aachen.de> */

//////////////////////////////////////////////////////////////////////

#include <plt/array.h>

#include "ks/rpc.h"

//////////////////////////////////////////////////////////////////////

typedef u_long KSC_IP_TYPE;

class KscHostEnt 
{
public: 
  KscHostEnt(struct hostent * = 0);
  KscHostEnt(const KscHostEnt &);
  
  KscHostEnt &operator = (const KscHostEnt &);

  // query functions
  size_t numIP() const;
  KSC_IP_TYPE getIP(size_t) const;

protected:
  void copy_ips(struct hostent *);

  PltArray<KSC_IP_TYPE> ips;
};

//////////////////////////////////////////////////////////////////////
// INLINE IMPLEMENTATION
//////////////////////////////////////////////////////////////////////

inline
KscHostEnt::KscHostEnt(struct hostent *he)
{
  copy_ips(he);
}

//////////////////////////////////////////////////////////////////////

inline
KscHostEnt::KscHostEnt(const KscHostEnt &other)
  : ips(other.ips)
{}

//////////////////////////////////////////////////////////////////////

inline
KscHostEnt &
KscHostEnt::operator = (const KscHostEnt &other)
{
  ips = other.ips;
  return *this;
}

//////////////////////////////////////////////////////////////////////

inline size_t
KscHostEnt::numIP() const
{
  return ips.size();
}

//////////////////////////////////////////////////////////////////////

inline KSC_IP_TYPE
KscHostEnt::getIP(size_t i) const
{
  return ips[i];
}

//////////////////////////////////////////////////////////////////////

#endif


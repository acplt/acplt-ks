/* -*-plt-c++-*- */
#ifndef KS_XDR_INCLUDED
#define KS_XDR_INCLUDED
/* $Header: /home/david/cvs/acplt/ks/include/ks/xdr.h,v 1.7 1997-03-12 16:32:26 martin Exp $ */
/*
 * Copyright (c) 1996, 1997
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

#include "plt/debug.h"

extern "C" {
#include <rpc/xdr.h>
};


//////////////////////////////////////////////////////////////////////
// XDR streaming helper classes and macros
//////////////////////////////////////////////////////////////////////
//
// Intention:
// ==========
// - Objects should know best how to stream themselves.
// - Classes should construct objects directly from the stream
//   to avoid copying large data structures.
// - Constructors have to report failed deserialization via
//   a bool "return value" (== additional reference argument)
//   because exceptions are not generally available.
// - For those objects that are (de-)serialized by RPC there must
//   be extern "C" stubs that call C++ methods.
//
// Usage:
// ======
// Each XDR streamable class shoud derive from KsXdrAble and must define
// a1)   bool xdrEncode(XDR *) const;   // serialization
// a2)   bool xdrDecode(XDR *);         // deserialization
//
// b)    SomeClass::SomeClass(XDR *, bool & ok); // deserialize and construct
// c)    static SomeClass * SomeClass::xdrNew(XDR *); 
//          // factory method for b)
//
// a1) and a2) must be manually implemented
// b) can be implemented by executing KS_IMPL_XDRCTOR(SomeClass) if
//    SomeClass has a cheap default constructor
// c) can be implemented by executing KS_IMPL_XDRNEW(SomeClass); 
// 
// Class KsXdrUnion is an abstract base class for classes with some
// known derivations representing a XDR union. It defines a1) and a2)
// Each derived class has to 
// - define a a distinct "virtual constant" xdrTypeCode() const 
//   (the same value as the XDR union discriminator)
// - define xdrEncodeVariant(XDR *) const
// - define xdrDecodeVariant(XDR *)
// - define the deserialization constructor
// 
// The base class can define 
// - xdrEncodeCommon(XDR *) const 
// - xdrDecodeCommon(XDR*)
// to (de)serialize common state.
//
// The abstract base class definition body can be generated by expanding
//    KS_DECL_XDRUNION(Base);
//
// The definition of c) (s.a.) can be generated by expanding
//    KS_BEGIN_IMPL_XDRUNION( base );
//    KS_XDR_MAP( CONST1, Derived1 );
//    KS_XDR_MAP( CONST2, Derived2 );
//     ...
//    KS_END_IMPL_XDRUNION;
//
// The constructor b) is not needed (Base is abstract)
// a2) works only in some cases because you can't change an
//     object's class
//////////////////////////////////////////////////////////////////////
// See tests/txdr2.cpp for an example
//////////////////////////////////////////////////////////////////////

class KsXdrAble 
{
public:
    virtual bool xdrEncode(XDR *) const = 0;  // serialize
    virtual bool xdrDecode(XDR *) = 0;        // deserialize
 // static KsXdrUnion * xdrNew(XDR *) = 0; // deserialization factory meth.
};

//////////////////////////////////////////////////////////////////////

class KsXdrUnion 
: public KsXdrAble 
{
public:
    virtual bool xdrEncode(XDR *) const;
    virtual bool xdrDecode(XDR *) ;
//  static KsXdrUnion * xdrNew(XDR *) = 0;
    virtual enum_t xdrTypeCode() const = 0; // XDR union discriminator 
protected:
    virtual bool xdrEncodeCommon(XDR *) const;       // serialize common part
    virtual bool xdrDecodeCommon(XDR *);             // deser.    common part
    virtual bool xdrEncodeVariant(XDR *) const = 0;
    virtual bool xdrDecodeVariant(XDR *) = 0;
};

//////////////////////////////////////////////////////////////////////

#define KS_IMPL_XDR_C(classname)                  \
    extern "C" bool_t                             \
    xdr_##classname(XDR *xdrs, classname **pp)    \
    {                                             \
        PLT_PRECONDITION( xdrs && pp );           \
        switch(xdrs->x_op) {                      \
                                                  \
        case XDR_FREE:                            \
            if (!*pp) {                           \
                delete *pp;                       \
                *pp = 0;                          \
                return TRUE;                      \
            }                                     \
        case XDR_ENCODE:                          \
            if (! *pp) return FALSE;              \
            return (*pp)->xdrEncode(xdrs);        \
                                                  \
        case XDR_DECODE:                          \
            *pp = classname::xdrNew(xdrs);        \
            return *pp != 0;                      \
                                                  \
        default:                                  \
            return FALSE;                         \
        }                                         \
    }                                             \
    typedef void ks_dummy_typedef

//////////////////////////////////////////////////////////////////////

#define KS_DECL_XDRUNION(base)                                    \
    public:                                                       \
    static base * xdrNew(XDR *);                                  \
    virtual enum_t xdrTypeCode() const = 0; /*redeclaration*/     \
    protected:                                                    \
    virtual bool xdrEncodeVariant(XDR *) const = 0; /*redecl.*/   \
    virtual bool xdrDecodeVariant(XDR *) = 0 /*;     *redecl.*/          

//////////////////////////////////////////////////////////////////////

#define KS_BEGIN_IMPL_XDRUNION(base)                               \
                                                                   \
    base * base::xdrNew(XDR * xdrs)                                \
    {                                                              \
        PLT_PRECONDITION(xdrs->x_op == XDR_DECODE);                \
        base * p = 0;                                              \
        enum_t typecode;                                           \
            if (xdr_enum(xdrs, &typecode)) {                       \
            /* typecode successfully decoded */                    \
            switch(typecode) {                                     \

#define KS_XDR_MAP(constant,derived)                       \
            case constant:                                 \
            {                                              \
                bool ok=false;                             \
                p = new derived(xdrs, ok);                 \
                if (! ok && p) {                           \
                    delete p;                              \
                    p = 0;                                 \
                }                                          \
            } break                                        

#define KS_END_IMPL_XDRUNION                                       \
           default:                                                \
                /* unknown typecode */                             \
                /* p = 0;           */                             \
                    { }                                            \
            }                                                      \
        }                                                          \
        if (p) {                                                   \
            PLT_ASSERT(p->xdrTypeCode() == typecode);              \
            if (!p->xdrDecodeCommon(xdrs)) {                       \
                delete p;                                          \
                return 0;                                          \
            }                                                      \
        }                                                          \
        return p;                                                  \
    }                                                              \
typedef void ks_dummy_typedef

//////////////////////////////////////////////////////////////////////

#define KS_IMPL_XDRCTOR(classname)                         \
    classname::classname(XDR *xdr, bool &ok) {             \
        ok = xdrDecode(xdr);                               \
    }

//////////////////////////////////////////////////////////////////////

#define KS_IMPL_XDRNEW(classname)                          \
    classname * classname::xdrNew(XDR * xdrs)              \
    {                                                      \
        bool ok=false;                                     \
        classname * p = new classname(xdrs, ok);           \
        if ( !ok && p) {                                   \
            delete p;                                      \
            p = 0;                                         \
        }                                                  \
        return p;                                          \
    }                                                      \
typedef void ks_dummy_typedef

//////////////////////////////////////////////////////////////////////
// INLINE IMPLEMENTATION
//////////////////////////////////////////////////////////////////////

inline bool 
KsXdrUnion::xdrDecodeCommon(XDR *)
{
    return true;
}

//////////////////////////////////////////////////////////////////////

inline bool
KsXdrUnion::xdrEncodeCommon(XDR *) const
{
    return true;
}

#endif

/* -*-plt-c++-*- */

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

/* Author: Markus Juergens <markusj@plt.rwth-aachen.de> */

//////////////////////////////////////////////////////////////////////

#include "ks/sorter.h"
#include "ks/package.h"

//////////////////////////////////////////////////////////////////////

bool
operator == (const KscSorter::Key &k1, const KscSorter::Key &k2)
{
    return k1.server == k2.server
        && k1.av_module == k2.av_module;
}

//////////////////////////////////////////////////////////////////////

unsigned long
KscSorter::Key::hash() const 
{
    return (unsigned long)av_module 
        + (unsigned long)server;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

bool
KscSorterBucket::add(KscBucketHandle hbucket)
{
    PLT_ASSERT(hbucket);

#if PLT_DEBUG
    size_t start_size = size() + hbucket->size();
#endif

    bool ok = true;

    while(ok && !(hbucket->isEmpty())) {
        ok = add(hbucket->removeFirst());
    }

    PLT_POSTCONDITION((start_size == size()) && (hbucket->size() == 0));

    return ok;
}

//////////////////////////////////////////////////////////////////////

KscVariableHandle
KscSorterBucket::removeFirst()
{
    PLT_PRECONDITION(!var_lst.isEmpty());

    KscVariableHandle hvar = var_lst.removeFirst();
    --var_count;
    return hvar;
}

//////////////////////////////////////////////////////////////////////


PltArray<KscVariableHandle> 
KscSorterBucket::getSortedVars()
{
    PltArray<KscVariableHandle> sv(size());

    if(sv.size() != size()) {
        // failed to allocate memory
        return PltArray<KscVariableHandle>(0);
    }

    size_t count = 0;
    while(!var_lst.isEmpty()) {
        sv[count++] = var_lst.removeFirst();
    }

    var_count = 0;

    PltSort<KscVariableHandle>::qsort(sv);

    return sv;
}       
    
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

KscSorter::~KscSorter() 
{
}

//////////////////////////////////////////////////////////////////////

const KscAvModule *
KscSorter::findAvModule(const KscVariable *var)
{
    const KscAvModule *temp = var->getAvModule();

    if( temp ) {
        return temp;
    } else if( (temp = rel_pkg.getAvModule()) ) {
        return temp;
    } else if( (temp = var->getServer()->getAvModule()) ) {
        return temp;
    } else {
        return KscClient::getClient()->getAvModule();
    }
}

//////////////////////////////////////////////////////////////////////

KscBucketHandle 
KscSorter::removeFirst()
{
    PltHashIterator<Key, KscBucketHandle> it(table);
    KscBucketHandle bucket;

    if(it) {
        Key key = it->a_key;
        bucket = it->a_value;

        table.remove(key, bucket);
    } else {
        PLT_DMSG("KscSorter::removeFirst() called for empty object" << endl;);
    }

    return bucket;
}

//////////////////////////////////////////////////////////////////////

KscBucketHandle 
KscSorter::removeMatchingBucket(KscBucketHandle bucket)
{
    KscBucketHandle res;
    Key key(bucket->getAvModule(), bucket->getServer());

    table.remove(key, res);

    return res;
}

//////////////////////////////////////////////////////////////////////

bool
KscSorter::sortVars()
{
    // iterate over variables and
    // (1) add a variable to the approbiate bucket if it exists
    // (2) create a new bucket if neccessary and add variable
    //
    PltIterator<KscVariableHandle> *var_it = 
        rel_pkg.newVariableIterator(false);
    if(!var_it) return false;

    bool ok = true;
    (*var_it).toStart();

    while(ok && *var_it) {
        KscVariableHandle current = **var_it;
        if( !fDirtyOnly || current->isDirty() ) { 
            Key key(findAvModule(current.getPtr()),
                    current->getServer());
            KscBucketHandle bucket;
            
            if(table.query(key, bucket)) {
                // bucket found
                //
                ok = bucket->add(current);
            } else {
                // create bucket
                //
                bucket.bindTo(new KscSorterBucket(key.getAvModule(),
                                                  key.getServer()),
                              PltOsNew);
                if(bucket) {
                    ok = table.add(key, bucket)
                        && bucket->add(current);
                } else {
                    ok = false;
                }
            }
        }

        ++(*var_it);
    }

    delete var_it;
    var_it = 0;

    if(!ok) return false;

    // iterate over the subpackages and sort their variables too
    //
    KscSubpackageIterator *pkg_it =
        rel_pkg.newSubpackageIterator();
    if(!pkg_it) return false;

    while(ok && *pkg_it) {
        KscSorter sub_sorter(***pkg_it, fDirtyOnly);
        ++(*pkg_it);
        if( (ok = sub_sorter.isValid()) ) {
            while(!sub_sorter.isEmpty()) {
                KscBucketHandle new_bucket = sub_sorter.removeFirst();
                Key key(new_bucket->getAvModule(),
                        new_bucket->getServer());
                KscBucketHandle matching_bucket;
                if(table.query(key, matching_bucket)) {
                    ok = matching_bucket->add(new_bucket);
                } else {
                    ok = table.add(key, new_bucket);
                }
            }
        }
    }

    delete pkg_it;
        
    return ok;
}
                
//////////////////////////////////////////////////////////////////////
// EOF sorter.cpp
//////////////////////////////////////////////////////////////////////













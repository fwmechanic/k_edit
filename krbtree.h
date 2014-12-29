/*
Generic C code for red-black trees.
Copyright (C) 2000 James S. Plank

This library is a free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* Revision 1.2.  Jim Plank */

/* Original code by Jim Plank (plank@cs.utk.edu) */
/* modified for THINK C 6.0 for Macintosh by Chris Bartley */

//
// Massively reshaped and extended by Kevin Goodwin 14/15-May-2002
//

#ifndef __KRBTREE_H__
#define __KRBTREE_H__

#include <cstddef>
#include <cstdio>

#include "my_strutils.h"

struct RbCtrl;
struct RbNode;
struct RbNode
   {
   char    red;
   char    internal;
   char    left;
   char    roothead;  // bit 1 is root, bit 2 is head
   RbNode *flink;
   RbNode *blink;
   RbNode *parent;
   union
      {
      // if node is "internal", this points to LEFT external node
      RbNode         *leftExt;
      RbCtrl         *pCtrl;   // iff ishead

      // if node is "external", this points to a key (the node DOES NOT own, and thus NEVER FREES, the key)
      PCVoid          pv;   // ptr to arbitrary key value (app must provide cmpfxn)
                         // OR ptr to BIG-endian multibyte key value (app must provide keylen)
      const char     *s;    // ptr to C-string (NUL-terminated) key value (this is in essence BIG-endian)
      const uint16_t *pu16; // ptr to host-endian multibyte key value
      const uint32_t *pu32; // ptr to host-endian multibyte key value
      } key;

   union
      {
      // if node is "internal", this points to RIGHT external node
      RbNode         *rightExt;

      // if node is "external", this points to the node's VALUE
      PVoid           v; // the node DOES NOT own, and thus NEVER FREES, the value
      } val;

   int     chkTag;
   #define chkTagRbNode  0x3C59A55A
   };

typedef RbNode RbTree;

static inline void SetPRBNValid(   RbNode *p )    {        p->chkTag =  chkTagRbNode; }
static inline int  IsPRBNValid ( const RbNode *p) { return p->chkTag == chkTagRbNode; }


//
//  From a user perspective, rb trees are a collection of nodes, the collection
//  having certain properties.
//
//  The tree-head node (*PRbTree) is special in that it remains the head of
//  the tree even though it is otherwise identical to any other node.  We take
//  advantage of this property to use the one available pointer to point to a
//  "control structure" (see RbCtrl).
//
//  - A node is described by a PRbNode, and has (pointer to) key, and (pointer
//    to) value.
//
//  - The association of PRbNode->[key,value] remains fixed for the life of the
//    node, even though the position of the node may change within the tree.
//
//    - it is GENERALLY assumed that key points within *value: deallocation of
//      an entire tree may include deallocation of the value, but APIs are not
//      included to delete the key directly.
//
//  - From the tree head pointer, pointers to the least- (rb_first) and
//    greatest- (rb_last) keyed members of the tree can be directly obtained.
//
//    From these endpoints there exists a sorted doubly-linked-list of all the
//    nodes in the tree which can be negotiated using rb_next and rb_prev.
//
//       rb_next( rb_last ( tree ) ) == tree
//       rb_prev( rb_first( tree ) ) == tree
//
//    Macros rb_traverse and rb_reverse offer a plug-and-play mechanism for
//    performing this traversal.
//

typedef int (*rb_cmpfxn)(PCVoid,PCVoid);

//
// An instance of RbCtrl is associated with one _OR MORE_ PRbTrees
// and controls certain aspects of the tree's behavior (how memory blocks
// containing nodes (but NOT values) are allocated and deallocated, and how
// log output is written).
//
// Operation counters are also maintained.  Although these may overflow, any
// overflow halts the ALL counter increments, so ratios between counters are
// always valid.  rb_ctrl_reset_counters() erases any overflow condition.
//
// An element count IS NOT MAINTAINED simply because the scope of RbCtrl IS
// NOT A SINGLE TREE.
//
// If pCtrl==nullptr is passed to rb_alloc_tree, a default RbCtrl is used which
// uses malloc & free, and logs output to stderr.
//
struct RbCtrl
   {
   void *(*nd_alloc)( size_t );          // NODE-alloc function
   void  (*nd_dealloc)( void * );        // NODE-dealloc function
   int   (CDECL__ * log)( const char *fmt, ... ); // logging function
   int fOverflow;                        // operation counters
   int searches;
   int inserts;
   int deletes;
   };

// TREE HEAD (valueless-node) constructor (rb_insert_XXX are value-node constructors)
//
extern RbTree *rb_alloc_tree( RbCtrl *pCtrl );   // Allocates and inits a new rb-tree (head structure)

// TREE destructors (these NEVER deallocate keys, which are assumed to lie within values)
//
// return value, if any, is always return nullptr (to store in tree pointer)
//
extern RbTree *rb_dealloc_tree( RbTree *tree ); // frees all nodes INcluding root (tree param) but NOT the associated values
extern void rb_dealloc_subtree( RbTree *tree ); // just like rb_dealloc_tree but doesn't dealloc root (tree param)

extern RbTree *rb_dealloc_treev(  // frees all nodes INcluding root (tree param) AND every associated value
    RbTree *tree, void *pExtra, void (*val_dealloc)( void *pData, void *pExtra ) );
extern void rb_dealloc_subtreev( // just like rb_dealloc_treev, but doesn't dealloc root (tree param)
    RbTree *tree, void *pExtra, void (*val_dealloc)( void *pData, void *pExtra ) );


// value-node destructor
//
extern void *rb_delete_node( RbTree *tree, RbNode *node ); // unlinks & frees node (but not the key), returns value of node freed

// Creates a node with key and val and inserts it into tree BEFORE (sort-order-wise)
// node.  Useful in cases where you want to check first for a key collision,
// THEN insert.  In this case, do:
//
//    RbNode *nd = rb_find_gte_xxx( &eq, hd, key );
//    if( !eq ) rb_insert_before( hd, nd, key, val );
//
// DOES NOT CHECK TO ENSURE THAT YOU ARE MAINTAINING CORRECT SORT ORDER.
//
extern RbNode *rb_insert_before( RbTree *tree, RbNode *node, PCVoid key, PVoid val );

// to create a new type of tree-key, create 3 new functions:
//   rb_find_gte_XXX
//   rb_find_XXX
//   rb_insert_XXX
//

#define RB_FUNCS_SCALAR_EXT(name,keytype)                                  \
extern RbNode *rb_find_gte_##name(int *equal, RbTree *tree, keytype key);  \
extern RbNode *rb_find_##name(RbTree *tree, keytype key);                  \
extern RbNode *rb_insert_##name(RbTree *tree, keytype key, PVoid val);

RB_FUNCS_SCALAR_EXT(u32,const uint32_t*)
RB_FUNCS_SCALAR_EXT(u16,const uint16_t*)

// Creates a node with key and val and inserts it into tree.
//   rb_insert_str uses strcmp() as a comparison function (key is C string);
//   rb_insert_mem uses memcmp() as a comparison function (key is array of uint8_t);
//   rb_insert_gen uses user-supplied fxn() as a comparison function;
//
extern RbNode *rb_insert_str( RbTree *tree, const char *key, PVoid val);
extern RbNode *rb_insert_mem( RbTree *tree, PCVoid key, size_t keylen, PVoid val );
extern RbNode *rb_insert_gen( RbTree *tree, PCVoid key, rb_cmpfxn fxn, PVoid val );

// Returns the external node in tree whose key-value == key.
// Returns nullptr if there is no such node in the tree
//
extern RbNode *rb_find_str( RbTree *tree, const char *key);
extern RbNode *rb_find_mem( RbTree *tree, PCVoid key, size_t keylen);
extern RbNode *rb_find_gen( RbTree *tree, PCVoid key, rb_cmpfxn fxn);
extern RbNode *rb_find_sri( RbTree *tree, const boost::string_ref &key);

// Returns the external node in tree whose key-value == key OR
// whose key-value is the smallest value greater than key.
// Sets *equal to 0 unless an identical key was found.
//
extern RbNode *rb_find_gte_str( int *equal, RbTree *tree, const char *key           );
extern RbNode *rb_find_gte_sri( int *equal, RbTree *tree, const boost::string_ref &key );
extern RbNode *rb_find_gte_mem( int *equal, RbTree *tree, PCVoid key, size_t keylen );
extern RbNode *rb_find_gte_gen( int *equal, RbTree *tree, PCVoid key, rb_cmpfxn fxn );

// iterators and accessors
//
extern RbCtrl *rb_ctrl(    const RbTree *t ); // returns (pointer to) RbCtrl struct of tree
extern RbNode *rb_first(   const RbTree *t ); // returns (pointer to) lowest        sorting RbNode *in tree
extern RbNode *rb_last(    const RbTree *t ); // returns (pointer to) greatest      sorting RbNode *in tree
extern RbNode *rb_next(    const RbNode *n ); // returns (pointer to) next greatest sorting RbNode *in tree
extern RbNode *rb_prev(    const RbNode *n ); // returns (pointer to) next lowest   sorting RbNode *in tree
extern int     rb_isempty( const RbNode *n ); // returns C bool val
extern PVoid   rb_val(     const RbNode *n ); // returns (pointer to) value
extern PCVoid  rb_key(     const RbNode *n ); // returns (pointer to) key

// integrity checking API (also gathers metrics)
//
typedef struct
   {
   int elements;
   int plMax; // max # of nodes in path from n to the root
   int plMin; // min # of nodes in path from n to the root
   } TRbInfo, *PRbInfo;
extern const char * rb_tree_corrupt( RbTree *tree, PRbInfo pInfo );
extern int rb_walk_tree_ok( RbNode *t, int (*fDataValid)( PVoid ) ); // check every node in the tree

extern void rb_ctrl_reset_counters( RbCtrl *pCtrl );
extern void rb_logstats(            RbCtrl *pCtrl ); // write counter contents using log function

// rb_traverse & rb_reverse are loop control structures that allow you to
// iterate thru an rb_tree in sorted (or reverse-sorted) order.
//
#define rb_nil(t)   static_cast<RbNode *>(t)
#define rb_traverse( ptr, tree)  for( ptr=rb_first(tree); ptr != rb_nil(tree); ptr=rb_next(ptr) )
#define rb_reverse(  ptr, tree)  for( ptr=rb_last( tree); ptr != rb_nil(tree); ptr=rb_prev(ptr) )

/////////////////////////////////////////////////////////////////////////////
//
// maybe not the best mnenomics, but:
// "RTN?_UNLESS_Pxxx_OK" return-rv/fail if nullptr or bad struct ptr
// "RTN?_UNLESS_Pxxx_0OK" same as "RTN?_UNLESS_Pxxx_OK", but nullptr ptr is
// non-fatal (still causes rv return tho)
//

// #include "defines.h" // for RTN_UNLESS

#define  RTN_UNLESS( rv, test )  assert( test )
#define  RTNV_UNLESS(    test )  assert( test )

// function returns value case:
#define  RTN_UNLESS_PRBN_0OK(rv,PRBN) if (!(PRBN)) return rv; RTN_UNLESS((rv),IsPRBNValid(PRBN))
#define  RTN_UNLESS_PRBN_OK(rv,PRBN)                          RTN_UNLESS((rv),IsPRBNValid(PRBN))

// function returns void case:
#define  RTNV_UNLESS_PRBN_0OK(PRBN)   if (!(PRBN)) return; RTNV_UNLESS(IsPRBNValid(PRBN))
#define  RTNV_UNLESS_PRBN_OK(PRBN)                         RTNV_UNLESS(IsPRBNValid(PRBN))
//
// Note that the internal checks using RTN_UNLESS_PRBN_OK, etc.  offer little
// hope of continuing in the face of an error given that most of
// RTN_UNLESS_PRBN_OK return nullptr on failure (can you think of a better
// alternative?).  If the tree is mucked up, and by convention a nullptr
// is never returned, a crash will happen when the null ptr deref is done by
// calling code (the only other alternative I can think of is returning 'p',
// the pointer that was found to be in error, but best case that would seem
// to lead to an infinite loop...)
//
// So all there is to say here is that leaving an error message at _the first
// detection_ of corruption (and hopefully, an error trace), and letting the
// app crash, is better than having the app get much more confused and bomb
// out in a less decipherable way.
//
//   03-Sep-2002 klg
//
/////////////////////////////////////////////////////////////////////////////

typedef RbNode TIdxNode;

#endif // __KRBTREE_H__

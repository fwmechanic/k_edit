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

#ifndef  UNIT_TEST_KRBTREE
#define  UNIT_TEST_KRBTREE   0
#endif

#include "krbtree.h"  // FIRST to reveal include dependencies!

/////////////////////////////////////////////////////////////////////////////
//
// private methods:
//
static inline RbNode *getlext(   const RbNode *n )         { return n->key.leftExt       ; }
static inline RbNode *getrext(   const RbNode *n )         { return n->val.rightExt      ; }
static inline void    setlext(   RbNode * n , RbNode *v )  {        n->key.leftExt  = v  ; }
static inline void    setrext(   RbNode * n , RbNode *v )  {        n->val.rightExt = v  ; }

static inline int     isred(     const RbNode *n )         { return n->red               ; }
static inline int     isblack(   const RbNode *n )         { return !isred(n)            ; }
static inline void    setred(    RbNode * n )              {        n->red = 1           ; }
static inline void    setblack(  RbNode * n )              {        n->red = 0           ; }

static inline int     isleft(    const RbNode *n )         { return n->left              ; }
static inline int     isright(   const RbNode *n )         { return !isleft(n)           ; }
static inline void    setleft(   RbNode * n )              {        n->left = 1          ; }
static inline void    setright(  RbNode * n )              {        n->left = 0          ; }

static inline int     ishead(    const RbNode *n )         { return n->roothead &  2     ; }
static inline int     isroot(    const RbNode *n )         { return n->roothead &  1     ; }
static inline void    sethead(   RbNode * n )              {        n->roothead |= 2     ; }
static inline void    setroot(   RbNode * n )              {        n->roothead |= 1     ; }
static inline void    setnormal( RbNode * n )              {        n->roothead  = 0     ; }

static inline int     isint(     const RbNode *n )         { return n->internal          ; }
static inline int     isext(     const RbNode *n )         { return !isint(n)            ; }
static inline void    setint(    RbNode * n )              {        n->internal = 1      ; }
static inline void    setext(    RbNode * n )              {        n->internal = 0      ; }

static inline RbCtrl *getctrl(   const RbNode *hd )        { return hd->key.pCtrl      ; }
static inline void    setctrl(   RbTree * hd, RbCtrl *pc ) {        hd->key.pCtrl = pc ; }

static inline RbNode *sibling(   RbNode * n )              { return isleft(n) ? n->parent->blink
                                                                              : n->parent->flink;
                                                           }

/////////////////////////////////////////////////////////////////////////////
//
// checker for a particular variant of RbNode: the HEAD node:
//
static inline int  IsPRBHDValid ( const RbNode *p) { return IsPRBNValid(p) && ishead(p); }

#define  RTN_UNLESS_PRBHD_0OK(rv,PRBHD) if (!PRBHD) return rv; RTN_UNLESS(rv,IsPRBHDValid(PRBHD))
#define  RTN_UNLESS_PRBHD_OK(rv,PRBHD)                         RTN_UNLESS(rv,IsPRBHDValid(PRBHD))

// function returns void case:
#define  RTNV_UNLESS_PRBHD_0OK(PRBHD)   if (!PRBHD) return; RTNV_UNLESS(IsPRBHDValid(PRBHD))
#define  RTNV_UNLESS_PRBHD_OK(PRBHD)                        RTNV_UNLESS(IsPRBHDValid(PRBHD))
//
/////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
// #include <cctype>

// ===============================  begin Unit Test Code  ========================================
// ===============================  begin Unit Test Code  ========================================
// ===============================  begin Unit Test Code  ========================================
// ===============================  begin Unit Test Code  ========================================
// ===============================  begin Unit Test Code  ========================================

#if UNIT_TEST_KRBTREE

// 20091215 kgoodwin got this running again; with redirection it's VERY fast these days :)
//
// cl -DUNIT_TEST_KRBTREE=1 -J -Zi -Od -DDEBUG -MTd krbtree.cpp && krbtree.exe < krbtree.cod > krbtree.log && del krbtree.obj krbtree.exe krbtree.pdb krbtree.ilk krbtree.log
//
// Note: I'm nuking various output files when done as the unit-test .obj file bends the editor link.

#if !defined(_WIN32)

#define rand16()  rand()

#else

#define DBGf      printf

// idiotic MSVC rand impl only yields _15 bits_ of random value!!!
uint16_t rand16()
   {
   return (rand() << 1) ^ rand();
   }

#endif

static void Free( void *pData, void *pExtra )
   {
   free( pData );
   }

PVoid AllocNZ_( size_t bytes )             { return malloc( bytes ); }
void  Free_( void *pv )                    { free( pv ); }

static RbCtrl s_CmdIdxRbCtrl = { AllocNZ_, Free_, printf };

typedef struct
   {
   uint16_t key;
   uint16_t bkupKey;
   } *PNode;

void PRbInfoPrint( char *const name, PRbInfo pInfo )
   {
   DBGf( "%s: %i elements, pathlen=( %i, %i )\n",
           name, pInfo->elements, pInfo->plMin , pInfo->plMax );
   }

int main( int argc, char *argv[] )
   {
   TRbInfo rbi;
   size_t  ix;

   RbTree *u16tree;
   RbTree *stree   = rb_alloc_tree(&s_CmdIdxRbCtrl);
   RbTree *itree   = rb_alloc_tree(&s_CmdIdxRbCtrl);
   RbCtrl *rbc     = rb_ctrl( itree );
   RbNode *ptr;
   PNode   pprev;

   DBGf( " alloc=%p, free=%p\nmalloc=%p, free=%p\n", rbc->nd_alloc, rbc->nd_dealloc, (void *)malloc, (void *)free );

   for(;;)
      {
      char buf[300];
      char *p = fgets( BSOB(buf), stdin );
      if( !p )
         break;

      p = strdup( p );
      rb_insert_str( stree, p, p );
      if( strlen( p ) >= 9 )
         {
         p = strdup( p );
         rb_insert_mem( itree, p, 9, p );
         }
      }

   rb_logstats( rbc );

   // ------------------------------------------------------------------
   u16tree = rb_alloc_tree(&s_CmdIdxRbCtrl);

   for( ix=0; ix<0x10000; ix++ )
      {
      PNode pNew = (PNode)malloc( sizeof(*pNew) );
      pNew->key = pNew->bkupKey = ix;
      rb_insert_u16( u16tree, &pNew->key, pNew );
      }

   assert( rb_next( rb_last ( u16tree ) ) == u16tree );
   assert( rb_prev( rb_first( u16tree ) ) == u16tree );

   assert( !rb_tree_corrupt( u16tree, &rbi ) );
   PRbInfoPrint( "u16tree inorder at fullest", &rbi );
   rb_logstats( rbc );

   pprev = nullptr;
   rb_traverse(ptr, u16tree)
      {
      PNode pNew = (PNode)rb_val(ptr);
      assert( pNew->key == pNew->bkupKey );
      if( pprev )
         assert( pprev->key+1 == pNew->key );
      pprev = pNew;
      }

   for( ix=0;ix<0x8000;ix++ )
      {
      int      equal;
      uint16_t key = rand16();
      RbNode *pNd = rb_find_gte_u16( &equal, u16tree, &key );
      if( equal )
         free( rb_delete_node( u16tree, pNd ) );
      }

   assert( !rb_tree_corrupt( u16tree, &rbi ) );
   PRbInfoPrint( "u16tree inorder after random deletion", &rbi );
   rb_logstats( rbc );

   pprev = nullptr;
   rb_traverse(ptr, u16tree)
      {
      PNode pNew = (PNode)rb_val(ptr);
      assert( pNew->key == pNew->bkupKey );
      if( pprev )
         assert( pprev->key < pNew->key );
      pprev = pNew;
      }
   u16tree = rb_dealloc_treev( u16tree, 0, Free );

   // ------------------------------------------------------------------
   u16tree = rb_alloc_tree(&s_CmdIdxRbCtrl);

   for( ix=0;ix<0x10000*4;ix++ )
      {
      RbNode *pNd;
      int     equal;
      PNode   pNew = (PNode)malloc( sizeof(*pNew) );
      pNew->key = pNew->bkupKey = rand16();

      pNd = rb_find_gte_u16( &equal, u16tree, &pNew->key );
      if( equal )
         free( rb_delete_node( u16tree, pNd ) );

      rb_insert_u16( u16tree, &pNew->key, pNew );
      }

   assert( !rb_tree_corrupt( u16tree, &rbi ) );
   PRbInfoPrint( "u16tree at fullest", &rbi );

   rb_logstats( rbc );

   rb_traverse(ptr, u16tree)
      {
      PNode pNew = (PNode)rb_val(ptr);
      assert( pNew->key == pNew->bkupKey );
      DBGf( "%5u\n", pNew->key );
      }

   for( ix=0;ix<0x8000;ix++ )
      {
      int      equal;
      uint16_t key = rand16();
      RbNode *pNd = rb_find_gte_u16( &equal, u16tree, &key );
      if( equal )
         free( rb_delete_node( u16tree, pNd ) );
      }

   assert( !rb_tree_corrupt( u16tree, &rbi ) );
   PRbInfoPrint( "u16tree after random deletion", &rbi );

   rb_logstats( rbc );

   rb_traverse(ptr, u16tree)
      {
      PNode pNew = (PNode)rb_val(ptr);
      assert( pNew->key == pNew->bkupKey );
      DBGf( "%5u\n", pNew->key );
      }
   u16tree = rb_dealloc_treev( u16tree, 0, Free );

   // ------------------------------------------------------------------

   assert( !rb_tree_corrupt( stree, &rbi ) );
   PRbInfoPrint( "stree", &rbi );
   rb_logstats( rbc );

   assert( !rb_tree_corrupt( itree, &rbi  ) );
   PRbInfoPrint( "itree", &rbi );
   rb_logstats( rbc );

   rb_traverse(ptr, stree)
      {
      DBGf( "%s", (char *)rb_val(ptr) );
      }
   stree = rb_dealloc_treev( stree, 0, Free );

   DBGf( "--------------------------------- ignore-case --------------------------\n" );

   rb_traverse(ptr, itree)
      {
      DBGf( "%s", (char *)rb_val(ptr) );
      }
   itree = rb_dealloc_treev( itree, 0, Free );
   rb_logstats( rbc );

   return 0;
   }

#endif // UNIT_TEST_KRBTREE
// ===============================  end Unit Test Code  ========================================
// ===============================  end Unit Test Code  ========================================
// ===============================  end Unit Test Code  ========================================
// ===============================  end Unit Test Code  ========================================
// ===============================  end Unit Test Code  ========================================

#include <climits>
#include <cstdarg>

static inline RbNode *rb_first__(   const RbNode *n ) { return n->flink; }
static inline RbNode *rb_last__(    const RbNode *n ) { return n->blink; }
static inline RbNode *rb_next__(    const RbNode *n ) { return n->flink; }
static inline RbNode *rb_prev__(    const RbNode *n ) { return n->blink; }
static inline int     rb_isempty__( const RbNode *n ) { return n->flink == n; }


// extern (checked) versions
//
RbNode *rb_first(   const RbTree *t ) { RTN_UNLESS_PRBHD_OK(static_cast<RbNode *>(rb_nil(t)),t); return rb_first__(t);   }
RbNode *rb_last(    const RbTree *t ) { RTN_UNLESS_PRBHD_OK(static_cast<RbNode *>(rb_nil(t)),t); return rb_last__(t);    }
RbNode *rb_next(    const RbNode *n ) { RTN_UNLESS_PRBN_OK(nullptr,n);             return rb_next__(n);    }
RbNode *rb_prev(    const RbNode *n ) { RTN_UNLESS_PRBN_OK(nullptr,n);             return rb_prev__(n);    }
int     rb_isempty( const RbNode *n ) { RTN_UNLESS_PRBN_OK(1,n);                   return rb_isempty__(n); }

#define rb_first_(n) rb_first(n)

/* Revision 1.2.  Jim Plank */
/* Original code by Jim Plank (plank@cs.utk.edu) */
/* modified for THINK C 6.0 for Macintosh by Chris Bartley */

#ifdef CHK_RBTREE
int HeadOk( RbTree *tree )
   {
// return  tree->chkTag == chkTagRbNode
//     &&  ishead(tree)
//     &&  tree->parent == tree
//     &&  tree->key.s[0] == 0;

   assert( tree->chkTag == chkTagRbNode );
   assert( ishead(tree)                 );
// assert( tree->parent == tree         );
// assert( tree->key.s[0] == 0          ); // now points to ctrl!
   return 1;
   }
#endif



#define  INCR_STAT(stattagname)                           \
static inline void incr_##stattagname( RbTree *tree )     \
   {                                                      \
   RbCtrl *pc = tree->key.pCtrl;                          \
   if( pc->fOverflow ) return;                            \
   if( ++pc->stattagname == INT_MAX ) pc->fOverflow = 1;  \
   }

INCR_STAT(searches)
INCR_STAT(inserts)
INCR_STAT(deletes)


RbCtrl *rb_ctrl( const RbTree *tree )
   {
   RTN_UNLESS_PRBHD_OK( nullptr, tree );
   return getctrl(tree);
   }

void rb_ctrl_reset_counters( RbCtrl *pCtrl )
   {
   pCtrl->fOverflow = 0;
   pCtrl->searches  = 0;
   pCtrl->inserts   = 0;
   pCtrl->deletes   = 0;
   }

void rb_logstats( RbCtrl *pCtrl )
   {
   pCtrl->log( "rb_logstats: %ssrch=%i ins=%i del=%i\n"
      , pCtrl->fOverflow ? "OVF! " : ""
      , pCtrl->searches
      , pCtrl->inserts
      , pCtrl->deletes
      );
   }

static void insert(RbNode *item, RbNode *list) // Inserts to the end of a list
   {
   RbNode *last_node = list->blink;
   list->blink = item;
   last_node->flink = item;
   item->blink = last_node;
   item->flink = list;
   }

static void inline unlink_node(RbNode *node)
   {
   node->flink->blink = node->blink;
   node->blink->flink = node->flink;
   }

static void single_rotate(RbNode *y, int l)
   {
   int rl, ir;
   RbNode *x; RbNode *yp;

   ir = isroot(y);
   yp = y->parent;
   if( !ir )
      rl = isleft(y);
   else
      rl = 0; // JUST TO INHIBIT GCC 2.96: "warning: `rl' might be used uninitialized in this function"

   if( l )
      {
      x = y->flink;
      y->flink = x->blink;
      setleft(y->flink);
      y->flink->parent = y;
      x->blink = y;
      setright(y);
      }
   else
      {
      x = y->blink;
      y->blink = x->flink;
      setright(y->blink);
      y->blink->parent = y;
      x->flink = y;
      setleft(y);
      }

   x->parent = yp;
   y->parent = x;
   if( ir )
      {
      yp->parent = x;
      setnormal(y);
      setroot(x);
      }
   else
      {
      if( rl )
         {
         yp->flink = x;
         setleft(x);
         }
      else
         {
         yp->blink = x;
         setright(x);
         }
      }
   }


static void recolor( RbNode *nd )
   {
   for(;;)
      {
      if( isroot(nd) )
         {
         setblack(nd);
         return;
         }

      RbNode *p = nd->parent;

      if( isblack(p) ) return;

      if( isroot(p) )
         {
         setblack(p);
         return;
         }

      RbNode *gp = p->parent;
      RbNode *sib = sibling(p);
      if( isred(sib) )
         {
         setblack(p);
         setred(gp);
         setblack(sib);
         nd = gp;
         }
      else // p's sibling is black, p is red, gp is black
         {
         if( (isleft(nd) == 0) == (isleft(p) == 0) )
            {
            single_rotate(gp, isleft(nd));
            setblack(p);
            setred(gp);
            }
         else
            {
            single_rotate(p, isleft(nd));
            single_rotate(gp, isleft(nd));
            setblack(nd);
            setred(gp);
            }
         return;
         }
      }
   }


// mk_new_ext and mk_new_int are ONLY CALLED BY rb_insert_before

static RbNode *mk_new_ext( RbTree *tree, PCVoid key, PVoid val )
   {
   RbNode *newnode = static_cast<RbNode *>( getctrl(tree)->nd_alloc(sizeof(*newnode)) );
   assert( newnode );
   SetPRBNValid(newnode);
   newnode->val.v  = val;
   newnode->key.pv = key;
   setext(newnode);
   setblack(newnode);
   setnormal(newnode);
   assert( IsPRBNValid(newnode) );
   return newnode;
   }

static void mk_new_int(RbTree *tree, RbNode *l, RbNode *r, RbNode *p, int il)
   {
   RbNode *newnode = static_cast<RbNode *>( getctrl(tree)->nd_alloc(sizeof(*newnode)) );
   SetPRBNValid(newnode);
   setint(newnode);
   setred(newnode);
   setnormal(newnode);
   newnode->flink = l;
   newnode->blink = r;
   newnode->parent = p;
   setlext(newnode, l);
   setrext(newnode, r);
   l->parent = newnode;
   r->parent = newnode;
   setleft(l);
   setright(r);
   if( ishead(p) )
      {
      p->parent = newnode;
      setroot(newnode);
      }
   else if( il )
      {
      setleft(newnode);
      p->flink = newnode;
      }
   else
      {
      setright(newnode);
      p->blink = newnode;
      }
   recolor(newnode);
   }


static RbNode *lprev(RbNode *n)
   {
   RTN_UNLESS_PRBN_OK( nullptr, n );
   if( ishead(n) ) return n;
   while( !isroot(n) )
      {
      RTN_UNLESS_PRBN_OK( nullptr, n );
      if( isright(n) ) return n->parent;
      n = n->parent;
      }
   return n->parent;
   }

static RbNode *rprev(RbNode *n)
   {
   RTN_UNLESS_PRBN_OK( nullptr, n );
   if( ishead(n) ) return n;
   while( !isroot(n) )
      {
      RTN_UNLESS_PRBN_OK( nullptr, n );
      if( isleft(n) ) return n->parent;
      n = n->parent;
      }
   return n->parent;
   }


// exposing init_rbtreehead in the public interface of this module, while it
// WILL optimize memory allocation by allowing you to embed a RbNode inside
// another data structure (vs.  having to malloc a standalone RbNode and
// assign it to a RbNode *within your application structure), breaks
// encapsulation of the definition of RbNode.
//
// This is the reason init_rbtreehead is currently static.
//
static RbNode *init_rbtreehead( RbTree *tree, RbCtrl *pCtrl )
   {
   RTN_UNLESS( nullptr, pCtrl );
   RTN_UNLESS( nullptr, tree );

   setctrl(tree, pCtrl);
   SetPRBNValid(tree);

   tree->flink  = tree;
   tree->blink  = tree;
   tree->parent = tree;
   sethead(tree);
   assert( IsPRBHDValid(tree) );
   return tree;
   }


RbNode *rb_alloc_tree( RbCtrl *pCtrl )
   {
   return init_rbtreehead( static_cast<RbTree *>( pCtrl->nd_alloc(sizeof(RbNode)) ), pCtrl );
   }

RbNode *rb_insert_before( RbTree *tree, RbNode *n, PCVoid key, PVoid val )
   {
   RbNode *prev; RbNode *newleft;

   RTN_UNLESS_PRBN_OK(nullptr,n);
   RTN_UNLESS_PRBHD_OK(nullptr,tree);

   incr_inserts(tree);
   if( ishead(n) )
      {
      if( n->parent == n ) // Tree is empty
         {
         RbNode *newnode = mk_new_ext(tree, key, val);
         insert(newnode, n);
         n->parent = newnode;
         newnode->parent = n;
         setroot(newnode);
         return newnode;
         }
      else
         {
         RbNode *newright = mk_new_ext(tree, key, val);
         insert(newright, n);
         newleft = newright->blink;
         setnormal(newleft);
         mk_new_int(tree, newleft, newright, newleft->parent, isleft(newleft));
         prev = rprev(newright);
         if( !ishead(prev) ) setlext(prev, newright);
         return newright;
         }
      }
   else
      {
      newleft = mk_new_ext(tree, key, val);
      insert(newleft, n);
      setnormal(n);
      mk_new_int(tree, newleft, n, n->parent, isleft(n));
      prev = lprev(newleft);
      if( !ishead(prev) ) setrext(prev, newleft);
      return newleft;
      }
   }

void *rb_delete_node(RbTree *tree, RbNode *n)
   {
   RTN_UNLESS_PRBN_OK(nullptr,n);
   RTN_UNLESS_PRBHD_OK(nullptr,tree);
   incr_deletes(tree);
   assert( isext(n) );
   assert( !ishead(n) );
   void *rv = static_cast<void *>(n->val.v); // const-breaking here since the presumed purpose is to pass back a void * for freeing...
   unlink_node(n); // Delete it from the list
   RbNode *p = n->parent;  // The only node
   if( isroot(n) )
      {
      p->parent = p;
      getctrl(tree)->nd_dealloc(n);
      return rv;
      }
   RbNode *s = sibling(n);  // The only node after deletion
   if( isroot(p) )
      {
      s->parent = p->parent;
      s->parent->parent = s;
      setroot(s);
      getctrl(tree)->nd_dealloc(p);
      getctrl(tree)->nd_dealloc(n);
      return rv;
      }
   RbNode *gp = p->parent;  // Set parent to sibling
   s->parent = gp;
   if( isleft(p) )
      {
      gp->flink = s;
      setleft(s);
      }
   else
      {
      gp->blink = s;
      setright(s);
      }
   char ir = isred(p);
   getctrl(tree)->nd_dealloc(p);
   getctrl(tree)->nd_dealloc(n);

   if( isext(s) )
      {      // Update proper rext and lext values
      p = lprev(s);
      if( !ishead(p) ) setrext(p, s);
      p = rprev(s);
      if( !ishead(p) ) setlext(p, s);
      }
   else if( isblack(s) )
      {
      assert( !isblack(s) );
      }
   else
      {
      p = lprev(s);
      if( !ishead(p) ) setrext(p, s->flink);
      p = rprev(s);
      if( !ishead(p) ) setlext(p, s->blink);
      setblack(s);
      return rv;
      }

   if( ir )
      return rv;

   // Recolor

   n = s;
   p = n->parent;
   s = sibling(n);
   while( isblack(p) && isblack(s) && isint(s) && isblack(s->flink) && isblack(s->blink) )
      {
      setred(s);
      n = p;
      if( isroot(n) )
         return rv;
      p = n->parent;
      s = sibling(n);
      }

   if( isblack(p) && isred(s) )
      {  // Rotation 2.3b
      single_rotate(p, isright(n));
      setred(p);
      setblack(s);
      s = sibling(n);
      }

   { // NOT a loop, JUST a new scope!
   RbNode *x; RbNode *z;
   char il;

   assert( isint(s) );

   il = isleft(n);
   x = il ? s->flink : s->blink ;
   z = sibling(x);

   if( isred(z) )
      {  // Rotation 2.3f
      single_rotate(p, !il);
      setblack(z);
      if( isred(p) ) setred  (s);
      else           setblack(s);
      setblack(p);
      }
   else if( isblack(x) )
      {   // Recoloring only (2.3c)
      assert( !(isred(s) || isblack(p)) );
      setblack(p);
      setred(s);
      }
   else if( isred(p) )
      { // 2.3d
      single_rotate( s,  il );
      single_rotate( p, !il );
      setblack(x);
      setred(s);
      }
   else
      {  // 2.3e
      single_rotate( s,  il );
      single_rotate( p, !il );
      setblack(x);
      }
   }
   return rv;
   }


//
// rb_walk_tree_ok - touch EVERY node in rbtree (both INTERNAL and EXTERNAL)
//                (rb_traverse only touches EXTERNAL nodes)
//
// caller supplied function (fDataValid) is called to check value-field of
// external nodes.  fDataValid should return NZ if OK, 0 if bad.
//
// returns NZ if all nodes OK
// returns 0  if any node NOT OK
//
int rb_walk_tree_ok( RbNode *t, int (*fDataValid)( PCVoid ) )
   {
   if( !IsPRBNValid(t) )
      return 0;  // <-------  THIS is the reason for rb_walk_tree!

   if( ishead(t) && t->parent == t )
      {
      return 1; // OK
      }
   else if( ishead(t) )
      {
      return rb_walk_tree_ok( t->parent, fDataValid );
      }
   else
      {
      if( isext(t) )
         {
         return fDataValid( t->val.v );  // <-------  THIS is the reason for rb_walk_tree!
         }
      else
         {
         return rb_walk_tree_ok( t->flink, fDataValid )
             && rb_walk_tree_ok( t->blink, fDataValid );
         }
      }
   }


// not used 03-Sep-2002 klg
// void rb_print_tree( RbNode *t, int level, void (*dispfxn)( void *pVal, void *glb_param ), void *glb_param )
//    {
//    RTN_UNLESS_PRBN_OK(t);
//    if( ishead(t) && t->parent == t )
//       {
//       printf("tree 0x%x is empty\n", t);
//       }
//    else if( ishead(t) )
//       {
//       printf("Head: 0x%x.  Root = 0x%x\n", t, t->parent);
//       rb_print_tree(t->parent, 0, dispfxn, glb_param );
//       }
//    else
//       {
//       int i;
//
//       if( isext(t) )
//          {
//          for( i = 0; i < level; i++ ) putchar(' ');
//          printf("Ext node 0x%x: %c,%c: p=0x%x, k=%s\n",
//                 t, isred(t)?'R':'B', isleft(t)?'l':'r', t->parent, t->key.s);
//          dispfxn( t->val.v, glb_param );
//          }
//       else
//          {
//          rb_print_tree(t->flink, level+2, dispfxn, glb_param );
//          rb_print_tree(t->blink, level+2, dispfxn, glb_param );
//          for( i = 0; i < level; i++ ) putchar(' ');
//          printf("Int node 0x%x: %c,%c: l=0x%x, r=0x%x, p=0x%x, lr=(%s,%s)\n",
//                 t, isred(t)?'R':'B', isleft(t)?'l':'r', t->flink,
//                 t->blink,
//                 t->parent, getlext(t)->key.s, getrext(t)->key.s);
//          }
//       }
//    }


void rb_dealloc_subtreev(RbTree *tree, void *pExtra, void (*val_dealloc)( void *pData, void *pExtra ) )
   { // FREES VALUES AS WELL AS NODES
   RTNV_UNLESS_PRBN_OK(tree);
   assert( ishead(tree) );
   while( rb_first_(tree) != rb_nil(tree) )
      {
      val_dealloc( rb_delete_node( tree, rb_first_(tree) ), pExtra );
      }
   // DOES NOT FREE 'tree', only it's children
   }

void rb_dealloc_subtree(RbTree *tree)
   {
   RTNV_UNLESS_PRBN_OK(tree);
   assert( ishead(tree) );
   while( rb_first_(tree) != rb_nil(tree) )
      {
      rb_delete_node( tree, rb_first_(tree) );
      }
   // DOES NOT FREE 'tree', only it's children
   }

RbNode *rb_dealloc_tree(RbTree *tree)
   {
   RTN_UNLESS_PRBHD_OK( nullptr, tree );
   rb_dealloc_subtree( tree );
   getctrl(tree)->nd_dealloc( tree );
   return nullptr;
   }

RbNode *rb_dealloc_treev( RbTree *tree, void *pExtra, void (*val_dealloc)( void *pData, void *pExtra ) )
   {
   RTN_UNLESS_PRBHD_OK( nullptr, tree );
   rb_dealloc_subtreev( tree, pExtra, val_dealloc );
   getctrl(tree)->nd_dealloc( tree );
   return nullptr;
   }

PVoid rb_val(const RbNode *node)
   {
   RTN_UNLESS_PRBN_OK( nullptr, node );
   assert( isext(node) );
   return node->val.v;
   }

PCVoid rb_key(const RbNode *node)
   {
   RTN_UNLESS_PRBN_OK( nullptr, node );
   assert( isext(node) );
   return node->key.pv;
   }

// 03-Sep-2002 klg this is seldom (never?) used...
// static RbNode *rb_insert_a(RbNode *nd, void *key, void *val)
//    {
//    RTN_UNLESS_PRBN_OK(nd);
//    return rb_insert_before(nd->flink, key, val);
//    }


//******************* rb_find_gte_ ****************************************************************

//-------------------------------------------------------------------------------------
//
// RB_FUNCS_SCALAR and RB_AUXFUNCS are used to "stampout" (via "rubber stamp")
// new instances of rb_find_gte_ for SCALAR TYPES (for which >, == and < are
// defined operators).  The rules are:
//
// - function generated will be named "rb_find_gte_<name>"
//
// - There MUST be a member in the union {} key named "p<name>" (ie.  for
//   name="u16", member named "pu16" must exist); and the typeof this member
//   MUST be identical to keytype: 'pointer to keyvalue'.
//
// - note that like keys for other APIs, keytype is A POINTER to the key value
//
#define RB_AUXFUNCS(name,keytype)                                                                \
RbNode *rb_find_##name(RbTree *tree, keytype key)                                                \
   { int equal; RbNode *rv=rb_find_gte_##name(&equal, tree, key); return equal ? rv : nullptr; } \
RbNode *rb_insert_##name(RbTree *tree, keytype key, PVoid val)                          \
   { int equal; return rb_insert_before( tree, rb_find_gte_##name(&equal, tree, key), key, val); }

#define RB_FUNCS_SCALAR(name,keytype)                                                 \
RbNode *rb_find_gte_##name(int *equal, RbTree *tree, keytype key)                     \
   {                                                                                  \
   RTN_UNLESS_PRBHD_OK( nullptr, tree );                                              \
   incr_searches( tree );                                                             \
   *equal = 0;                                                                        \
   if( tree->parent == tree ) return tree;                                            \
   if( *key == *tree->blink->key.p##name ) { *equal = 1; return tree->blink; }        \
   if( *key >  *tree->blink->key.p##name ) return tree;                               \
   tree = tree->parent;                                                               \
   while( 1 )                                                                         \
      {                                                                               \
      RTN_UNLESS_PRBN_OK( nullptr, tree );                                            \
      if( isext(tree) ) return tree;                                                  \
      if( *key == *getlext(tree)->key.p##name ) { *equal = 1; return getlext(tree); } \
      if( *key <  *getlext(tree)->key.p##name ) tree = tree->flink;                   \
      else                                      tree = tree->blink;                   \
      }                                                                               \
   }                                                                                  \
   RB_AUXFUNCS(name,keytype)

//-------------------------------------------------------------------------------------

RB_FUNCS_SCALAR(u32,const uint32_t*)
RB_FUNCS_SCALAR(u16,const uint16_t*)

RbNode *rb_find_gte_mem(int *equal, RbTree *tree, PCVoid key, size_t keylen)
   {
   RTN_UNLESS_PRBHD_OK( nullptr, tree );
   incr_searches( tree );
   *equal = 0;
   if( tree->parent == tree ) return tree;
   {
   int cmp = memcmp(key, tree->blink->key.pv, keylen);
   if( cmp == 0 ) { *equal = 1; return tree->blink; }
   if( cmp  > 0 ) return tree;
   tree = tree->parent;
   while( 1 )
      {
      RTN_UNLESS_PRBN_OK( nullptr, tree );
      if( isext(tree) ) return tree;
      cmp = memcmp(key, getlext(tree)->key.pv, keylen);
      if( cmp == 0 ) { *equal = 1; return getlext(tree); }
      if( cmp < 0 ) tree = tree->flink;
      else          tree = tree->blink;
      }
   }
   }

RbNode *rb_find_gte_str( int *equal, RbTree *tree, const char *key )
   {
   RTN_UNLESS_PRBHD_OK( nullptr, tree );
   incr_searches( tree );
   *equal = 0;
   if( tree->parent == tree ) return tree;
   {
   int cmp = strcmp(key, tree->blink->key.s);
   if( cmp == 0 ) { *equal = 1; return tree->blink; }
   if( cmp  > 0 ) return tree;
   tree = tree->parent;
   while( 1 )
      {
      RTN_UNLESS_PRBN_OK( nullptr, tree );
      if( isext(tree) ) return tree;
      cmp = strcmp(key, getlext(tree)->key.s);
      if( cmp == 0 ) { *equal = 1; return getlext(tree); }
      if( cmp < 0 ) tree = tree->flink;
      else          tree = tree->blink;
      }
   }
   }

RbNode *rb_find_gte_sri( int *equal, RbTree *tree, const boost::string_ref &key )
   {
   RTN_UNLESS_PRBHD_OK( nullptr, tree );
   incr_searches( tree );
   *equal = 0;
   if( tree->parent == tree ) return tree;
   {
   int cmp = cmpi(key, static_cast<PCChar>(tree->blink->key.pv));
   if( cmp == 0 ) { *equal = 1; return tree->blink; }
   if( cmp  > 0 ) return tree;
   tree = tree->parent;
   while( 1 )
      {
      RTN_UNLESS_PRBN_OK( nullptr, tree );
      if( isext(tree) ) return tree;
      cmp = cmpi(key, static_cast<PCChar>(getlext(tree)->key.pv));
      if( cmp == 0 ) { *equal = 1; return getlext(tree); }
      if( cmp < 0 ) tree = tree->flink;
      else          tree = tree->blink;
      }
   }
   }

RbNode *rb_find_gte_gen( int *equal, RbTree *tree, PCVoid key, rb_cmpfxn fxn )
   {
   RTN_UNLESS_PRBHD_OK( nullptr, tree );
   incr_searches( tree );
   *equal = 0;
   if( tree->parent == tree ) return tree;
   {
   int cmp = (*fxn)(key, tree->blink->key.pv);
   if( cmp == 0 ) { *equal = 1; return tree->blink; }
   if( cmp  > 0 ) return tree;
   tree = tree->parent;
   while( 1 )
      {
      RTN_UNLESS_PRBN_OK( nullptr, tree );
      if( isext(tree) ) return tree;
      cmp = (*fxn)(key, getlext(tree)->key.pv);
      if( cmp == 0 ) { *equal = 1; return getlext(tree); }
      if( cmp < 0 ) tree = tree->flink;
      else          tree = tree->blink;
      }
   }
   }

//******************* rb_find_ ********************************************************************

// these are "standard" coded since they take the "standard" parameter set:
RB_AUXFUNCS(str,const char*)


// these are specially coded since they take extra parameters: keylen, fxn

RbNode *rb_find_mem(RbTree *tree, PCVoid key, size_t keylen)
   {
   int equal;
   RbNode *rv = rb_find_gte_mem(&equal, tree, key, keylen);
   return equal ? rv : nullptr;
   }

RbNode *rb_find_gen(RbTree *tree, PCVoid key, rb_cmpfxn fxn)
   {
   int equal;
   RbNode *rv = rb_find_gte_gen(&equal, tree, key, fxn);
   return equal ? rv : nullptr;
   }

RbNode *rb_find_sri(RbTree *tree, const boost::string_ref &key )
   {
   int equal;
   RbNode *rv = rb_find_gte_sri(&equal, tree, key);
   return equal ? rv : nullptr;
   }

//******************* rb_insert_ ******************************************************************

// these are specially coded since they take extra parameters: keylen/fxn

RbNode *rb_insert_mem( RbTree *tree, PCVoid key, size_t keylen, PVoid val )
   {
   int equal;
   return rb_insert_before( tree, rb_find_gte_mem(&equal, tree, key, keylen), key, val);
   }

RbNode *rb_insert_gen( RbTree *tree, PCVoid key, rb_cmpfxn fxn, PVoid val )
   {
   int equal;
   return rb_insert_before( tree, rb_find_gte_gen(&equal, tree, key, fxn), key, val);
   }

//*************************************************************************************************

// not used -> warning -> promoted to ERROR, so commented out
// static int rb_nblack(RbNode *n) // returns # of black nodes in path from n to the root
//    {
//    int nb;
//    RTN_UNLESS_PRBN_OK(n);
//    assert( !(ishead(n) || isint(n)) );
//    nb = 0;
//    while( !ishead(n) )
//       {
//       if( isblack(n) ) nb++;
//       n = n->parent;
//       }
//    return nb;
//    }


static int rb_plength(RbNode *n) // returns the # of nodes in path from n to the root
   {
   int pl = 0;
   RTN_UNLESS_PRBN_OK(pl,n);
   RTN_UNLESS(pl, !ishead(n) && !isint(n) );
   while( !ishead(n) )
      {
      pl++;
      n = n->parent;
      }
   return pl;
   }

#define rbn_corrupt( pn )  (!IsPRBNValid(pn))

const char * rb_tree_corrupt( RbTree *tree, PRbInfo pInfo )
   {
   int plMax = 0;
   int plMin = 0;

   size_t fwdCt = 0;
   size_t rvsCt = 0;
   RbNode *ptr;
   if( rbn_corrupt(tree) )   return "RB_HEAD_CORRUPT";
   rb_traverse(ptr, tree)
      {
      int pl;
      if( rbn_corrupt(ptr) ) return "RB_FWD_CORRUPT";
      pl = rb_plength(ptr);
      if( plMax < pl ) plMax = pl;
      if( plMin > pl ) plMin = pl;
      ++fwdCt;
      }

   rb_reverse(ptr, tree)
      {
      if( rbn_corrupt(ptr) ) return "RB_RVS_CORRUPT";
      if( ++rvsCt > fwdCt )  return "RB_COUNTS_DIFFER";
      }
   if( fwdCt != rvsCt )      return "RB_COUNTS_DIFFER";
   if( pInfo )
      {
      pInfo->elements = fwdCt;
      pInfo->plMax    = plMax;
      pInfo->plMin    = plMin;
      }

   return nullptr;
   }

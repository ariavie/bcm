#ifndef _SB__DQ_H_
#define _SB__DQ_H_
/* --------------------------------------------------------------------------
**
** $Id: sbDq.h,v 1.10 Broadcom SDK $
**
** $Copyright: Copyright 2012 Broadcom Corporation.
** This program is the proprietary software of Broadcom Corporation
** and/or its licensors, and may only be used, duplicated, modified
** or distributed pursuant to the terms and conditions of a separate,
** written license agreement executed between you and Broadcom
** (an "Authorized License").  Except as set forth in an Authorized
** License, Broadcom grants no license (express or implied), right
** to use, or waiver of any kind with respect to the Software, and
** Broadcom expressly reserves all rights in and to the Software
** and all intellectual property rights therein.  IF YOU HAVE
** NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
** IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
** ALL USE OF THE SOFTWARE.  
**  
** Except as expressly set forth in the Authorized License,
**  
** 1.     This program, including its structure, sequence and organization,
** constitutes the valuable trade secrets of Broadcom, and you shall use
** all reasonable efforts to protect the confidentiality thereof,
** and to use this information only in connection with your use of
** Broadcom integrated circuit products.
**  
** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
** PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
** REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
** OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
** DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
** NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
** ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
** CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
** OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
** 
** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
** BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
** INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
** ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
** TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
** THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
** WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
** ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
**
** sbDq.h: doubly linked lists
**
** --------------------------------------------------------------------------*/

/*
 * Singly linked list (stack) structure
 */
#define SL_INIT(l)  do { (l) = NULL; } while (0)
#define SL_EMPTY(l) (!(l))

#define SL_INSERT_HEAD(l, e)          \
do                                    \
{	 		              \
  *((void **) (e)) = (void *) (l);    \
  ((void *) (l)) = (void *) (e);      \
} while (0)

#define SL_REMOVE_HEAD(l, e)          \
do                                    \
{				      \
  ((void *) e) = (void *) (l);	      \
  ((void *) (l)) = *((void **) (e));  \
} while (0)



/*
 * Doubly linked queue structure
 */

typedef struct dq_s *dq_p_t;
typedef struct dq_s
{
  volatile dq_p_t flink;	/* Forward link  */
  volatile dq_p_t blink;	/* Backward link */
}
dq_t;


#define DQ_INIT(q)  \
do                  \
{                   \
  (q)->flink = (q); \
  (q)->blink = (q); \
} while (0)

/* true if element (e) is not in a list */
#define DQ_NULL(e) (((e)->flink == (e)) && ((e)->blink == (e)))

/* true if queue (q) is empty */
#define DQ_EMPTY(q) ((q)->flink == (q))

#define DQ_HEAD(q, t) ((t) (q)->flink)
#define DQ_TAIL(q, t) ((t) (q)->blink)
#define DQ_NEXT(q, t) ((t) (q)->flink)
#define DQ_PREV(q, t) ((t) (q)->blink)

/*
 * Arguments are:
 *   q: pointer to queue block
 *   e: pointer to element (DQ) to insert
*/
#define DQ_INSERT_HEAD(q, e)      \
do                                \
{                                 \
  dq_p_t pElement;                \
  pElement = (dq_p_t) (e);        \
  pElement->flink = (q)->flink;   \
  pElement->blink =  (q);         \
  (q)->flink->blink = pElement;   \
  (q)->flink = pElement;          \
} while (0)


/*
 * Arguments are:
 *   q: pointer to queue block
 *   e: pointer to element (DQ) to insert
 */
#define DQ_INSERT_TAIL(q, e)      \
do                                \
{                                 \
  dq_p_t pElement;                \
  pElement = (dq_p_t) (e);        \
  pElement->flink = (q);          \
  pElement->blink = (q)->blink;   \
  (q)->blink->flink = pElement;   \
  (q)->blink = pElement;          \
} while (0)


/*
 * Arguments are:
 *   e: pointer to previous element
 *   n: pointer to new element to insert
 * CHECK If pNew is head before using
*/
#define DQ_INSERT_PREV(e, n)      \
do                                \
{                                 \
  dq_p_t pElement;                \
  dq_p_t _pNew;                    \
  dq_p_t pPrev;                   \
  pElement = (dq_p_t) (e);        \
  _pNew = (dq_p_t) (n);            \
  pPrev = pElement->blink;        \
  pPrev->flink = _pNew;            \
  _pNew->blink =  pPrev;           \
  _pNew->flink = pElement;         \
  pElement->blink = _pNew;         \
} while (0)

/*
 * Arguments are:
 *   e: pointer to next element
 *   n: pointer to new element to insert
 * CHECK If pNew is tail before using
*/
#define DQ_INSERT_NEXT(e, n)      \
do                                \
{                                 \
  dq_p_t pElement;                \
  dq_p_t _pNew;                    \
  dq_p_t pNext;                   \
  pElement = (dq_p_t) (e);        \
  _pNew = (dq_p_t) (n);            \
  pNext = pElement->flink;        \
  pNext->blink = _pNew;            \
  _pNew->flink =  pNext;           \
  pElement->flink = _pNew;         \
  _pNew->blink = pElement;         \
} while (0)

/*
 * Argument is:
 *   e: pointer to element (DQ) to remove
 */
#define DQ_REMOVE(e)                         \
do                                           \
{                                            \
  dq_p_t pElement;                           \
  pElement = (dq_p_t) (e);                   \
  pElement->blink->flink = pElement->flink;  \
  pElement->flink->blink = pElement->blink;  \
} while (0)


/*
 * Arguments are:
 *   q: pointer to queue block
 *   e: pointer to element (DQ) removed from the head of q
 */
#define DQ_REMOVE_HEAD(q, e)                               \
do                                                         \
{                                                          \
  dq_p_t pElement;                           \
  pElement = (dq_p_t) (e);                   \
  pElement = (q)->flink;                     \
  e = (void *)pElement;                      \
  pElement->blink->flink = pElement->flink;  \
  pElement->flink->blink = pElement->blink;  \
} while (0)

/*
 * Arguments are:
 *   q: pointer to queue block
 *   e: pointer to element (DQ) removed from the head of q
 */
#define DQ_REMOVE_TAIL(q, e)                               \
do                                                         \
{                                                          \
  dq_p_t pElement;                           \
  pElement = (dq_p_t) (e);                   \
  pElement = (q)->blink;                     \
  e = (void *)pElement;                      \
  pElement->blink->flink = pElement->flink;  \
  pElement->flink->blink = pElement->blink;  \
} while (0)

/*
 * Arguments new list head, old list head
 * Used to copy list head from one memory area
 * to a different memory area without affecting
 * contents of list
 */
#define DQ_SWAP_HEAD(n, o)               \
do                                       \
{                                        \
   (n)->flink = (o)->flink;              \
   (n)->blink = (o)->blink;              \
   (o)->flink->blink = (n);              \
   (o)->blink->flink = (n);              \
} while (0)                     

/*
 * This macro is a tidy way of performing subtraction to move from a
 * pointer within an object to a pointer to the object.
 *
 * Arguments are:
 *    type of object to recover (e.g. dq_p_t)
 *    pointer to object from which to recover element pointer
 *    pointer to an object of type t
 *    name of the DQ field in t through which the queue is linked
 * Returns:
 *    a pointer to the object, of type t
 */
#define DQ_ELEMENT(t, p, ep, f) \
  ((t) (((char *) (p)) - (((char *) &((ep)->f)) - ((char *) (ep)))))

/*
 * DQ_ELEMENT_GET performs the same function as DQ_ELEMENT, but does not
 * require a pointer of type (t).  This form is preferred as DQ_ELEMENT
 * typically generate Coverity errors, and the (ep) argument is unnecessary.
 *
 * Arguments are:
 *    type of object to recover (e.g. dq_p_t)
 *    pointer to object from which to recover element pointer
 *    name of the DQ field in t through which the queue is linked (t->dq_t)
 * Returns:
 *    a pointer to the object, of type t
 */
#define DQ_ELEMENT_GET(t, p, f) \
  ((t) (((char *) (p)) - (((char *) &(((t)(0))->f)))))

/*
 * Arguments:
 *   q:  head of queue on which to map f
 *   f:  function to apply to each element of q
 */
#define DQ_MAP(q, f, a)                                  \
do                                                       \
{                                                        \
  dq_p_t e, e0, q0;                                      \
  q0 = (q);                                              \
  for (e=q0->flink; e != q0; e = e0) {                   \
    e0 = e->flink;                                       \
    (void) (f)(e, a);                                    \
  }                                                      \
} while (0)

/*
 * Arguments:
 *   q:  head of queue on which to map f
 *   f:  function to apply to each element of q; should return 1 when
 *       desired queue element has been found.
 */
#define DQ_FIND(q, f, a)                                 \
do                                                       \
{                                                        \
  dq_p_t e, e0, q0;                                      \
  q0 = (q);                                              \
  for (e=q0->flink; e != q0; e = e0) {                   \
    e0 = e->flink;                                       \
    if ((f)(e, a)) break;                                \
  }                                                      \
} while (0)

/*
 * Arguments:
 *   q:  head of queue to determine length
 */
#define DQ_LENGTH(q, cnt)                                \
do                                                       \
{                                                        \
  dq_p_t e, q0;                                          \
  (cnt) = 0;                                             \
  q0 = (q);                                              \
  for (e = q0->flink; e != q0; e = e->flink) {           \
    (cnt)++;                                             \
  }                                                      \
} while (0)                                              \

/*
 * Arguments:
 *   q:  head of queue
 *   e:  each elem during traverse
 */
#define DQ_TRAVERSE(q, e)                                \
do                                                       \
{                                                        \
  dq_p_t e0, q0;                                         \
  q0 = (q);                                              \
  for (e=q0->flink; e != q0; e = e0) {                   \
    e0 = e->flink;                                       \

/*
 * Arguments:
 *   q:  head of queue
 *   e:  each elem during traverse
 */
#define DQ_BACK_TRAVERSE(q, e)                           \
do                                                       \
{                                                        \
  dq_p_t e0, q0;                                         \
  q0 = (q);                                              \
  for (e=q0->blink; e != q0; e = e0) {                   \
    e0 = e->blink;                                       \

#define DQ_TRAVERSE_END(q, e)                            \
  }                                                      \
} while (0)

#endif  /* _SB__DQ_H_ */

/*
 * File      : rt_rbtree.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009 RT-Thread Develop Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-11-21     weety      first implementation
 */


#ifndef	_RT_RBTREE_H
#define	_RT_RBTREE_H
#include <rtdef.h>

#if defined(container_of)
  #undef container_of
  #define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})
#else
  #define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#if defined(offsetof)
  #undef offsetof
  #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#else 
  #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#undef NULL
#if defined(__cplusplus)
  #define NULL 0
#else
  #define NULL ((void *)0)
#endif

struct rt_rb_node
{
	unsigned long  rt_rb_parent_color;
#define	RB_RED		0
#define	RB_BLACK	1
	struct rt_rb_node *rt_rb_right;
	struct rt_rb_node *rt_rb_left;
} ALIGN(sizeof(long));
    /* The alignment might seem pointless, but allegedly CRIS needs it */

struct rt_rb_root
{
	struct rt_rb_node *rt_rb_node;
};


#define rt_rb_parent(r)   ((struct rt_rb_node *)((r)->rt_rb_parent_color & ~3))
#define rt_rb_color(r)   ((r)->rt_rb_parent_color & 1)
#define rt_rb_is_red(r)   (!rt_rb_color(r))
#define rt_rb_is_black(r) rt_rb_color(r)
#define rt_rb_set_red(r)  do { (r)->rt_rb_parent_color &= ~1; } while (0)
#define rt_rb_set_black(r)  do { (r)->rt_rb_parent_color |= 1; } while (0)

static inline void rt_rb_set_parent(struct rt_rb_node *rb, struct rt_rb_node *p)
{
	rb->rt_rb_parent_color = (rb->rt_rb_parent_color & 3) | (unsigned long)p;
}
static inline void rt_rb_set_color(struct rt_rb_node *rb, int color)
{
	rb->rt_rb_parent_color = (rb->rt_rb_parent_color & ~1) | color;
}

#define RT_RB_ROOT	{ NULL }
#define	rt_rb_entry(ptr, type, member) \
	((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

#define RT_RB_EMPTY_ROOT(root)	((root)->rt_rb_node == NULL)
#define RT_RB_EMPTY_NODE(node)	(rt_rb_parent(node) == node)
#define RT_RB_CLEAR_NODE(node)	(rt_rb_set_parent(node, node))

static inline void rt_rb_init_node(struct rt_rb_node *rb)
{
	rb->rt_rb_parent_color = 0;
	rb->rt_rb_right = NULL;
	rb->rt_rb_left = NULL;
	RT_RB_CLEAR_NODE(rb);
}

extern void rt_rb_insert_color(struct rt_rb_node *, struct rt_rb_root *);
extern void rt_rb_erase(struct rt_rb_node *, struct rt_rb_root *);

typedef void (*rt_rb_augment_f)(struct rt_rb_node *node, void *data);

extern void rt_rb_augment_insert(struct rt_rb_node *node,
			      rt_rb_augment_f func, void *data);
extern struct rt_rb_node *rt_rb_augment_erase_begin(struct rt_rb_node *node);
extern void rt_rb_augment_erase_end(struct rt_rb_node *node,
				 rt_rb_augment_f func, void *data);

/* Find logical next and previous nodes in a tree */
extern struct rt_rb_node *rt_rb_next(const struct rt_rb_node *);
extern struct rt_rb_node *rt_rb_prev(const struct rt_rb_node *);
extern struct rt_rb_node *rt_rb_first(const struct rt_rb_root *);
extern struct rt_rb_node *rt_rb_last(const struct rt_rb_root *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void rt_rb_replace_node(struct rt_rb_node *victim, struct rt_rb_node *new, 
			    struct rt_rb_root *root);

static inline void rt_rb_link_node(struct rt_rb_node * node, struct rt_rb_node * parent,
				struct rt_rb_node ** rt_rb_link)
{
	node->rt_rb_parent_color = (unsigned long )parent;
	node->rt_rb_left = node->rt_rb_right = NULL;

	*rt_rb_link = node;
}

#endif	/* _RT_RBTREE_H */


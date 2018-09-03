/*
 * File      : rt_rbtree.c
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


#include "rt_rbtree.h"

static void __rt_rb_rotate_left(struct rt_rb_node *node, struct rt_rb_root *root)
{
	struct rt_rb_node *right = node->rt_rb_right;
	struct rt_rb_node *parent = rt_rb_parent(node);

	if ((node->rt_rb_right = right->rt_rb_left))
		rt_rb_set_parent(right->rt_rb_left, node);
	right->rt_rb_left = node;

	rt_rb_set_parent(right, parent);

	if (parent)
	{
		if (node == parent->rt_rb_left)
			parent->rt_rb_left = right;
		else
			parent->rt_rb_right = right;
	}
	else
		root->rt_rb_node = right;
	rt_rb_set_parent(node, right);
}

static void __rt_rb_rotate_right(struct rt_rb_node *node, struct rt_rb_root *root)
{
	struct rt_rb_node *left = node->rt_rb_left;
	struct rt_rb_node *parent = rt_rb_parent(node);

	if ((node->rt_rb_left = left->rt_rb_right))
		rt_rb_set_parent(left->rt_rb_right, node);
	left->rt_rb_right = node;

	rt_rb_set_parent(left, parent);

	if (parent)
	{
		if (node == parent->rt_rb_right)
			parent->rt_rb_right = left;
		else
			parent->rt_rb_left = left;
	}
	else
		root->rt_rb_node = left;
	rt_rb_set_parent(node, left);
}

void rt_rb_insert_color(struct rt_rb_node *node, struct rt_rb_root *root)
{
	struct rt_rb_node *parent, *gparent;

	while ((parent = rt_rb_parent(node)) && rt_rb_is_red(parent))
	{
		gparent = rt_rb_parent(parent);

		if (parent == gparent->rt_rb_left)
		{
			{
				register struct rt_rb_node *uncle = gparent->rt_rb_right;
				if (uncle && rt_rb_is_red(uncle))
				{
					rt_rb_set_black(uncle);
					rt_rb_set_black(parent);
					rt_rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rt_rb_right == node)
			{
				register struct rt_rb_node *tmp;
				__rt_rb_rotate_left(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rt_rb_set_black(parent);
			rt_rb_set_red(gparent);
			__rt_rb_rotate_right(gparent, root);
		} else {
			{
				register struct rt_rb_node *uncle = gparent->rt_rb_left;
				if (uncle && rt_rb_is_red(uncle))
				{
					rt_rb_set_black(uncle);
					rt_rb_set_black(parent);
					rt_rb_set_red(gparent);
					node = gparent;
					continue;
				}
			}

			if (parent->rt_rb_left == node)
			{
				register struct rt_rb_node *tmp;
				__rt_rb_rotate_right(parent, root);
				tmp = parent;
				parent = node;
				node = tmp;
			}

			rt_rb_set_black(parent);
			rt_rb_set_red(gparent);
			__rt_rb_rotate_left(gparent, root);
		}
	}

	rt_rb_set_black(root->rt_rb_node);
}

static void __rt_rb_erase_color(struct rt_rb_node *node, struct rt_rb_node *parent,
			     struct rt_rb_root *root)
{
	struct rt_rb_node *other;

	while ((!node || rt_rb_is_black(node)) && node != root->rt_rb_node)
	{
		if (parent->rt_rb_left == node)
		{
			other = parent->rt_rb_right;
			if (rt_rb_is_red(other))
			{
				rt_rb_set_black(other);
				rt_rb_set_red(parent);
				__rt_rb_rotate_left(parent, root);
				other = parent->rt_rb_right;
			}
			if ((!other->rt_rb_left || rt_rb_is_black(other->rt_rb_left)) &&
			    (!other->rt_rb_right || rt_rb_is_black(other->rt_rb_right)))
			{
				rt_rb_set_red(other);
				node = parent;
				parent = rt_rb_parent(node);
			}
			else
			{
				if (!other->rt_rb_right || rt_rb_is_black(other->rt_rb_right))
				{
					rt_rb_set_black(other->rt_rb_left);
					rt_rb_set_red(other);
					__rt_rb_rotate_right(other, root);
					other = parent->rt_rb_right;
				}
				rt_rb_set_color(other, rt_rb_color(parent));
				rt_rb_set_black(parent);
				rt_rb_set_black(other->rt_rb_right);
				__rt_rb_rotate_left(parent, root);
				node = root->rt_rb_node;
				break;
			}
		}
		else
		{
			other = parent->rt_rb_left;
			if (rt_rb_is_red(other))
			{
				rt_rb_set_black(other);
				rt_rb_set_red(parent);
				__rt_rb_rotate_right(parent, root);
				other = parent->rt_rb_left;
			}
			if ((!other->rt_rb_left || rt_rb_is_black(other->rt_rb_left)) &&
			    (!other->rt_rb_right || rt_rb_is_black(other->rt_rb_right)))
			{
				rt_rb_set_red(other);
				node = parent;
				parent = rt_rb_parent(node);
			}
			else
			{
				if (!other->rt_rb_left || rt_rb_is_black(other->rt_rb_left))
				{
					rt_rb_set_black(other->rt_rb_right);
					rt_rb_set_red(other);
					__rt_rb_rotate_left(other, root);
					other = parent->rt_rb_left;
				}
				rt_rb_set_color(other, rt_rb_color(parent));
				rt_rb_set_black(parent);
				rt_rb_set_black(other->rt_rb_left);
				__rt_rb_rotate_right(parent, root);
				node = root->rt_rb_node;
				break;
			}
		}
	}
	if (node)
		rt_rb_set_black(node);
}

void rt_rb_erase(struct rt_rb_node *node, struct rt_rb_root *root)
{
	struct rt_rb_node *child, *parent;
	int color;

	if (!node->rt_rb_left)
		child = node->rt_rb_right;
	else if (!node->rt_rb_right)
		child = node->rt_rb_left;
	else
	{
		struct rt_rb_node *old = node, *left;

		node = node->rt_rb_right;
		while ((left = node->rt_rb_left) != NULL)
			node = left;

		if (rt_rb_parent(old)) {
			if (rt_rb_parent(old)->rt_rb_left == old)
				rt_rb_parent(old)->rt_rb_left = node;
			else
				rt_rb_parent(old)->rt_rb_right = node;
		} else
			root->rt_rb_node = node;

		child = node->rt_rb_right;
		parent = rt_rb_parent(node);
		color = rt_rb_color(node);

		if (parent == old) {
			parent = node;
		} else {
			if (child)
				rt_rb_set_parent(child, parent);
			parent->rt_rb_left = child;

			node->rt_rb_right = old->rt_rb_right;
			rt_rb_set_parent(old->rt_rb_right, node);
		}

		node->rt_rb_parent_color = old->rt_rb_parent_color;
		node->rt_rb_left = old->rt_rb_left;
		rt_rb_set_parent(old->rt_rb_left, node);

		goto color;
	}

	parent = rt_rb_parent(node);
	color = rt_rb_color(node);

	if (child)
		rt_rb_set_parent(child, parent);
	if (parent)
	{
		if (parent->rt_rb_left == node)
			parent->rt_rb_left = child;
		else
			parent->rt_rb_right = child;
	}
	else
		root->rt_rb_node = child;

 color:
	if (color == RB_BLACK)
		__rt_rb_erase_color(child, parent, root);
}

static void rt_rb_augment_path(struct rt_rb_node *node, rt_rb_augment_f func, void *data)
{
	struct rt_rb_node *parent;

up:
	func(node, data);
	parent = rt_rb_parent(node);
	if (!parent)
		return;

	if (node == parent->rt_rb_left && parent->rt_rb_right)
		func(parent->rt_rb_right, data);
	else if (parent->rt_rb_left)
		func(parent->rt_rb_left, data);

	node = parent;
	goto up;
}

/*
 * after inserting @node into the tree, update the tree to account for
 * both the new entry and any damage done by rebalance
 */
void rt_rb_augment_insert(struct rt_rb_node *node, rt_rb_augment_f func, void *data)
{
	if (node->rt_rb_left)
		node = node->rt_rb_left;
	else if (node->rt_rb_right)
		node = node->rt_rb_right;

	rt_rb_augment_path(node, func, data);
}

/*
 * before removing the node, find the deepest node on the rebalance path
 * that will still be there after @node gets removed
 */
struct rt_rb_node *rt_rb_augment_erase_begin(struct rt_rb_node *node)
{
	struct rt_rb_node *deepest;

	if (!node->rt_rb_right && !node->rt_rb_left)
		deepest = rt_rb_parent(node);
	else if (!node->rt_rb_right)
		deepest = node->rt_rb_left;
	else if (!node->rt_rb_left)
		deepest = node->rt_rb_right;
	else {
		deepest = rt_rb_next(node);
		if (deepest->rt_rb_right)
			deepest = deepest->rt_rb_right;
		else if (rt_rb_parent(deepest) != node)
			deepest = rt_rb_parent(deepest);
	}

	return deepest;
}

/*
 * after removal, update the tree to account for the removed entry
 * and any rebalance damage.
 */
void rt_rb_augment_erase_end(struct rt_rb_node *node, rt_rb_augment_f func, void *data)
{
	if (node)
		rt_rb_augment_path(node, func, data);
}

/*
 * This function returns the first node (in sort order) of the tree.
 */
struct rt_rb_node *rt_rb_first(const struct rt_rb_root *root)
{
	struct rt_rb_node	*n;

	n = root->rt_rb_node;
	if (!n)
		return NULL;
	while (n->rt_rb_left)
		n = n->rt_rb_left;
	return n;
}

struct rt_rb_node *rt_rb_last(const struct rt_rb_root *root)
{
	struct rt_rb_node	*n;

	n = root->rt_rb_node;
	if (!n)
		return NULL;
	while (n->rt_rb_right)
		n = n->rt_rb_right;
	return n;
}

struct rt_rb_node *rt_rb_next(const struct rt_rb_node *node)
{
	struct rt_rb_node *parent;

	if (rt_rb_parent(node) == node)
		return NULL;

	/* If we have a right-hand child, go down and then left as far
	   as we can. */
	if (node->rt_rb_right) {
		node = node->rt_rb_right; 
		while (node->rt_rb_left)
			node=node->rt_rb_left;
		return (struct rt_rb_node *)node;
	}

	/* No right-hand children.  Everything down and left is
	   smaller than us, so any 'next' node must be in the general
	   direction of our parent. Go up the tree; any time the
	   ancestor is a right-hand child of its parent, keep going
	   up. First time it's a left-hand child of its parent, said
	   parent is our 'next' node. */
	while ((parent = rt_rb_parent(node)) && node == parent->rt_rb_right)
		node = parent;

	return parent;
}

struct rt_rb_node *rt_rb_prev(const struct rt_rb_node *node)
{
	struct rt_rb_node *parent;

	if (rt_rb_parent(node) == node)
		return NULL;

	/* If we have a left-hand child, go down and then right as far
	   as we can. */
	if (node->rt_rb_left) {
		node = node->rt_rb_left; 
		while (node->rt_rb_right)
			node=node->rt_rb_right;
		return (struct rt_rb_node *)node;
	}

	/* No left-hand children. Go up till we find an ancestor which
	   is a right-hand child of its parent */
	while ((parent = rt_rb_parent(node)) && node == parent->rt_rb_left)
		node = parent;

	return parent;
}

void rt_rb_replace_node(struct rt_rb_node *victim, struct rt_rb_node *new,
		     struct rt_rb_root *root)
{
	struct rt_rb_node *parent = rt_rb_parent(victim);

	/* Set the surrounding nodes to point to the replacement */
	if (parent) {
		if (victim == parent->rt_rb_left)
			parent->rt_rb_left = new;
		else
			parent->rt_rb_right = new;
	} else {
		root->rt_rb_node = new;
	}
	if (victim->rt_rb_left)
		rt_rb_set_parent(victim->rt_rb_left, new);
	if (victim->rt_rb_right)
		rt_rb_set_parent(victim->rt_rb_right, new);

	/* Copy the pointers/colour from the victim to the replacement */
	*new = *victim;
}

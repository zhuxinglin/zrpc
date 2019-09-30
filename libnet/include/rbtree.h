/*
*
*
*
*
*
*
*
*
*
* 
*/

#ifndef __RB_TREE_H__
#define __RB_TREE_H__

// http://www.bbniu.com/matrix/ShowApplication.aspx?id=149
/*
https://sandbox.runjs.cn/show/2nngvn8w
红黑树的5条性质：
1）每个结点要么是红的，要么是黑的。
2）根结点是黑的。
3）每个叶结点（叶结点即指树尾端NIL指针或NULL结点）是黑的。
4）如果一个结点是红的，那么它的俩个儿子都是黑的。
5）对于任一结点而言，其到叶结点树尾端NIL指针的每一条路径都包含相同数目的黑结点。
*/
#include <stdio.h>
#include "memory_pool.h"

namespace znet
{

#define GET_NUMBER(ptr, type, number) (type *)((char *)ptr - (long)&(((type *)0)->number))

template <typename K, typename V>
struct Iterator
{
	K first;
	V second;
};

template <typename K, typename V>
class rbtree
{
public:
	typedef struct Iterator<K, V> iterator;

private:
	struct rb
	{
		struct rb* left;
		struct rb* right;
		struct rb* parent;
		char color;
		iterator it;
	};

public:
	rbtree() : root(0),
			 min(0),
			 max(0),
			 count(0),
			 m_oPool(sizeof(rb), 1000000)
  	{
	}

	~rbtree()
	{clear();}

public:
	iterator* find(K& key)
	{
		struct rb* tmp = root;
		while (tmp)
		{
			if (tmp->it.first > key)
				tmp = tmp->left;
			else if (tmp->it.first < key)
				tmp = tmp->right;
			else
				break;
		}

		if (!tmp)
			return 0;

		return &(tmp->it);
	}

	void print()
	{
		struct rb *tmp = root;
		while (tmp && tmp->left)
			tmp = tmp->left;
		
		while (tmp)
		{
			printf("%lu s:%p l:%p r:%p p:%p %d==>", tmp->it.first, tmp, tmp->left, tmp->right, tmp->parent, tmp->color);
			if (tmp->right)
				tmp = min_rb(tmp->right);
			else if (tmp->parent && tmp == tmp->parent->left)
				tmp = tmp->parent;
			else
			{
				while (tmp->parent && tmp == tmp->parent->right)
					tmp = tmp->parent;
				tmp = tmp->parent;
			}
		}
		printf("\n\n");
	}

	iterator *insert(K& key, V &val)
	{
		if (!root)
		{
			root = new_rb(key, val, 0);
			if (!root)
				return 0;

			// 如果插入的是根结点，因为原树是空树，此情况只会违反性质2，所以直接把此结点涂为黑色。
			root->color = rb_BLACK;
			min = root;
			max = root;
			count++;
			return &root->it;
		}

		struct rb* node;
		struct rb* tmp = root;
		while (tmp)
		{
			if (tmp->it.first > key)
			{
				if (!tmp->left)
				{
					tmp->left = new_rb(key, val, tmp);
					node = tmp->left;
					if (!tmp->left)
						return 0;

					tmp = tmp->left;
					if (min->it.first > key)
						min = tmp;
					break;
				}
				else
					tmp = tmp->left;
			}
			else if (tmp->it.first < key)
			{
				if (!tmp->right)
				{
					tmp->right = new_rb(key, val, tmp);
					node = tmp->right;
					if (!tmp->right)
						return 0;

					tmp = tmp->right;
					if (max->it.first < key)
						max = tmp;
					break;
				}
				else
					tmp = tmp->right;
			}
			else
				return 0;
		}
		insert_balance_tree(tmp);
		count++;
		return &node->it;
	}

	int erase(K& key)
	{
		struct rb* del;
		del = find_node(key);
		if (!del)
			return -1;

		erase(del);
		return 0;
	}

	iterator* erase(iterator* cur)
	{
		if (!cur)
			return nullptr;

		iterator* tmp = begin(cur);

		struct rb* del = GET_NUMBER(cur, struct rb, it);
		erase(del);
		return tmp;
	}

	iterator* begin(iterator* cur)
	{
		if (!min)
			return nullptr;

		if (!cur)
			cur = &(min->it);
		else
		{
			// 中序遍历
			struct rb* tmp = GET_NUMBER(cur, struct rb, it);
			if (tmp->right)
				tmp = min_rb(tmp->right);
			else if (tmp->parent && tmp == tmp->parent->left)
				tmp = tmp->parent;
			else
			{
				while (tmp->parent && tmp == tmp->parent->right)
					tmp = tmp->parent;
				tmp = tmp->parent;
			}

			if (!tmp)
				return nullptr;

			cur = &(tmp->it);
		}
		return cur;
	}

	iterator* end(iterator* cur)
	{
		if (!max)
			return nullptr;

		if (!cur)
			cur = &(max->it);
		else
		{
			struct rb* tmp = GET_NUMBER(cur, struct rb, it);
			if (tmp->left)
				tmp = max_rb(tmp->left);
			else if (tmp->parent && tmp == tmp->parent->right)
				tmp = tmp->parent;
			else
			{
				while (tmp->parent && tmp == tmp->parent->left)
					tmp = tmp->parent;
				tmp = tmp->parent;
			}

			if (!tmp)
				return nullptr;

			cur = &(tmp->it);
		}
		return cur;
	}

	int size(){return count;}

	void clear()
	{
		// test
		struct rb* tmp = root;
		while (tmp)
		{
			if (tmp->left)
			{
				tmp = tmp->left;
				continue;
			}

			if (tmp->right)
			{
				tmp = tmp->right;
				continue;
			}

			if (tmp->parent)
			{
				if (tmp == tmp->parent->left)
					tmp->parent->left = 0;
				else
					tmp->parent->right = 0;
			}
			struct rb* del = tmp;
			tmp = tmp->parent;
			m_oPool.Free(del);
		}

		root = 0;
		max = 0;
		min = 0;
		count = 0;
	}

private:
	struct rb* min_rb(struct rb* t)
	{
		while (t->left)
			t = t->left;
		return t;
	}

	struct rb* max_rb(struct rb* t)
	{
		while (t->right)
			t = t->right;
		return t;
	}

	struct rb* new_rb(K& key, V& val, struct rb* patent)
	{
		struct rb* tmp = (struct rb*)m_oPool.Malloc();
		if (!tmp)
			return 0;
		tmp->left = 0;
		tmp->right = 0;
		tmp->parent = patent;
		tmp->it.first = key;
		tmp->it.second = val;
		tmp->color = rb_RED;
		return tmp;
	}

	/*
			 P           P             P                 P
			/ \         / \           / \               / \
		   pi  d       y   d         d   pi            d   y
		  /  \    ==> / \               /  \    ==>       / \
		 a    y      pi  c             a    y            pi  c
		/ \    / \                    / \          /  \
	   b   c  a   b                  b   c        a    b
	*/
	struct rb* left_rotate(struct rb* pi)
	{
		struct rb* y = pi->right;
		struct rb* p = pi->parent;
		if (!p)
			root = y;
		else
		{
			if (pi == p->left)
				p->left = y;
			else
				p->right = y;
		}
		y->parent = p;
		pi->right = y->left;
		if (y->left)
			y->left->parent = pi;
		y->left = pi;
		pi->parent = y;
		return pi;
	}

	/*
			 P               P				       P                 P
			/ \             / \                  / \               / \
		   d  pi           d   y                pi  d             y   d
		     /  \   ==>       / \              /  \              / \
			y    a           b   pi           y    a      ==>   b   pi
		   / \                  /  \         / \                   /  \
		  b   c                c    a       b   c                 c    a
	*/
	struct rb* right_rotate(struct rb* pi)
	{
		struct rb* y = pi->left;
		struct rb* p = pi->parent;
		if (!p)
			root = y;
		else
		{
			if (pi == p->right)
				p->right = y;
			else
				p->left = y;
		}
		y->parent = p;
		pi->left = y->right;
		if (y->right)
			y->right->parent = pi;
		y->right = pi;
		pi->parent = y;
		return pi;
	}

	/*
	1. 如果要删除的节点只有一个孩子，那么直接用孩子节点的值代替父节点的值。删除子节点就可以，不需要进入删除调整算法。
	2. 若当前要删除的节点两个孩子都不为空，此时我们只需要找到当前节点中序序列的后继节点。用后继节点的值替换当前节点的值。将后继节点作为新的当前节点，
	此时的当前节点一定只有一个右孩子或左右孩子都为空。
	3. 通过步骤2后如果当前节点有后继节点，直接用其后继节点值替换当前节点值，不需要进入删除调整算法。如果当前节点没有后继节点，进入删除调整算法。
	*/
	void erase(struct rb* del)
	{
		count--;
		if (min == del)
		{
			if (del->right)
				min = del->right;
			else
				min = del->parent;
		}

		if (del == max)
		{
			if (del->left)
				max = del->left;
			else
				max = del->parent;
		}

		struct rb* c, *p;
		char col;
		if (del->left && del->right)
		{
			// 将要删除的结点与找到右儿子结点中最小的数换位
			struct rb *swap = min_rb(del->right);

			if (del->parent)
			{
				if (del->parent->left == del)
					del->parent->left = swap;
				else
					del->parent->right = swap;
			}
			else
				root = swap;

			col = swap->color;
			c = swap->right;
			p = swap->parent;

			if (p == del)
				p = swap;
			else
			{
				if (c)
					c->parent = p;
				p->left = c;
				swap->right = del->right;
				del->right->parent = swap;
			}

			swap->parent = del->parent;
			swap->color = del->color;
			swap->left = del->left;
			del->left->parent = swap;
		}
		else
		{
			col = del->color;
			p = del->parent;
			if (!del->left)
				c = del->right;
			else
				c = del->left;

			if (c)
				c->parent = p;

			if (p)
			{
				if (del == p->left)
					p->left = c;
				else
					p->right = c;
			}
			else
				root = c;
		}

		if (!col)
			delete_balance_tree(c, p);
		m_oPool.Free(del);
	}

	/*
	删除修复情况1:当前结点是红+黑色
	解法，直接把当前结点染成黑色，结束此时红黑树性质全部恢复。
	删除修复情况2:当前结点是黑+黑且是根结点
	解法：什么都不做，结束。
	删除修复情况3：当前结点是黑+黑且兄弟结点为红色(此时父结点和兄弟结点的子结点分为黑)
	解法：把父结点染成红色，把兄弟结点染成黑色，将父结点做旋转，之后重新进入算法。
	此变换后原红黑树性质5不变，而把问题转化为兄弟结点为黑色的情况(注：变化前，原本就未违反性质5，只是为了把问题转化为兄弟结点为黑色的情况)
	删除修复情况4：当前结点是黑+黑且兄弟是黑色且兄弟结点的两个子结点全为黑色
	解法：把当前结点和兄弟结点中抽取一重黑色追加到父结点上，把父结点当成新的当前结点，重新进入算法。（此变换后性质5不变
	删除修复情况5：当前结点颜色是黑+黑，兄弟结点是黑色，兄弟的左子是红色，右子是黑色
	解法：把兄弟结点染红，兄弟左子(注:当前结点为父结点左结点情况)结点染黑，之后再在兄弟结点为支点解右旋，之后重新进入算法。此是把当前的情况转化为情况4，而性质5得以保持
	删除修复情况6：当前结点颜色是黑-黑色，它的兄弟结点是黑色，但是兄弟结点的右子是红色，兄弟结点左子的颜色任意
	解法：把兄弟结点染成当前结点父结点的颜色，把当前结点父结点染成黑色，兄弟结点右子染成黑色，之后以当前结点的父结点为支点进行左旋，此时算法结束，红黑树所有性质调整正确
	*/
	void delete_balance_tree(struct rb* bal, struct rb* parent)
	{
		while ((!bal || !bal->color) && bal != root)
		{
			struct rb* child;
			if (bal == parent->left)
			{
				child = parent->right;
				// 删除修复情况3：
				if (rb_RED == child->color)
				{
					parent->color = rb_RED;
					child->color = rb_BLACK;
					left_rotate(parent);
					child = parent->right;
				}

				// 删除修复情况4：
				if ((!child->left || rb_BLACK == child->left->color) &&
					(!child->right || rb_BLACK == child->right->color))
				{
					child->color = rb_RED;
					bal = parent;
					parent = bal->parent;
					continue;
				}

				// 删除修复情况5:
				if (!child->right || rb_BLACK == child->right->color)
				{
					child->color = rb_RED;
					child->left->color = rb_BLACK;
					right_rotate(child);
					child = parent->right;
				}

				// 删除修复情况6:
				child->color = parent->color;
				parent->color = rb_BLACK;
				child->right->color = rb_BLACK;

				left_rotate(parent);
				bal = root;
				break;
			}
			else
			{
				child = parent->left;
				// 删除修复情况3：
				if (rb_RED == child->color)
				{
					parent->color = rb_RED;
					child->color = rb_BLACK;
					right_rotate(parent);
					child = parent->left;
				}

				// 删除修复情况4：
				if ((!child->left || rb_BLACK == child->left->color) &&
					(!child->right || rb_BLACK == child->right->color))
				{
					child->color = rb_RED;
					bal = parent;
					parent = bal->parent;
					continue;
				}

				// 删除修复情况5:
				if (!child->left || rb_BLACK == child->left->color)
				{
					child->color = rb_RED;
					child->right->color = rb_BLACK;
					left_rotate(child);
					child = parent->left;
				}

				// 删除修复情况6:
				child->color = parent->color;
				parent->color = rb_BLACK;
				child->left->color = rb_BLACK;

				right_rotate(parent);
				bal = root;
				break;
			}
		}
		if (bal)
			bal->color = rb_BLACK;
	}

	struct rb* find_node(K& key)
	{
		struct rb* tmp = root;
		while (tmp)
		{
			if (tmp->it.first > key)
				tmp = tmp->left;
			else if (tmp->it.first < key)
				tmp = tmp->right;
			else
				break;
		}
		return tmp;
	}

	/*
	情况1：插入的是根结点。
	原树是空树，此情况只会违反性质2。
	对策：直接把此结点涂为黑色。
	情况2：插入的结点的父结点是黑色。
	此不会违反性质2和性质4，红黑树没有被破坏。
	对策：什么也不做。
	情况3：当前结点的父结点是红色且祖父结点的另一个子结点（叔叔结点）是红色。
	此时父结点的父结点一定存在，否则插入前就已不是红黑树。
	与此同时，又分为父结点是祖父结点的左子还是右子，对于对称性，我们只要解开一个方向就可以了。

	在此，我们只考虑父结点为祖父左子的情况。
	同时，还可以分为当前结点是其父结点的左子还是右子，但是处理方式是一样的。我们将此归为同一类。
	对策：将当前节点的父节点和叔叔节点涂黑，祖父结点涂红，把当前结点指向祖父节点，从新的当前节点重新开始算法。
	情况4：当前节点的父节点是红色,叔叔节点是黑色，当前节点是其父节点的右子
	对策：当前节点的父节点做为新的当前节点，以新当前节点为支点左旋。
	情况5：当前节点的父节点是红色,叔叔节点是黑色，当前节点是其父节点的左子
	解法：父节点变为黑色，祖父节点变为红色，在祖父节点为支点右旋
	*/
	void insert_balance_tree(struct rb* tr)
	{
		// 如果插入的结点的父结点是黑色，由于此不会违反性质2和性质4，红黑树没有被破坏，所以此时也是什么也不做。
//		if (tr->parent->color == rb_BLACK)
//			return;

		/*
		修复插入情况1：如果当前结点的父结点是红色且祖父结点的另一个子结点（叔结点）是红色
		修复插入情况2：当前结点的父结点是红色,叔叔结点是黑色，当前结点是其父结点的右子
		修复插入情况3：当前结点的父结点是红色,叔叔结点是黑色，当前结点是其父结点的左子
		*/

		struct rb* uncle;
		struct rb* parent;
		struct rb* gparent;
		while ((parent = tr->parent) && (parent->color == rb_RED))
		{
			// 父结点是红色
			gparent = parent->parent;
			// 由性质2：祖父结点存在
			if (parent == gparent->left)
			{
				uncle = gparent->right;

				// 修复插入情况1
				if (uncle && uncle->color == rb_RED) {
					// 对策：将当前结点的父结点和叔叔结点涂黑，祖父结点涂红，把当前结点指向祖父结点，从新的当前结点重新开始算法
					parent->color = rb_BLACK;
					uncle->color = rb_BLACK;
					gparent->color = rb_RED;
					tr = gparent;
					continue;
				}
				if (tr == parent->right)
				{
					uncle = tr;
					// 修复插入情况2
					// 对策：当前结点的父结点做为新的当前结点，以新当前结点为支点左旋，也不需要做颜色改动
					tr = left_rotate(parent); // y = parent, tr = c (旋转后是情况3)
					parent = uncle;
				}

				// 修复插入情况3
				// 解法：父结点变为黑色，祖父结点变为红色，在祖父结点为支点右旋
				// 最后，把根结点涂为黑色，整棵红黑树便重新恢复了平衡
				parent->color = rb_BLACK;
				gparent->color = rb_RED;
				right_rotate(gparent);
			}
			else
			{
				uncle = gparent->left;
				// 修复插入情况1
				if (uncle && uncle->color == rb_RED)
				{
					// 对策：将当前结点的父结点和叔叔结点涂黑，祖父结点涂红，把当前结点指向祖父结点，从新的当前结点重新开始算法
					parent->color = rb_BLACK;
					uncle->color = rb_BLACK;
					gparent->color = rb_RED;
					tr = gparent;
					continue;
				}
				if (tr == parent->left)
				{
					uncle = tr;
					// 修复插入情况2
					// 对策：当前结点的父结点做为新的当前结点，以新当前结点为支点左旋，也不需要做颜色改动
					tr = right_rotate(parent); // y = parent, tr = c (旋转后是情况3)
					parent = uncle;
				}

				// 修复插入情况3
				// 解法：父结点变为黑色，祖父结点变为红色，在祖父结点为支点右旋
				// 最后，把根结点涂为黑色，整棵红黑树便重新恢复了平衡
				parent->color = rb_BLACK;
				gparent->color = rb_RED;
				left_rotate(gparent);
			}
		}
		root->color = rb_BLACK;
	}

private:
	enum
	{
		rb_BLACK,
		rb_RED,
	};

private:
	struct rb* root;
	struct rb* min;
	struct rb* max;
	int count;
	CMemoryPool m_oPool;
};

}

#endif

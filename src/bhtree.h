#ifndef __BHTREE_H__
#define __BHTREE_H__

#include <stddef.h>

#include "pool.h"

#include <stddef.h>

#define THETA_SQUARED 0.25

enum bh_node_type { LEAF = 0, INTERNAL, EMPTY };

// Octree indexing:
//   5   4
// 6   7
//
//   1   0
// 2   3

struct bh_vec3 {
  double x, y, z;
};

struct bh_node {
  enum bh_node_type type;
  struct bh_vec3 cm;
  double mass;
  struct bh_node *children[8];
};

struct bh_tree {
  struct bh_node *root;
  struct pool node_pool;
  struct bh_vec3 bb_min, bb_max; // Bounding box for tree
};

int bh_tree_clear(struct bh_tree *tree);

int bh_tree_set_bb(struct bh_tree *tree, struct bh_vec3 bb_min,
                   struct bh_vec3 bb_max);

int bh_tree_init(struct bh_tree *tree, size_t num_nodes);

int bh_tree_insert(struct bh_tree *tree, struct bh_vec3 p, double mass);

void bh_tree_solve_acc(struct bh_tree *tree, struct bh_vec3 const *p,
                       struct bh_vec3 *result);

#endif // __BHTREE_H__


#include "bhtree.h"

#include <math.h>

#define MINDIST 0.001

int bh_tree_clear(struct bh_tree *tree)
{
  pool_releaseall(&tree->node_pool);

  if ((tree->root = pool_acquire(&tree->node_pool)) == NULL) { return -1; }

  *(tree->root) =
      (struct bh_node){.type = EMPTY, .cm = {0.0, 0.0, 0.0}, .mass = 0.0};

  for (size_t i = 0; i < 8; ++i) {
    tree->root->children[i] = NULL;
  }

  return 0;
}

int bh_tree_set_bb(struct bh_tree *tree, struct bh_vec3 bb_min,
                   struct bh_vec3 bb_max)
{
  if (bb_min.x >= bb_max.x || bb_min.y >= bb_max.y || bb_min.z >= bb_max.z) {
    return -1;
  }

  double min_dim = fmin(bb_min.x, fmin(bb_min.y, bb_min.z));
  double max_dim = fmax(bb_max.x, fmax(bb_max.y, bb_max.z));
  tree->bb_min = (struct bh_vec3){min_dim, min_dim, min_dim};
  tree->bb_max = (struct bh_vec3){max_dim, max_dim, max_dim};

  return 0;
}

int bh_tree_init(struct bh_tree *tree, size_t num_nodes)
{
  int err;

  if ((err = pool_init(&tree->node_pool, 10*num_nodes, sizeof(struct bh_node))) !=
      0) {
    return err;
  }

  bh_tree_clear(tree);

  return 0;
}

int bh_tree_insert_impl(struct bh_tree *tree, struct bh_node *cur,
                        struct bh_vec3 bb_min, struct bh_vec3 bb_max,
                        struct bh_vec3 p, double mass)
{
  int err;

  if (cur->type != EMPTY) {
    if (cur->type == LEAF) {
      if (p.x == cur->cm.x && p.y == cur->cm.y && p.z == cur->cm.z) {
        cur->mass += mass;
        return 0;
      }
      cur->type = INTERNAL;
      if ((err = bh_tree_insert_impl(tree, cur, bb_min, bb_max, cur->cm,
                                     cur->mass)) != 0) {
        return err;
      }
    }

    struct bh_vec3 midpoint = {.x = (bb_max.x + bb_min.x) / 2.0,
                               .y = (bb_max.y + bb_min.y) / 2.0,
                               .z = (bb_max.z + bb_min.z) / 2.0};

    // Figure out which octant point lies in, and update bounding box
    int octant = -1;
    if (p.z < midpoint.z) { // Bottom layer of cube
      bb_max.z = midpoint.z;
      if (p.y < midpoint.y) {
        bb_max.y = midpoint.y;
        if (p.x < midpoint.x) {
          bb_max.x = midpoint.x;
          octant = 2;
        }
        else {
          bb_min.x = midpoint.x;
          octant = 3;
        }
      }
      else {
        bb_min.y = midpoint.y;
        if (p.x < midpoint.x) {
          bb_max.x = midpoint.x;
          octant = 1;
        }
        else {
          bb_min.x = midpoint.x;
          octant = 0;
        }
      }
    }
    else { // Top layer of cube
      bb_min.z = midpoint.z;
      if (p.y < midpoint.y) {
        bb_max.y = midpoint.y;
        if (p.x < midpoint.x) {
          bb_max.x = midpoint.x;
          octant = 6;
        }
        else {
          bb_min.x = midpoint.x;
          octant = 7;
        }
      }
      else {
        bb_min.y = midpoint.y;
        if (p.x < midpoint.x) {
          bb_max.x = midpoint.x;
          octant = 5;
        }
        else {
          bb_min.x = midpoint.x;
          octant = 4;
        }
      }
    }

    // Populate relevant child if null
    if (cur->children[octant] == NULL) {
      if ((cur->children[octant] = pool_acquire(&tree->node_pool)) == NULL) {
        return -1;
      }
      cur->children[octant]->type = EMPTY;
    }

    cur->type = INTERNAL;

    // Recurse
    if ((err = bh_tree_insert_impl(tree, cur->children[octant], bb_min, bb_max,
                                   p, mass)) != 0) {
      return err;
    }

    // Update our centre of mass
    cur->mass = 0;
    cur->cm = (struct bh_vec3){0.0, 0.0, 0.0};
    for (octant = 0; octant < 8; ++octant) {
      if (cur->children[octant] != NULL) {
        cur->cm.x += cur->children[octant]->cm.x;
        cur->cm.y += cur->children[octant]->cm.y;
        cur->cm.z += cur->children[octant]->cm.z;
        cur->mass += cur->children[octant]->mass;
      }
    }
    cur->cm.x /= cur->mass;
    cur->cm.y /= cur->mass;
    cur->cm.z /= cur->mass;
  }
  else {
    cur->type = LEAF;
    cur->cm = p;
    cur->mass = mass;
    for (size_t i = 0; i < 8; ++i) {
      cur->children[i] = NULL;
    }
  }
  return 0;
}

int bh_tree_insert(struct bh_tree *tree, struct bh_vec3 p, double mass)
{
  return bh_tree_insert_impl(tree, tree->root, tree->bb_min, tree->bb_max, p,
                             mass);
}

void bh_tree_solve_acc_impl(struct bh_node *node, struct bh_vec3 const *p,
                            struct bh_vec3 *result, double cube_dim)
{
  double distsq = (node->cm.x - p->x) * (node->cm.x - p->x) +
                  (node->cm.y - p->y) * (node->cm.y - p->y) +
                  (node->cm.z - p->z) * (node->cm.z - p->z);
  if (node->type == LEAF || (cube_dim / distsq < THETA_SQUARED)) {
    double dist3 = sqrt(distsq);
    if (dist3 < MINDIST) dist3 = MINDIST;
    dist3 = dist3 * dist3 * dist3;
    result->x += (node->cm.x - p->x) * node->mass / dist3;
    result->y += (node->cm.y - p->y) * node->mass / dist3;
    result->z += (node->cm.z - p->z) * node->mass / dist3;
  }
  else {
    for (size_t octant = 0; octant < 8; ++octant) {
      if (node->children[octant] != NULL) {
        bh_tree_solve_acc_impl(node->children[octant], p, result,
                               cube_dim / 2.0);
      }
    }
  }
}

void bh_tree_solve_acc(struct bh_tree *tree, struct bh_vec3 const *p,
                       struct bh_vec3 *result)
{
  result->x = result->y = result->z = 0.0;
  bh_tree_solve_acc_impl(tree->root, p, result,
                         tree->bb_max.x - tree->bb_min.x);
}


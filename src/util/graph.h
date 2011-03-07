/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 */

#ifndef GRAPH_H
#define GRAPH_H
#ifdef __cplusplus
extern "C" {
#endif

  typedef struct node {
    int marker;
    void *object;
  } node;

  typedef struct edge {
    int marker;
    node *node1;
    node *node2;
  } edge;

  typedef struct graph {
    vset nodes;
    vset edges;
  } graph;

#ifdef __cplusplus
}
#endif
#endif

/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 $Id: graph.c,v 1.1 2001/04/11 17:28:07 corwin Exp $
 */

/* This is a very simple graph library. It is not optimized in any
   way, and relies heavily on the vset and stack routines. */

#include <stdlib.h>
#include "vset.h"
#include "graph.h"

void
graph_init(graph *g)
{
	g->nodes    = malloc(sizeof(vset));
	g->vertices = malloc(sizeof(vset));
	vset_init(g->nodes);
	vset_init(g->vertices);
}

void
graph_free(graph *g)
{
	vset_destroy(g->nodes);
	vset_destroy(g->vertices);
	free(g->nodes);
	free(g->vertices);
}

void
graph_add_node(graph *g, node *n)
{
	vset_add(g->nodes, n);
}

void
graph_add_vertex(graph *g, vertex *v)
{
	vset_add(g->nodes, v);
}

void
graph_remove_node(graph *g, node *n)
{
	int i;

	for(i=0; i != g->vertices->size; ++i) {
		vertex *v = g->vertices->data[i];
		if(v->node1 == n || v->node2 == n) {
			vset_erase(g->nodes, v);
			i--;
		}
	}
	vset_erase(g->nodes, n);
}

node *
graph_find_node(graph *g, void *object)
{
	int i;

	for(i=0; i != g->nodes->size; ++i) {
	  	node *node = g->nodes->data[i];
		if(node->object == object) {
			return g->nodes->data[i];
		}
	}
	return NULL;
}

/* The vset returned has to freed externally, else this will be a
   memory leak. */

vset *
graph_neighbours(graph *g, node *n)
{
	int i;
	vset *vs = malloc(sizeof(vset));
	vset_init(vs);
	for(i=0; i != g->vertices->size; ++i) {
		vertex *v =  g->vertices->data[i];
		if(v->node1 == n && vset_count(vs, v->node2) == 0) {
			vset_add(vs, v->node2);
		} else if(v->node2 == n && vset_count(vs, v->node1) == 0) {
			vset_add(vs, v->node1);
		}
	}

	return vs;
}

void
graph_resetmarkers(graph *g)
{
	int i;

	for(i=0; i != g->nodes->size; ++i) {
	  	node *node = g->nodes->data[i];
		node->marker = 0;
	}
	for(i=0; i != g->vertices->size; ++i) {
	  	vertex *vertex = g->vertices->data[i];
		vertex->marker = 0;
	}
}

/* The vset returned has to freed externally, else this will be a
   memory leak. */

vset *
graph_connected_nodes(graph *g, node *n)
{
	vset *vs = malloc(sizeof(vset));
	vset *s  = malloc(sizeof(vset));

	vset_init(vs);
	vset_init(s);
	graph_resetmarkers(g);
	vset_add(vs, n);
	n->marker = 1;
	vset_add(s, n);
	while(s->size > 0) {
		node *n     = vset_pop(s);
		vset *vs_nb = graph_neighbours(g, n);
		int  i;
		for(i=0; i != vs_nb->size; ++i) {
			node *nb = vs_nb->data[i];
			if(nb->marker == 0) {
				nb->marker == 1;
				vset_add(vs, nb);
				vset_add(s, nb);
			}
		}
		vset_destroy(vs_nb);
		free(vs_nb);
	}

	vset_destroy(s);
	free(s);

	return vs;
}

/* The vset returned has to freed externally, else this will be a
   memory leak. */

vset *
graph_disjunct_graphs(graph *g)
{
  	vset *dg    = malloc(sizeof(vset));
	vset *nodes = malloc(sizeof(vset));

	vset_init(nodes);
	vset_concat(nodes, g->nodes);
	
	while(nodes->size > 0) {
		graph *gt = malloc(sizeof(graph));
		vset  s;
		int   i;
		node  *start;

		graph_init(gt);
		start = vset_pop(nodes);
		graph_resetmarkers(g);
		graph_add_node(gt, start);
		start->marker = 1;
		vset_init(&s);
		vset_add(&s,start);
		while(s.size > 0) {
			vset *nbs = graph_neighbours(g,vset_pop(&s));
			for(i=0; i != nbs->size; ++i) {
				node *nb = nbs->data[i];
				if(nb->marker == 0) {
					nb->marker = 1;
					graph_add_node(gt,nb);
					vset_erase(nodes,nb);
					vset_add(&s,nb);
				}
			}
			vset_destroy(nbs); free(nbs);
		}

		vset_destroy(&s);

		for(i=0;i != g->vertices->size; ++i) {
			vertex *v = g->vertices->data[i];
			if(vset_count(g->nodes, v->node1)) {
				graph_add_vertex(gt,v);
			}
		}

		vset_add(dg, gt);
	}

	vset_destroy(nodes);
	free(nodes);

	return dg;
}


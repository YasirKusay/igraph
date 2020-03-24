/* -*- mode: C -*-  */
/* vim:set ts=4 sw=4 sts=4 et: */
/*
   IGraph library.
   Copyright (C) 2005-2012  Gabor Csardi <csardi.gabor@gmail.com>
   334 Harvard street, Cambridge, MA 02139 USA

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA

*/

#include "igraph_eulerian.h"
#include "igraph_structural.h"
#include "igraph_transitivity.h"
#include "igraph_paths.h"
#include "igraph_math.h"
#include "igraph_memory.h"
#include "igraph_random.h"
#include "igraph_adjlist.h"
#include "igraph_interface.h"
#include "igraph_progress.h"
#include "igraph_interrupt_internal.h"
#include "igraph_centrality.h"
#include "igraph_components.h"
#include "igraph_constructors.h"
#include "igraph_conversion.h"
#include "igraph_types_internal.h"
#include "igraph_dqueue.h"
#include "igraph_attributes.h"
#include "igraph_neighborhood.h"
#include "igraph_topology.h"
#include "igraph_qsort.h"
#include "config.h"
#include "structural_properties_internal.h"

#include <assert.h>

/* solution adapted from https://www.geeksforgeeks.org/eulerian-path-and-circuit/
The function returns one of the following values
0 -> if graph is not Eulerian
1 -> if graph has an euler circuit
2 -> if graph has an euler path */
int is_eulerian_undirected(igraph_t *graph) {

    igraph_bool_t res;
    igraph_inclist_t il;
    igraph_vector_int_t *incedges;
    igraph_integer_t odd, i;

    odd = 0;

    igraph_is_connected(graph, &res, IGRAPH_WEAK);
    if (res == 0) return 0;

    IGRAPH_CHECK(igraph_inclist_init(graph, &il, IGRAPH_ALL));
    IGRAPH_FINALLY(igraph_inclist_destroy, &il);

    for (i = 0; i < igraph_vcount(graph); i++) {
        incedges = igraph_inclist_get(&il, i);
        if ( igraph_vector_int_size(incedges) % 2 == 1) odd++;
    }

    igraph_inclist_destroy(&il);

    if (odd > 2) return 0;

    /* if pdd count is 2, then semi-eulerian
    if odd count is 0, then eulerian
    odd count never is 1 for undirected graph
    */

    return (odd == 2) ? 2 : 1;
}

int is_eulerian_directed(igraph_t *graph) {
    igraph_es_t incoming, outgoing;
    igraph_bool_t res_strong, res_weak;
    igraph_integer_t incoming_excess, incoming_count, outgoing_excess, outgoing_count;
    int i;

    incoming_excess = 0;
    outgoing_excess = 0;

    /* checking if incoming vertices == outgoing vertices */
    for (i = 0; i < igraph_vcount(graph); i++) {
        igraph_es_incident(&incoming, i, IGRAPH_IN);
        igraph_es_incident(&outgoing, i, IGRAPH_OUT);
        
        igraph_es_size(graph, &incoming, &incoming_count);
        igraph_es_size(graph, &outgoing, &outgoing_count);

        if (incoming_count != outgoing_count) {
            if ((incoming_count + 1 == outgoing_count) && (incoming_excess < 2 && outgoing_excess < 1)) {
                outgoing_excess++;
            } else if ((outgoing_count + 1 == incoming_count) && (incoming_excess < 1 && outgoing_excess < 2)) {
                incoming_excess++;
            } else {
                return 0;
            }
        }
    }

    igraph_is_connected(graph, &res_strong, IGRAPH_STRONG);
    igraph_is_connected(graph, &res_weak, IGRAPH_WEAK);

    if ((outgoing_excess == 0 && incoming_excess == 0) && (res_strong == 1)) {
        return 1;
    } else if ((outgoing_excess == 1 && incoming_excess == 1) && 
        (res_strong == 1 || res_weak == 1)) {
        return 2;
    }

    return 0;
}

/* flexible function */
int igraph_is_eulerian(igraph_t *graph) {
    if (igraph_is_directed(graph)) {
        return is_eulerian_directed(graph);
    } else {
        return is_eulerian_undirected(graph);
    } 
}

igraph_bool_t check_if_bridge(igraph_t *g, igraph_integer_t start, igraph_integer_t v, long edge) {

    igraph_vector_t bridges;
    igraph_integer_t i;

    IGRAPH_CHECK(igraph_vector_init(&bridges, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &bridges);
    igraph_bridges(g, &bridges);

    for (i = 0; i < igraph_vector_size(&bridges); i++) {
        if (VECTOR(bridges)[i] == edge) {
            igraph_vector_destroy(&bridges);
            return 0;   
        }
    }

    igraph_vector_destroy(&bridges);

    return 1;
}

int print_euler_undirected_implementation(igraph_integer_t start, igraph_t *g, igraph_vector_t *path) {

    igraph_integer_t i, v;
    igraph_inclist_t il;
    igraph_vector_int_t *incedges;
    igraph_es_t edge_to_delete;
    long nc, edge;

    /* printf("Hello 2\n"); */

    i = 0;

    while (1) {
        //igraph_inclist_init(g, &il, IGRAPH_ALL);
        IGRAPH_CHECK(igraph_inclist_init(g, &il, IGRAPH_ALL)); 
        IGRAPH_FINALLY(igraph_inclist_destroy, &il); 
        incedges = igraph_inclist_get(&il, start);
        nc = igraph_vector_int_size(incedges);

        if (nc == 0) break;

        edge = (long) VECTOR(*incedges)[i];
        v = IGRAPH_TO(g, edge) == start ? IGRAPH_FROM(g, edge) : IGRAPH_TO(g, edge);
        if (nc == 1 || check_if_bridge(g, start, v, edge)) {
            igraph_vector_push_back(path, v);
            /* printf("%d\n", v); */
            igraph_es_1(&edge_to_delete, edge);
            igraph_delete_edges(g, edge_to_delete);
            print_euler_undirected_implementation(v, g, path);
        }
        i++;
    }

    /* if (v == 0) printf("safe\n"); */

    igraph_inclist_destroy(&il);

    /* if (v == 0) printf("safe 2\n"); */

    return IGRAPH_SUCCESS;

}


/* solution adapted from https://www.geeksforgeeks.org/eulerian-path-and-circuit/ */
int igraph_euler_path_undirected(igraph_t *graph, igraph_vector_t *path) {

    /* default starting vertex is 0
    when we have no odd vertices, we can start anywhere (usually 0)
    when we have 2 odd vertices, we start at any one of them
    igraph_vector_t path must be initialised prior to its us */

    igraph_integer_t res, start, i;
    igraph_t copy; 
    igraph_inclist_t incl;
    igraph_vector_int_t *incedges;

    /* printf("Hello\n"); */

    res = igraph_is_eulerian(graph);

    if (res == 0) {
        IGRAPH_WARNING("Euler cycle not possible");
        return 1;
    }

    IGRAPH_CHECK(igraph_copy(&copy, graph));
    IGRAPH_FINALLY(igraph_destroy, &copy);
    /* making a copy of the graph
    this is because this algorithm will delete edges 
    therefore we do not want to modify our original graph */

    start = 0;

    if (res == 1) {
        igraph_vector_push_back(path, start);
        print_euler_undirected_implementation(start, &copy, path);
        /* printf("Hello 3\n"); */
    } else {
        IGRAPH_CHECK(igraph_inclist_init(graph, &incl, IGRAPH_ALL));
        IGRAPH_FINALLY(igraph_inclist_destroy, &incl);
        for (i = 0; i < igraph_vcount(graph); i++) {
            incedges = igraph_inclist_get(&incl, i);
            if ( igraph_vector_int_size(incedges) % 2 == 1) {
                start = i;
                break;
            }
        }
        igraph_vector_push_back(path, start);
        print_euler_undirected_implementation(start, &copy, path);
        /* printf("Hello 3"); */
    }

    /*

    printf("Hello 4\n");
    printf("jks\n");

    */

    igraph_inclist_destroy(&incl);

    /*

    printf("Hello 5\n");
    printf("jks 2\n");

    */

    igraph_destroy(&copy);

    /*

    printf("Hello 5");

    */

    return IGRAPH_SUCCESS;

}

/* solution adapted from https://www.geeksforgeeks.org/hierholzers-algorithm-directed-graph/ */
int eulerian_path_directed_implementation(igraph_t *graph, igraph_integer_t *start_node, igraph_vector_t *outgoing_list, igraph_vector_t *res) {
    igraph_integer_t start = *start_node;
    igraph_integer_t curr = start;
    igraph_integer_t next;
    igraph_inclist_t il;
    igraph_es_t es;
    igraph_stack_t path;
    igraph_stack_t tracker;
    igraph_vector_int_t *incedges;
    long nc;
    long edge;

    IGRAPH_CHECK(igraph_stack_init(&path, igraph_vcount(graph)));
    IGRAPH_FINALLY(igraph_stack_destroy, &path);

    IGRAPH_CHECK(igraph_stack_init(&tracker, igraph_vcount(graph)));
    IGRAPH_FINALLY(igraph_stack_destroy, &tracker);

    igraph_stack_push(&tracker, start);

    while (!igraph_stack_empty(&tracker)) {
        
        if (VECTOR(*outgoing_list)[curr] != 0) {
            igraph_stack_push(&tracker, curr);
            
            igraph_inclist_init(graph, &il, IGRAPH_ALL);
            incedges = igraph_inclist_get(&il, curr);
            nc = igraph_vector_int_size(incedges);
            edge = (long) VECTOR(*incedges)[0];
            
            next = IGRAPH_TO(graph, edge);

            /* remove edge here */
            VECTOR(*outgoing_list)[curr]--;
            igraph_es_1(&es, edge);
            igraph_delete_edges(graph, es);

            curr = next;
        } else { /* back track to find remaining circuit */
            igraph_stack_push(&path, curr);
            curr = igraph_stack_pop(&tracker);
        }
    }

    while (!igraph_stack_empty(&path)) {
        igraph_vector_push_back(res, igraph_stack_pop(&path));           
    }

    igraph_stack_destroy(&path);
    igraph_stack_destroy(&tracker);

    return IGRAPH_SUCCESS;
}


int igraph_eulerian_path_directed(igraph_t *graph, igraph_vector_t *res) {
    igraph_es_t incoming, outgoing;
    igraph_bool_t res_strong, res_weak;
    igraph_integer_t incoming_excess, incoming_count, outgoing_excess, outgoing_count;
    igraph_integer_t start_node;
    igraph_vector_t outgoing_list;
    igraph_t copy;
    int i;

    if (igraph_is_eulerian(graph) == 0) {
        IGRAPH_WARNING("Euler cycle not possible");
        return IGRAPH_FAILURE;
    }

    IGRAPH_CHECK(igraph_vector_init(&outgoing_list, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &outgoing_list);

    IGRAPH_CHECK(igraph_copy(&copy, graph));
    IGRAPH_FINALLY(igraph_destroy, &copy);

    incoming_excess = 0;
    outgoing_excess = 0;

    /* determining the start node */
    /* also getting the outgoing list for each vector*/
    for (i = 0; i < igraph_vcount(graph); i++) {
        igraph_es_incident(&incoming, i, IGRAPH_IN);
        igraph_es_incident(&outgoing, i, IGRAPH_OUT);
        
        igraph_es_size(graph, &incoming, &incoming_count);
        igraph_es_size(graph, &outgoing, &outgoing_count);

        igraph_vector_push_back(&outgoing_list, outgoing_count);

        if (incoming_count != outgoing_count) {
            if ((incoming_count + 1 == outgoing_count) && (incoming_excess < 2 && outgoing_excess < 1)) {
                outgoing_excess++;
                start_node = i;
                // printf("start node is %d\n", start_node);
            } else if ((outgoing_count + 1 == incoming_count) && (incoming_excess < 1 && outgoing_excess < 2)) {
                incoming_excess++;
            } 
        }
    }

    igraph_is_connected(graph, &res_strong, IGRAPH_STRONG);
    igraph_is_connected(graph, &res_weak, IGRAPH_WEAK);

    if ((outgoing_excess == 0 && incoming_excess == 0) && (res_strong == 1)) {
        start_node = 0;
        eulerian_path_directed_implementation(&copy, &start_node, &outgoing_list, res);
    } else if ((outgoing_excess == 1 && incoming_excess == 1) && 
        (res_strong == 1 || res_weak == 1)) {
        eulerian_path_directed_implementation(&copy, &start_node, &outgoing_list, res);
    }

    igraph_vector_destroy(&outgoing_list);
    igraph_destroy(&copy);

    return IGRAPH_SUCCESS;
}

int igraph_eulerian_paths(igraph_t *graph, igraph_vector_t *res) {
    if (igraph_is_directed(graph)) {
        return igraph_eulerian_path_directed(graph, res);
    } else {
        return igraph_euler_path_undirected(graph, res);
    } 
}
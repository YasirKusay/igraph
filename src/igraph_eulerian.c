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
#include "igraph_adjlist.h"
#include "igraph_interface.h"
#include "igraph_components.h"
#include "igraph_types_internal.h"

/**
 * \section about_eulerian
 *
 * <para>These functions calculate whether an Eulerian path or cycle exists
 * and if so, can find them.</para>
 */


/* solution adapted from https://www.geeksforgeeks.org/eulerian-path-and-circuit/
The function returns one of the following values
has_path is set to 1 if a path exists, 0 otherwise
has_cycle is set to 1 if a cycle exists, 0 otherwise
*/
int igraph_i_is_eulerian_undirected(const igraph_t *graph, igraph_bool_t *has_path, igraph_bool_t *has_cycle, igraph_integer_t *start_of_path) {
    igraph_integer_t odd;
    igraph_vector_t degree, csize;
    int i, cluster_count;

    if (igraph_ecount(graph) == 0 || igraph_vcount(graph) <= 1) {
        *has_path = 1;
        *has_cycle = 1;
        return  IGRAPH_SUCCESS;
    }

    IGRAPH_CHECK(igraph_vector_init(&csize, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &csize);

    IGRAPH_CHECK(igraph_clusters(graph, NULL, &csize, NULL, IGRAPH_WEAK));
    cluster_count = 0;
    for (i = 0; i < igraph_vector_size(&csize); i++) {
        if (VECTOR(csize)[i] > 1) cluster_count++;
    }

    if (cluster_count > 1) {
        *has_path = 0;
        *has_cycle = 0;
        return IGRAPH_SUCCESS;
    }

    odd = 0;

    IGRAPH_CHECK(igraph_vector_init(&degree, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &degree);

    IGRAPH_CHECK(igraph_degree(graph, &degree, igraph_vss_all(), IGRAPH_ALL, IGRAPH_LOOPS));

    for (i = 0; i < igraph_vector_size(&degree); i++) {
        if (((long int) VECTOR(degree)[i]) % 2 == 1) odd++;
    }

    if (odd > 2) {
        *has_path = 0;
        *has_cycle = 0;
    } else if (odd == 2) {
        *has_path = 1;
        *has_cycle = 0;
    } else {
        *has_path = 1;
        *has_cycle = 1;
    }

    /* CHECK LATER IN CASE OF BUGS */

    for (i = 0; i < igraph_vector_size(&degree); i++) {
        if ((*has_cycle && ((long int) VECTOR(degree)[i]) > 0) || (!*has_cycle && ((long int) VECTOR(degree)[i]) %2 == 1)) {
            *start_of_path = i;
            break;
        }
    }

    igraph_vector_destroy(&csize);
    igraph_vector_destroy(&degree);
    IGRAPH_FINALLY_CLEAN(2);

    return IGRAPH_SUCCESS;
}


int igraph_i_is_eulerian_directed(const igraph_t *graph, igraph_bool_t *has_path, igraph_bool_t *has_cycle, igraph_integer_t *start_of_path) {
    igraph_bool_t res_weak;
    igraph_integer_t incoming_excess, outgoing_excess, vector_count;
    int i, cluster_count;
    igraph_vector_t out_degree, in_degree, csize_weak;

    vector_count = igraph_vcount(graph);

    if (igraph_ecount(graph) == 0 || vector_count <= 1) {
        *has_path = 1;
        *has_cycle = 1;
        return IGRAPH_SUCCESS;
    }

    incoming_excess = 0;
    outgoing_excess = 0;

    res_weak = 1;

    IGRAPH_CHECK(igraph_vector_init(&out_degree, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &out_degree);
    IGRAPH_CHECK(igraph_degree(graph, &out_degree, igraph_vss_all(), IGRAPH_OUT, IGRAPH_LOOPS));

    IGRAPH_CHECK(igraph_vector_init(&in_degree, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &in_degree);
    IGRAPH_CHECK(igraph_degree(graph, &in_degree, igraph_vss_all(), IGRAPH_IN, IGRAPH_LOOPS));

    /* checking if incoming vertices == outgoing vertices */
    for (i = 0; i < vector_count; i++) {
        if (VECTOR(in_degree)[i] != VECTOR(out_degree)[i]) {
            if ((VECTOR(in_degree)[i] + 1 == VECTOR(out_degree)[i]) && (incoming_excess < 2 && outgoing_excess < 1)) {
                outgoing_excess++;
                *start_of_path = i;
            } else if ((VECTOR(out_degree)[i] + 1 == VECTOR(in_degree)[i]) && (incoming_excess < 1 && outgoing_excess < 2)) {
                incoming_excess++;
            } else {
                *has_path = 0;
                *has_cycle = 0;
                igraph_vector_destroy(&in_degree);
                igraph_vector_destroy(&out_degree);
                IGRAPH_FINALLY_CLEAN(2);

                return IGRAPH_SUCCESS;
            }
        }
    }

    IGRAPH_CHECK(igraph_vector_init(&csize_weak, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &csize_weak);

    IGRAPH_CHECK(igraph_clusters(graph, NULL, &csize_weak, NULL, IGRAPH_WEAK));
    cluster_count = 0;
    for (i = 0; i < igraph_vector_size(&csize_weak); i++) {
        if (VECTOR(csize_weak)[i] > 1) cluster_count++;
    }

    if (cluster_count > 1) {
        res_weak = 0;
    }

    if ((outgoing_excess == 0 && incoming_excess == 0) && (res_weak)) {
        *has_path = 1;
        *has_cycle = 1;

        for (i = 0; i < igraph_vector_size(&out_degree); i++) {
            if ((((long int) VECTOR(out_degree)[i]) > 0) || (((long int) VECTOR(in_degree)[i]) > 0)) {
                *start_of_path = i;
                break;
            }
        }
        
    igraph_vector_destroy(&csize_weak);
    igraph_vector_destroy(&in_degree);
    igraph_vector_destroy(&out_degree);
    IGRAPH_FINALLY_CLEAN(3);

    return IGRAPH_SUCCESS;

    } else if ((outgoing_excess == 1 && incoming_excess == 1) && (res_weak)) {
        *has_path = 1;
        *has_cycle = 0;

        igraph_vector_destroy(&csize_weak);
        igraph_vector_destroy(&in_degree);
        igraph_vector_destroy(&out_degree);
        IGRAPH_FINALLY_CLEAN(3);

        return IGRAPH_SUCCESS;
    }

    *has_path = 0;
    *has_cycle = 0;

    igraph_vector_destroy(&csize_weak);
    igraph_vector_destroy(&in_degree);
    igraph_vector_destroy(&out_degree);
    IGRAPH_FINALLY_CLEAN(3);

    return IGRAPH_SUCCESS;
}


/**
 * \ingroup Eulerian
 * \function igraph_is_eulerian
 * \brief Checks whether an Eulerian path or cycle exists
 *
 * An Eulerian path traverses each edge of the graph precisely once. A closed
 * Eulerian path is referred to as an Eulerian cycle.
 *
 * \param graph The graph object.
 * \param has_path Pointer to a Boolean, will be set to true if an Eulerian path exists.
 * \param has_cycle Pointer to a Boolean, will be set to true if an Eulerian cycle exists.
 * \return Error code:
 *         \c IGRAPH_ENOMEM, not enough memory for
 *         temporary data.
 *
 * Time complexity: O(|V|+|E|), the
 * number of vertices times the number of edges.
 *
 */

int igraph_is_eulerian(const igraph_t *graph, igraph_bool_t *has_path, igraph_bool_t *has_cycle) {
    igraph_integer_t start_of_path = 0;
    
    if (igraph_is_directed(graph)) {
        IGRAPH_CHECK(igraph_i_is_eulerian_directed(graph, has_path, has_cycle, &start_of_path));
    } else {
        IGRAPH_CHECK(igraph_i_is_eulerian_undirected(graph, has_path, has_cycle, &start_of_path));
    } 
    return IGRAPH_SUCCESS;
}


int igraph_i_eulerian_path_undirected(const igraph_t *graph, igraph_vector_t *res, igraph_integer_t start_of_path) {
    igraph_integer_t curr;
    igraph_integer_t next, edge_count, curr_e, vector_count;
    igraph_inclist_t il;
    igraph_stack_t path, tracker, edge_tracker, edge_path;
    igraph_vector_bool_t visited_list;
    igraph_vector_t degree;
    long edge;

    vector_count = igraph_vcount(graph);
    edge_count = igraph_ecount(graph);

    igraph_vector_clear(res);
    IGRAPH_CHECK(igraph_vector_reserve(res, edge_count));

    if (edge_count == 0 || vector_count == 0) {
        return IGRAPH_SUCCESS;
    }

    IGRAPH_CHECK(igraph_vector_init(&degree, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &degree);
    IGRAPH_CHECK(igraph_degree(graph, &degree, igraph_vss_all(), IGRAPH_ALL, IGRAPH_LOOPS));

    curr = start_of_path;

    IGRAPH_CHECK(igraph_stack_init(&path, vector_count));
    IGRAPH_FINALLY(igraph_stack_destroy, &path);

    IGRAPH_CHECK(igraph_stack_init(&tracker, vector_count));
    IGRAPH_FINALLY(igraph_stack_destroy, &tracker);

    IGRAPH_CHECK(igraph_stack_init(&edge_path, vector_count));
    IGRAPH_FINALLY(igraph_stack_destroy, &edge_path);

    IGRAPH_CHECK(igraph_stack_init(&edge_tracker, vector_count));
    IGRAPH_FINALLY(igraph_stack_destroy, &edge_tracker);

    IGRAPH_CHECK(igraph_vector_bool_init(&visited_list, edge_count));
    IGRAPH_FINALLY(igraph_vector_bool_destroy, &visited_list);

    IGRAPH_CHECK(igraph_stack_push(&tracker, start_of_path));

    IGRAPH_CHECK(igraph_inclist_init(graph, &il, IGRAPH_OUT));
    IGRAPH_FINALLY(igraph_inclist_destroy, &il);

    while (!igraph_stack_empty(&tracker)) {
        igraph_vector_int_t *incedges;
        long nc;
        
        if (VECTOR(degree)[curr] != 0) {
            int j;
            IGRAPH_CHECK(igraph_stack_push(&tracker, curr));
            
            incedges = igraph_inclist_get(&il, curr);
            nc = igraph_vector_int_size(incedges);

            for (j = 0; j < nc; j++) {
                edge = (long) VECTOR(*incedges)[j];
                if (!VECTOR(visited_list)[edge]) {
                    break;
                }
            }
            
            next = IGRAPH_OTHER(graph, edge, curr);

            IGRAPH_CHECK(igraph_stack_push(&edge_tracker, edge));

            /* remove edge here */
            VECTOR(degree)[curr]--;
            VECTOR(degree)[next]--;
            VECTOR(visited_list)[edge] = 1;

            curr = next;
        } else { /* back track to find remaining circuit */
            IGRAPH_CHECK(igraph_stack_push(&path, curr));
            curr = igraph_stack_pop(&tracker);
            if (!igraph_stack_empty(&edge_tracker)) {
                curr_e = igraph_stack_pop(&edge_tracker);
                IGRAPH_CHECK(igraph_stack_push(&edge_path, curr_e));
            }
        }
    }

    while (!igraph_stack_empty(&edge_path)) {
        IGRAPH_CHECK(igraph_vector_push_back(res, igraph_stack_pop(&edge_path)));           
    }

    igraph_stack_destroy(&path);
    igraph_stack_destroy(&tracker);
    igraph_stack_destroy(&edge_path);
    igraph_stack_destroy(&edge_tracker);
    igraph_vector_bool_destroy(&visited_list);
    igraph_inclist_destroy(&il);
    igraph_vector_destroy(&degree);    
    IGRAPH_FINALLY_CLEAN(7);

    return IGRAPH_SUCCESS;
}

/* solution adapted from https://www.geeksforgeeks.org/hierholzers-algorithm-directed-graph/ */
int igraph_i_eulerian_path_directed(const igraph_t *graph, igraph_vector_t *res, igraph_integer_t start_node) {
    igraph_integer_t curr = start_node;
    igraph_integer_t next, curr_e, vector_count, edge_count;
    igraph_inclist_t il;
    igraph_stack_t path, tracker, edge_tracker, edge_path;
    igraph_vector_int_t *incedges;
    igraph_vector_bool_t visited_list;
    igraph_vector_t outgoing_list;
    long nc, edge;
    int j;

    vector_count = igraph_vcount(graph);
    edge_count = igraph_ecount(graph);

    igraph_vector_clear(res);
    IGRAPH_CHECK(igraph_vector_reserve(res, edge_count));

    if (edge_count == 0 || vector_count == 0) {
        return IGRAPH_SUCCESS;
    }

    IGRAPH_CHECK(igraph_stack_init(&path, vector_count));
    IGRAPH_FINALLY(igraph_stack_destroy, &path);

    IGRAPH_CHECK(igraph_stack_init(&tracker, vector_count));
    IGRAPH_FINALLY(igraph_stack_destroy, &tracker);

    IGRAPH_CHECK(igraph_stack_init(&edge_path, vector_count));
    IGRAPH_FINALLY(igraph_stack_destroy, &edge_path);

    IGRAPH_CHECK(igraph_stack_init(&edge_tracker, vector_count));
    IGRAPH_FINALLY(igraph_stack_destroy, &edge_tracker);

    IGRAPH_CHECK(igraph_vector_bool_init(&visited_list, edge_count));
    IGRAPH_FINALLY(igraph_vector_bool_destroy, &visited_list);

    IGRAPH_CHECK(igraph_stack_push(&tracker, start_node));

    IGRAPH_CHECK(igraph_inclist_init(graph, &il, IGRAPH_OUT));
    IGRAPH_FINALLY(igraph_inclist_destroy, &il);

    IGRAPH_CHECK(igraph_vector_init(&outgoing_list, 0));
    IGRAPH_FINALLY(igraph_vector_destroy, &outgoing_list);
    IGRAPH_CHECK(igraph_degree(graph, &outgoing_list, igraph_vss_all(), IGRAPH_OUT, IGRAPH_LOOPS));

    while (!igraph_stack_empty(&tracker)) {
        
        if (VECTOR(outgoing_list)[curr] != 0) {
            IGRAPH_CHECK(igraph_stack_push(&tracker, curr));
            
            incedges = igraph_inclist_get(&il, curr);
            nc = igraph_vector_int_size(incedges);

            for (j = 0; j < nc; j++) {
                edge = (long) VECTOR(*incedges)[j];
                if (!VECTOR(visited_list)[edge]) {
                    break;
                }
            }
            
            next = IGRAPH_TO(graph, edge);

            IGRAPH_CHECK(igraph_stack_push(&edge_tracker, edge));

            /* remove edge here */
            VECTOR(outgoing_list)[curr]--;
            VECTOR(visited_list)[edge] = 1;

            curr = next;
        } else { /* back track to find remaining circuit */
            IGRAPH_CHECK(igraph_stack_push(&path, curr));
            curr = igraph_stack_pop(&tracker);
            if (!igraph_stack_empty(&edge_tracker)) {
                curr_e = igraph_stack_pop(&edge_tracker);
                IGRAPH_CHECK(igraph_stack_push(&edge_path, curr_e));
            }
        }
    }

    while (!igraph_stack_empty(&edge_path)) {
        IGRAPH_CHECK(igraph_vector_push_back(res, igraph_stack_pop(&edge_path)));           
    }

    igraph_stack_destroy(&path);
    igraph_stack_destroy(&tracker);
    igraph_stack_destroy(&edge_path);
    igraph_stack_destroy(&edge_tracker);
    igraph_vector_bool_destroy(&visited_list);
    igraph_inclist_destroy(&il);
    IGRAPH_FINALLY_CLEAN(6);

    return IGRAPH_SUCCESS;
}


/**
 * \ingroup Eulerian
 * \function igraph_eulerian_cycle
 * \brief Finds an Eulerian cycle
 *
 * Finds an Eulerian cycle, if it exists. An Eulerian cycle is a closed path
 * that traverses each edge precisely once.
 *
 * \param graph The graph object.
 * \param res Pointer to an initialised vector. The indices of edges
 *            belonging to the cycle will be stored here.
 * \return Error code:
 *        \clist
 *        \cli IGRAPH_ENOMEM
 *           not enough memory for temporary data.
 *        \cli IGRAPH_EINVVID
 *           graph does not have an Eulerian cycle.
 *        \endclist
 *
 * Time complexity: O(|V||E|), the
 * number of vertices times the number of edges.
 *
 */


int igraph_eulerian_cycle(const igraph_t *graph, igraph_vector_t *res) {
    igraph_bool_t has_cycle;
    igraph_bool_t has_path;
    igraph_integer_t start_of_path = 0;

    if (igraph_is_directed(graph)) {
        IGRAPH_CHECK(igraph_i_is_eulerian_directed(graph, &has_path, &has_cycle, &start_of_path));
        
        if (!has_cycle) {
            IGRAPH_ERROR("The graph does not have an Eulerian cycle.", IGRAPH_EINVAL);
        }

        IGRAPH_CHECK(igraph_i_eulerian_path_directed(graph, res, start_of_path));
    } else {
        IGRAPH_CHECK(igraph_i_is_eulerian_undirected(graph, &has_path, &has_cycle, &start_of_path));

        if (!has_cycle) {
            IGRAPH_ERROR("The graph does not have an Eulerian cycle.", IGRAPH_EINVAL);
        }   

        IGRAPH_CHECK(igraph_i_eulerian_path_undirected(graph, res, start_of_path));
    }

    return IGRAPH_SUCCESS;
}


/**
 * \ingroup Eulerian
 * \function igraph_eulerian_path
 * \brief Finds an Eulerian path
 *
 * Finds an Eulerian path, if it exists. An Eulerian path traverses
 * each edge precisely once.
 *
 * \param graph The graph object.
 * \param res Pointer to an initialised vector. The indices of edges
 *            belonging to the path will be stored here.
 * \return Error code:
 *        \clist
 *        \cli IGRAPH_ENOMEM
 *           not enough memory for temporary data.
 *        \cli IGRAPH_EINVVID
 *           graph does not have an Eulerian path.
 *        \endclist
 *
 * Time complexity: O(|V||E|), the
 * number of vertices times the number of edges.
 *
 */

int igraph_eulerian_path(const igraph_t *graph, igraph_vector_t *res) {
    igraph_bool_t has_cycle;
    igraph_bool_t has_path;
    igraph_integer_t start_of_path = 0;

    if (igraph_is_directed(graph)) {
        IGRAPH_CHECK(igraph_i_is_eulerian_directed(graph, &has_path, &has_cycle, &start_of_path));
        
        if (!has_path) {
            IGRAPH_ERROR("The graph does not have an Eulerian path.", IGRAPH_EINVAL);
        }
        IGRAPH_CHECK(igraph_i_eulerian_path_directed(graph, res, start_of_path));
    } else {
        IGRAPH_CHECK(igraph_i_is_eulerian_undirected(graph, &has_path, &has_cycle, &start_of_path));
        
        if (!has_path) {
            IGRAPH_ERROR("The graph does not have an Eulerian path.", IGRAPH_EINVAL);
        }

        IGRAPH_CHECK(igraph_i_eulerian_path_undirected(graph, res, start_of_path));
    }

    return IGRAPH_SUCCESS;
}

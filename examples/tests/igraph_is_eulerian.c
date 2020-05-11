#include <igraph.h>

int main() {

    igraph_t graph, graph2, graph3, graph4, graph5, graph6, graph7, graph8, graph9, graph10, graph11;
    igraph_bool_t has_path, has_cycle;
    /* undirected cases */

    igraph_small(&graph, 0, IGRAPH_UNDIRECTED, 0,1 , 1,2, -1);
    igraph_is_eulerian(&graph, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph2, 0, IGRAPH_UNDIRECTED, 0,1, 1,2, 2,3, 3,4, 4,5, 5,2,
                2,6, 6,4, 4,8, 2,8, 2,7, 0,7, -1);
    igraph_is_eulerian(&graph2, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph3, 0, IGRAPH_UNDIRECTED, 0,1 , 1,2, 2,3, 2,4 , 3,5 , 4,5,
                4,6, 0,6, 6,7, 1,7, -1);
    igraph_is_eulerian(&graph3, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph4, 0, IGRAPH_UNDIRECTED, 0,1 , 1,2, 2,3, 3,4 , 2,4 , 1,5,
                0,5 , -1);
    igraph_is_eulerian(&graph4, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph5, 0, IGRAPH_UNDIRECTED, 0,1, 1,2, 2,4, 3,4, 1,3, 2,5, 4,5, 2,6, 1,6, 0,4, 6,5, -1);
    igraph_is_eulerian(&graph5, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    /* directed cases*/

    igraph_small(&graph6, 0, IGRAPH_DIRECTED, 0,1 , 1,2, -1);
    igraph_is_eulerian(&graph6, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph7, 0, IGRAPH_DIRECTED, 0,1, 1,2, 2,0, -1);
    igraph_is_eulerian(&graph7, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph8, 0, IGRAPH_DIRECTED, 0,1 , 1,2, 1,3, -1);
    igraph_is_eulerian(&graph8, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph9, 0, IGRAPH_DIRECTED, 0,1 , 1,3, 3,2, 2,0 , 2,1, -1);
    igraph_is_eulerian(&graph9, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph10, 0, IGRAPH_DIRECTED, 0,3, 3,4, 4,0, 0,2, 2,1, 1,0, -1);
    igraph_is_eulerian(&graph10, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_small(&graph11, 0, IGRAPH_DIRECTED, 0,6, 6,4, 4,5, 5,0, 0,1, 1,2,
                2,3, 3,4, 4,2, 2,0, -1);
    igraph_is_eulerian(&graph11, &has_path, &has_cycle);
    printf("%d %d\n", has_path, has_cycle);

    igraph_destroy(&graph);
    igraph_destroy(&graph2);
    igraph_destroy(&graph3);
    igraph_destroy(&graph4);
    igraph_destroy(&graph5);
    igraph_destroy(&graph6);
    igraph_destroy(&graph7);
    igraph_destroy(&graph8);
    igraph_destroy(&graph9);
    igraph_destroy(&graph10);
    igraph_destroy(&graph11);

    return 0;
}
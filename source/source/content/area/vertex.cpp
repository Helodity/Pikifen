/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Edge vertex class and related functions.
 */

#define _USE_MATH_DEFINES

#include "vertex.h"

#include "edge.h"


/**
 * @brief Constructs a new vertex object.
 *
 * @param x X coordinate.
 * @param y Y coordinate.
 */
Vertex::Vertex(float x, float y) :
    x(x),
    y(y) {
    
}


/**
 * @brief Adds an edge to the vertex's list of edges, if it's not there already.
 *
 * @param e_ptr Edge to add.
 * @param e_idx Index number of the edge to add.
 */
void Vertex::addEdge(Edge* e_ptr, size_t e_idx) {
    for(size_t i = 0; i < edges.size(); i++) {
        if(edges[i] == e_ptr) {
            return;
        }
    }
    edges.push_back(e_ptr);
    edgeIdxs.push_back(e_idx);
}


/**
 * @brief Returns the edge that has the specified vertex as a neighbor
 * of this vertex.
 *
 * @param neighbor The neighbor vertex to check.
 * @return The edge, or nullptr if not found.
 */
Edge* Vertex::getEdgeByNeighbor(const Vertex* neighbor) const {
    for(size_t e = 0; e < edges.size(); e++) {
        if(edges[e]->getOtherVertex(this) == neighbor) {
            return edges[e];
        }
    }
    return nullptr;
}


/**
 * @brief Returns whether or not this vertex has the specified edge in its list.
 *
 * @param e_ptr Edge to check.
 * @return Whether it has the edge.
 */
bool Vertex::hasEdge(const Edge* e_ptr) const {
    for(size_t e = 0; e < edges.size(); e++) {
        if(edges[e] == e_ptr) return true;
    }
    return false;
}


/**
 * @brief Returns whether or not this vertex is a second-degree neighbor to the
 * specified vertex. i.e. they have a shared neighbor between them.
 *
 * @param other_v The vertex to compare against.
 * @param first_neighbor Return the common neighbor between them here,
 * if the result is true.
 * @return Whether it is a second-degree neighbor.
 */
bool Vertex::is2ndDegreeNeighbor(
    const Vertex* other_v, Vertex** first_neighbor
) const {
    //Let's crawl forward through all edges and stop at the second level.
    //If other_v is at that distance, then we found it!
    
    for(size_t e1 = 0; e1 < edges.size(); e1++) {
        Vertex* next_v = edges[e1]->getOtherVertex(this);
        
        for(size_t e2 = 0; e2 < next_v->edges.size(); e2++) {
            if(next_v->edges[e2]->getOtherVertex(next_v) == other_v) {
                *first_neighbor = next_v;
                return true;
            }
        }
        
    }
    return false;
}


/**
 * @brief Returns whether or not this vertex is a second-degree neighbor to the
 * specified edge. i.e. one of the vertex's neighbors is used by the edge.
 *
 * @param other_e The edge to compare against.
 * @param first_neighbor Return the common neighbor between them here,
 * if the result is true.
 * @return Whether it is a second-degree neighbor.
 */
bool Vertex::is2ndDegreeNeighbor(
    const Edge* other_e, Vertex** first_neighbor
) const {
    //Let's crawl forward through all edges and stop at the second level.
    //If other_e is at that distance, then we found it!
    
    for(size_t e1 = 0; e1 < edges.size(); e1++) {
        Vertex* next_v = edges[e1]->getOtherVertex(this);
        
        for(size_t e2 = 0; e2 < next_v->edges.size(); e2++) {
            if(next_v->edges[e2] == other_e) {
                *first_neighbor = next_v;
                return true;
            }
        }
        
    }
    return false;
}


/**
 * @brief Returns whether or not this vertex is a neighbor to the
 * specified vertex. i.e. they have a shared edge between them.
 *
 * @param other_v The vertex to compare against.
 * @return Whether it is a neighbor.
 */
bool Vertex::isNeighbor(const Vertex* other_v) const {
    for(size_t e = 0; e < edges.size(); e++) {
        if(edges[e]->getOtherVertex(this) == other_v) {
            return true;
        }
    }
    return false;
}


/**
 * @brief Removes an edge from a vertex's list of edges, if it is there.
 *
 * @param e_ptr Edge to remove.
 */
void Vertex::removeEdge(const Edge* e_ptr) {
    size_t i = 0;
    for(; i < edges.size(); i++) {
        if(edges[i] == e_ptr) {
            edges.erase(edges.begin() + i);
            edgeIdxs.erase(edgeIdxs.begin() + i);
            return;
        }
    }
}

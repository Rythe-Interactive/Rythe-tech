#pragma once

#include <core/core.hpp>
#include <physics/mesh_splitter_utils/mesh_splitter_typedefs.hpp>
#include <physics/mesh_splitter_utils/mesh_half_edge.hpp>
#include <physics/mesh_splitter_utils/splittable_polygon.h>
#include <rendering/components/renderable.hpp>
#include <physics/mesh_splitter_utils/primitive_mesh.h>
#include <rendering/components/renderable.hpp>
#include <physics/mesh_splitter_utils/half_edge_finder.h>
#include <physics/mesh_splitter_utils/mesh_splitter_debug_helpers.h>
#include <physics/mesh_splitter_utils/intersecting_polygon_organizer.hpp>
#include <physics/mesh_splitter_utils/mesh_split_params.hpp>
#include <physics/mesh_splitter_utils/intersection_edge_info.h>

namespace legion::physics
{
    struct MeshSplitter
    {

        ecs::entity_handle owner;
        std::vector<ecs::entity_handle> splitTester;

        rendering::material_handle ownerMaterialH;

        std::vector<SplittablePolygonPtr> meshPolygons;

        MeshSplitterDebugHelper debugHelper;

        //------------------------------------------------------ Function related to Mesh Splitter Initialization ---------------------------------------------//

        /** @brief Creates a Half-Edge Data structure around the mesh and
       * @param entity the entity that this MeshSplitter is attached to
       */
        void InitializePolygons(ecs::entity_handle entity)
        {
            owner = entity;

            auto [meshFilter, meshRenderer] = entity.get_component_handles<rendering::mesh_renderable>();

            ownerMaterialH = meshRenderer.read().material;

            auto [posH, rotH, scaleH] = entity.get_component_handles<transform>();

            if (meshFilter && posH && rotH && scaleH)
            {
                log::debug("Mesh and Transform found");
                std::queue<meshHalfEdgePtr> meshHalfEdges;

                //auto renderable = renderable.read();
                mesh& mesh = meshFilter.read().get().second;

                const math::mat4 transform = math::compose(scaleH.read(), rotH.read(), posH.read());

                HalfEdgeFinder edgeFinder;
                edgeFinder.FindHalfEdge(mesh, transform, meshHalfEdges);

                BFSPolygonize(meshHalfEdges, transform);

                log::debug("Mesh vertices {}, Mesh indices {}", mesh.vertices.size(), mesh.indices.size());

            }
            else
            {
                log::warn("The given entity does not have a meshHandle!");
            }

            for (auto face : meshPolygons)
            {
                for (auto edge : face->GetMeshEdges())
                {
                    assert(edge->owner.lock());
                    assert(edge->pairingEdge->owner.lock());
                }
            }



        }

        /** @brief Given a queue of edges and a transform,
        * populates the std::vector 'meshPolygons' using BFS.
        * @note halfEdgeQueue will be empty after this function
        */
        void BFSPolygonize(std::queue<meshHalfEdgePtr>& halfEdgeQueue, const math::mat4& transform)
        {
            //while edge queue is not empty
            while (!halfEdgeQueue.empty())
            {
                meshHalfEdgePtr startEdge = halfEdgeQueue.front();
                halfEdgeQueue.pop();

                if (!startEdge->isVisited)
                {
                    SplittablePolygonPtr polygon = nullptr;

                    if (BFSIdentifyPolygon(startEdge, polygon, halfEdgeQueue, transform))
                    {
                        meshPolygons.push_back(polygon);
                    }
                }
            }
        }

        /** @brief Given an intial startEdge, do a BFS to identify the polygon
        * that the edge is in. Also populates the halfEdgeQueue with the neigbors of the boundary of the polygon
        */
        bool BFSIdentifyPolygon(meshHalfEdgePtr startEdge
            , std::shared_ptr<SplittablePolygon>& polygon, std::queue<meshHalfEdgePtr>& halfEdgeQueue
            , const math::mat4& transform)
        {
            log::debug("->BFSIdentifyPolygon");
            //polygonEdgeList : edges considered to be in the same polygon
            std::vector<meshHalfEdgePtr> edgesInPolygon;

            meshHalfEdgePtr nextEdge = nullptr;
            meshHalfEdgePtr prevEdge = nullptr;

            //startEdge may not form a triangle,we early out if this happens
            if (!startEdge->attemptGetTrianglesInEdges(nextEdge, prevEdge))
            {
                return false;
            }

            edgesInPolygon.push_back(startEdge);
            edgesInPolygon.push_back(nextEdge);
            edgesInPolygon.push_back(prevEdge);

            //mark all edges visited
            startEdge->markTriangleEdgeVisited();

            //get all neigbors of the startEdge Triangle and put them in unvisitedEdgeQueue
            std::queue<meshHalfEdgePtr> unvisitedEdgeQueue;
            startEdge->populateQueueWithTriangleNeighbor(unvisitedEdgeQueue);

            std::vector<meshHalfEdgePtr> edgesNotInPolygon;

            const math::vec3 comparisonNormal = startEdge->calculateEdgeNormal(transform);

            //BFS search for adjacent triangles with same normal
            while (!unvisitedEdgeQueue.empty())
            {
                //log::debug("Pop Edge");

                auto edgeToCheck = unvisitedEdgeQueue.front();
                unvisitedEdgeQueue.pop();

                if (!edgeToCheck) { continue; }

                if (!edgeToCheck->isVisited && edgeToCheck->isTriangleValid())
                {
                    edgeToCheck->markTriangleEdgeVisited();

                    //if triangle has same normal as original
                    if (edgeToCheck->isNormalCloseEnough(comparisonNormal, transform))
                    {
                        //add all edges in triangle to polygonEdgeList
                        edgeToCheck->populateVectorWithTriangle(edgesInPolygon);
                        //add neigbors to polygonEdgeNonVisitedQueue
                        edgeToCheck->populateQueueWithTriangleNeighbor(unvisitedEdgeQueue);
                    }
                    else
                    {
                        //add edge to edgesNotInPolygon
                        edgeToCheck->populateVectorWithTriangle(edgesNotInPolygon);
                    }
                }
            }

            for (auto edge : edgesNotInPolygon)
            {
                edge->isVisited = false;
                halfEdgeQueue.push(edge);
            }

            math::vec3 localNormal = math::inverse(transform) * math::vec4(comparisonNormal, 0);
            polygon = std::make_shared<SplittablePolygon>(edgesInPolygon, localNormal);

            polygon->AssignEdgeOwnership();
            polygon->IdentifyBoundaries(transform);

            return true;
        }

        //--------------------------------------------------------- Function related to splitting ----------------------------------------------------------------//

        /** @brief Given a list of splitting planes, splits the mesh based on the list of splitting planes
        */
        void MultipleSplitMesh(const std::vector<MeshSplitParams>& splittingPlanes, std::vector<ecs::entity_handle>& entitiesGenerated, bool keepBelow = true);
       

        /** @brief Given a list of polygons to split in 'polygonsToSplit', splits them based on a splitting plane defined by
        * 'planePosition' and 'planeNormal'. The result is then placed in 'resultingIslands.
        */
        void SplitPolygons
        (std::vector<SplittablePolygonPtr>& polygonsToSplit,
            const math::vec3& planeNormal,
            const math::vec3& planePosition,
            const math::mat4& transform,
            std::vector<std::vector<SplittablePolygonPtr>>& resultingIslands,
            bool keepBelow = true);
        

        //--------------------------------------------------------- Function related to polygon copying ----------------------------------------------------------------//

        /** @brief Copies the polygons of 'originalSplitMesh' and places them in 'copySplitMesh'
        */
        void CopyPolygons(std::vector<SplittablePolygonPtr>& originalSplitMesh, std::vector<SplittablePolygonPtr>& copySplitMesh);
        
        void CopyEdgeVector(std::vector<meshHalfEdgePtr>& originalHalfEdgeList, std::vector<meshHalfEdgePtr>& resultCopyList);
        
        //--------------------------------------------------------- MeshSplitting helper functions ----------------------------------------------------------------//

        /** @brief Does a floodfill to find polygons that are either intersecting the splitting plane or at the requested state
        */
        void BFSFindRequestedAndIntersecting(
            SplittablePolygonPtr& intialPolygon,
            std::vector<SplittablePolygonPtr>& originalSplitMesh,
            std::vector<SplittablePolygonPtr>& originalNonSplitMesh, SplitState requestedState)
        {
            std::queue<SplittablePolygonPtr> unvisitedPolygonQueue;
            unvisitedPolygonQueue.push(intialPolygon);

            while (!unvisitedPolygonQueue.empty())
            {
                auto polygonPtr = unvisitedPolygonQueue.front();
                unvisitedPolygonQueue.pop();

                //assert(polygonPtr);
                if (!polygonPtr) { continue; }

                if (!polygonPtr->isVisited)
                {
                    polygonPtr->isVisited = true;

                    auto polygonSplitState = polygonPtr->GetPolygonSplitState();

                    bool polygonAtRequestedState = polygonSplitState == requestedState;
                    bool polygonAtIntersection = polygonSplitState == SplitState::Split;

                    //place polygon in correct list

                    if (polygonAtRequestedState)
                    {
                        originalNonSplitMesh.push_back(polygonPtr);
                        debugHelper.nonIntersectionPolygons.push_back(polygonPtr->localCentroid);
                    }
                    else if (polygonAtIntersection)
                    {
                        originalSplitMesh.push_back(polygonPtr);
                        debugHelper.intersectionsPolygons.push_back(polygonPtr->localCentroid);
                    }

                    //only put it on the unvisited list if polygon is at requested state or is intersecting the splitting plane

                    if (polygonAtIntersection || polygonAtRequestedState)
                    {
                        for (auto edge : polygonPtr->GetMeshEdges())
                        {
                            bool isBoundary = edge->isBoundary;
                            bool hasPairing = (edge->pairingEdge) != nullptr;
                        
                            if (isBoundary && hasPairing)
                            {
                                auto newPolygon = edge->pairingEdge->owner.lock();
                                //assert(newPolygon);
                                unvisitedPolygonQueue.push(newPolygon);
                            }
                        }
                    }
                }
            }
        }

        /** @brief Given a list of polygons that are intersecting the splitting plane,
        * detects if there are sets of polygons that are disconected from each other
        */
        void DetectIntersectionIsland(std::vector<SplittablePolygonPtr>& splitPolygons,
            std::vector<std::vector<SplittablePolygonPtr>>& intersectionIslands)
        {
            for (auto pol : splitPolygons) { pol->isVisited = false; }

            //find first intersection polygon
            SplittablePolygonPtr initialPolygon;
            bool foundUnvisited = FindFirstUnivistedIntersectionPolygon(splitPolygons, initialPolygon);

            //while can find intersection polygon
            while (foundUnvisited)
            {
                std::vector< math::vec3> DEBUG_ONLY_polygonPositions;
                std::vector< SplittablePolygonPtr> intersectionIsland;
                std::queue< SplittablePolygonPtr> unvisitedPolygons;
                unvisitedPolygons.push(initialPolygon);

                while (!unvisitedPolygons.empty())
                {
                    auto polygon = unvisitedPolygons.front();
                    unvisitedPolygons.pop();

                    if (!polygon->isVisited && polygon->GetPolygonSplitState() == SplitState::Split)
                    {
                        polygon->isVisited = true;
                        intersectionIsland.push_back(polygon);
                        DEBUG_ONLY_polygonPositions.push_back(polygon->localCentroid);

                        for (auto edge : polygon->GetMeshEdges())
                        {
                            auto pairingEdge = edge->pairingEdge;

                            if (edge->isBoundary && pairingEdge)
                            {
                                if (auto pairingPolygon = pairingEdge->owner.lock())
                                {
                                    unvisitedPolygons.push(pairingPolygon);
                                }

                            }
                        }
                    }
                }

                intersectionIslands.push_back(intersectionIsland);
                debugHelper.intersectionIslands.push_back(DEBUG_ONLY_polygonPositions);

                foundUnvisited = FindFirstUnivistedIntersectionPolygon(splitPolygons, initialPolygon);

            }
        }

        /** @brief Creates an IntersectingPolygonOrganizer thats splits a 
        * given a SplittablePolygon along a plane located at 'planePosition' with a normal equal to 'planeNormal'.
        */
        void SplitPolygon(SplittablePolygonPtr splitPolygon
            , const math::mat4& transform, const math::vec3 cutPosition
            , const math::vec3 cutNormal, SplitState requestedState, std::vector<IntersectionEdgeInfo>& generatedIntersectionEdges)
        {
            IntersectingPolygonOrganizer polygonOrganizer(&debugHelper);
            polygonOrganizer.SplitPolygon(splitPolygon, transform, cutPosition, cutNormal, requestedState,generatedIntersectionEdges);
        }

        /** @brief Given a list of SplittablePolygons finds the first unvisited SplittablePolygon that has a split state of 'intersecting'
        * or a split state of 'requestedState'
        */
        bool FindFirstIntersectingOrRequestedState(SplittablePolygonPtr& outfirstFound, SplitState requestedState
            , std::vector<SplittablePolygonPtr>& polygonList)
        {
            for (auto polygon : polygonList)
            {
                if (!polygon->isVisited)
                {
                    auto polygonSplitState = polygon->GetPolygonSplitState();

                    if (polygonSplitState == SplitState::Split
                        || polygonSplitState == requestedState)
                    {
                        outfirstFound = polygon;
                        return true;
                    }
                }
            }

            return false;
        }

        bool FindFirstUnivistedIntersectionPolygon(std::vector< SplittablePolygonPtr>& splitPolygon, SplittablePolygonPtr& foundPolygon)
        {
            for (auto polygonPtr : splitPolygon)
            {
                if (!polygonPtr->isVisited)
                {
                    foundPolygon = polygonPtr;
                    return true;
                }
            }

            return false;
        }

        SplittablePolygonPtr CreateIntersectionPolygon(
            std::vector<IntersectionEdgeInfo>& generatedIntersectionEdges,
            const math::vec3& localSplitNormal)
        {
            //log::debug("CreateIntersectionPolygon");

            std::vector<meshHalfEdgePtr> edgesCreated;

            math::vec3 localCentroid{};

            //---------------------------------- Instantiate Edges and connect them into a triangle -------------------------------------//
            for (IntersectionEdgeInfo& info : generatedIntersectionEdges)
            {
                //instantiate edge and set its pairing
                meshHalfEdgePtr firstEdge = std::make_shared<MeshHalfEdge>(info.first);
                meshHalfEdgePtr secondEdge = std::make_shared<MeshHalfEdge>(info.second);
                //temporarily second edge to info.second
                meshHalfEdgePtr thirdEdge = std::make_shared<MeshHalfEdge>(info.second);

                info.centroidEdge = thirdEdge;
                info.instantiatedEdge  = firstEdge;

                info.pairingToConnectTo->setPairing(info.instantiatedEdge);
                
                info.instantiatedEdge->isBoundary = true;

                MeshHalfEdge::connectIntoTriangle(firstEdge, secondEdge, thirdEdge);
            
                info.instantiatedEdge->populateVectorWithTriangle(edgesCreated);

                localCentroid += info.first;
            }

            localCentroid /= (float)generatedIntersectionEdges.size();

            //---------------------------------- Set the centroid edge to the local centroid of the polygon  -------------------------------------//

            for (IntersectionEdgeInfo& info : generatedIntersectionEdges)
            {
                info.centroidEdge->position = localCentroid;
                assert(info.instantiatedEdge->nextEdge->nextEdge);
            }

            //---------------------------------- Set pairing information of all edges ---------------------------------------------//

            for (IntersectionEdgeInfo& info : generatedIntersectionEdges)
            {
                float currentClosestDistance = std::numeric_limits<float>::max();
                meshHalfEdgePtr closestEdge = nullptr;
                //get closest unvisisted IntersectionEdgeInfo

                const math::vec3& pointToCompare = info.second;
                //find edge closest to pointToCompare
                for (IntersectionEdgeInfo& otherInfo : generatedIntersectionEdges)
                {
            
                    assert(otherInfo.instantiatedEdge);
                    if (info.instantiatedEdge == otherInfo.instantiatedEdge) { continue; }

                    float distanceFound = math::distance2(pointToCompare, otherInfo.first);
                    //log::debug("distanceFound {}" , distanceFound);
                    if (distanceFound < currentClosestDistance)
                    {
                        currentClosestDistance = distanceFound;
                        closestEdge = otherInfo.centroidEdge;
                    }
                }

                assert(closestEdge);
                auto infoEdge = info.instantiatedEdge->nextEdge;
                auto infoEdgePairing = closestEdge;

                infoEdge->setPairing(infoEdgePairing);

            }

            auto polygon = std::make_shared<SplittablePolygon>(edgesCreated, localSplitNormal);
            polygon->AssignEdgeOwnership();

            return  polygon;
        };


        #pragma endregion

        //--------------------------------------------------------- Functions related to Debugging ---------------------------------------------------//

        void DestroyTestSplitter(ecs::EcsRegistry* m_ecs)
        {
            for (auto splitObject : splitTester)
            {
                m_ecs->destroyEntity(splitObject);
            }

            splitTester.clear();
        }

        void TestSplit()
        {

            if (!splitTester.empty())
            {
                std::vector<MeshSplitParams> splittingPlanes;

                for (auto splitObject : splitTester)
                {
                    auto [posH, rotH, scaleH] = splitObject.get_component_handles<transform>();
                    const math::mat4 transform = math::compose(scaleH.read(), rotH.read(), posH.read());
                    const math::vec3 worldUp = transform * math::vec4(0, 1, 0, 0);

                    splittingPlanes.push_back(MeshSplitParams(posH.read(), math::normalize(worldUp)));

                }
                std::vector<ecs::entity_handle> entities;
                MultipleSplitMesh(splittingPlanes, entities);

            }
            else
            {
                log::error("Split tester not set");
            }
        }
    };
}


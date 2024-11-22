#pragma once

#include <lightwave/core.hpp>
#include <lightwave/math.hpp>
#include <lightwave/shape.hpp>

#include <numeric>

namespace lightwave {

/**
 * @brief Parent class for shapes that combine many individual shapes (e.g.,
 * triangle meshes), and hence benefit from building an acceleration structure
 * over their children.
 *
 * To use this class, you will need to implement the following methods:
 * - numberOfPrimitives()           -- report the number of individual children
 * that the shape has
 * - intersect(primitiveIndex, ...) -- intersect a single child (identified by
 * the given index) for the given ray
 * - getBoundingBox(primitiveIndex) -- return the bounding box of a single child
 * (used for building the BVH)
 * - getCentroid(primitiveIndex)    -- return the centroid of a single child
 * (used for building the BVH)
 *
 * @example For a simple example of how to use this class, look at @ref
 * shapes/group.cpp
 * @see Group
 * @see TriangleMesh
 */
class AccelerationStructure : public Shape {
    /// @brief The datatype used to index BVH nodes and the primitive index
    /// remapping.
    typedef int32_t NodeIndex;

    /// @brief A node in our binary BVH tree.
    struct Node {
        /// @brief The axis aligned bounding box of this node.
        Bounds aabb;
        /**
         * @brief Either the index of the left child node in m_nodes (for
         * internal nodes), or the first primitive in m_primitiveIndices (for
         * leaf nodes).
         * @note For efficiency, we store the BVH nodes so that the right child
         * always directly follows the left child, i.e., the index of the right
         * child is always @code leftFirst + 1 @endcode .
         * @note For efficiency, we store primitives so that children of a leaf
         * node are always contigous in m_primitiveIndices.
         */
        NodeIndex leftFirst;
        /// @brief The number of primitives in a leaf node, or 0 to indicate
        /// that this node is not a leaf node.
        NodeIndex primitiveCount;

        /// @brief Whether this BVH node is a leaf node.
        bool isLeaf() const { return primitiveCount != 0; }

        /// @brief For internal nodes: The index of the left child node in
        /// m_nodes.
        NodeIndex leftChildIndex() const { return leftFirst; }
        /// @brief For internal nodes: The index of the right child node in
        /// m_nodes.
        NodeIndex rightChildIndex() const { return leftFirst + 1; }

        /// @brief For leaf nodes: The first index in m_primitiveIndices.
        NodeIndex firstPrimitiveIndex() const { return leftFirst; }
        /// @brief For leaf nodes: The last index in m_primitiveIndices (still
        /// included).
        NodeIndex lastPrimitiveIndex() const {
            return leftFirst + primitiveCount - 1;
        }
    };

    /// @brief A list of all BVH nodes.
    std::vector<Node> m_nodes;
    /**
     * @brief Mapping from internal @c NodeIndex to @c primitiveIndex as used by
     * all interface methods. For efficient storage, we assume that children of
     * BVH leaf nodes have contiguous indices, which would require re-ordering
     * the primitives. For simplicity, we instead perform this re-ordering on a
     * list of indices (which starts of as @code 0, 1, 2, ..., primitiveCount -
     * 1 @endcode ), which allows us to translate from re-ordered (contiguous)
     * indices to the indices the user of this class expects.
     */
    std::vector<int> m_primitiveIndices;

    /// @brief Returns the root BVH node.
    const Node &rootNode() const {
        // by convention, this is always the first element of m_nodes
        return m_nodes.front();
    }

    /**
     * @brief Intersects a BVH node, recursing into children (for internal
     * nodes), or intersecting all primitives (for leaf nodes).
     */
    bool intersectNode(const Node &node, const Ray &ray, Intersection &its,
                       Sampler &rng) const {
        // update the statistic tracking how many BVH nodes have been tested for
        // intersection
        its.stats.bvhCounter++;

        bool wasIntersected = false;
        if (node.isLeaf()) {
            for (NodeIndex i = 0; i < node.primitiveCount; i++) {
                // update the statistic tracking how many children have been
                // tested for intersection
                its.stats.primCounter++;
                // test the child for intersection
                wasIntersected |= intersect(
                    m_primitiveIndices[node.leftFirst + i], ray, its, rng);
            }
        } else { // internal node
            // test which bounding box is intersected first by the ray.
            // this allows us to traverse the children in the order they are
            // intersected in, which can help prune a lot of unnecessary
            // intersection tests.
            const auto leftT =
                intersectAABB(m_nodes[node.leftChildIndex()].aabb, ray);
            const auto rightT =
                intersectAABB(m_nodes[node.rightChildIndex()].aabb, ray);
            if (leftT < rightT) { // left child is hit first; test left child
                                  // first, then right child
                if (leftT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.leftChildIndex()], ray, its, rng);
                if (rightT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.rightChildIndex()], ray, its, rng);
            } else { // right child is hit first; test right child first, then
                     // left child
                if (rightT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.rightChildIndex()], ray, its, rng);
                if (leftT < its.t)
                    wasIntersected |= intersectNode(
                        m_nodes[node.leftChildIndex()], ray, its, rng);
            }
        }
        return wasIntersected;
    }

    /// @brief Performs a slab test to intersect a bounding box with a ray,
    /// returning Infinity in case the ray misses.
    float intersectAABB(const Bounds &bounds, const Ray &ray) const {
        const auto t1 = (bounds.min() - ray.origin) / ray.direction;
        // intersect all axes at once with the maximum slabs of the bounding box
        const auto t2 = (bounds.max() - ray.origin) / ray.direction;

        // the elementwiseMin picks the near slab for each axis, of which we
        // then take the maximum
        const auto tNear = elementwiseMin(t1, t2).maxComponent();
        // the elementwiseMax picks the far slab for each axis, of which we then
        // take the minimum
        const auto tFar = elementwiseMax(t1, t2).minComponent();

        if (tFar < tNear)
            return Infinity; // the ray does not intersect the bounding box
        if (tFar < Epsilon)
            return Infinity; // the bounding box lies behind the ray origin

        return tNear; // return the first intersection with the bounding box
                      // (may also be negative!)
    }

    /// @brief Computes the axis aligned bounding box for a leaf BVH node
    void computeAABB(Node &node) {
        node.aabb = Bounds::empty();
        for (NodeIndex i = 0; i < node.primitiveCount; i++) {
            const Bounds childAABB =
                getBoundingBox(m_primitiveIndices[node.leftFirst + i]);
            node.aabb.extend(childAABB);
        }
    }

    /// @brief Computes the surface area of a bounding box.
    float surfaceArea(const Bounds &bounds) const {
        const auto size = bounds.diagonal();
        return 2 * (size.x() * size.y() + size.x() * size.z() +
                    size.y() * size.z());
    }

    /**
     * For a given node, computes split axis and split position that minimize
     * the surface area heuristic.
     * @param node The BVH node to compute the split for.
     * @param out bestSplitAxis The optimal split axis, or -1 if no useful split
     * exists
     * @param out bestSplitPosition The optimal split position, undefined if no
     * useful split exists
     */
    struct Bin {
        Bounds aabb;
        int primCount = 0;
    };

    void binning(const Node &node, int &bestSplitAxis,
                 float &bestSplitPosition) {
        int num_bins   = 16;
        float bestCost = Infinity;
        std::cout << "Starting binning process...\n";

        for (int a = 0; a < 3; a++) {
            Bounds bbox(TPoint<float, 3>(0.0f, 0.0f, 0.0f),
                        TPoint<float, 3>(0.0f, 0.0f, 0.0f));
            std::cout << "Processing axis " << a << "...\n";

            int counter = 0;
            // ERROR: has to be here inside this loop, cause bbox.max and
            // bbox.min are +inf and -inf
            for (int i = 0; i < node.primitiveCount; i++) {
                counter++;
                Node prim = m_nodes[m_primitiveIndices[node.leftFirst + i]];
                // These are too big, maybe its a problem of get bounding box of
                // some shape or getCentroid?
                if (prim.aabb.min()[a] > -1000000 &
                    prim.aabb.max()[a] < 1000000) {
                    bbox.extend(prim.aabb);
                    // std::cout << "Here min is " << prim.aabb.min() << " max
                    // is "
                    //           << prim.aabb.max() << "...\n";
                    // std::cout << "Here min is " << bbox.min() << " max is "
                    //           << bbox.max() << "...\n";
                }
            }
            // bbox.max and bbox.min are incorrect some times.
            std::cout << "Here" << counter << "...\n";

            std::cout << "bbox.min()[" << a << "] = " << bbox.min()[a]
                      << ", bbox.max()[" << a << "] = " << bbox.max()[a]
                      << "\n";

            if (abs(bbox.min()[a] - bbox.max()[a]) > 100000000) {
                std::cout << "Skipping axis " << a << " as bbox min == max.\n";
                break;
            }

            Bin bin[num_bins];
            float scale = float(num_bins) / (bbox.max()[a] - bbox.min()[a]);
            std::cout << "scale = " << scale << "\n";

            for (int i = 0; i < node.primitiveCount; i++) {
                Node prim = m_nodes[m_primitiveIndices[node.leftFirst + i]];
                int binIdx =
                    min(num_bins - 1,
                        (int) ((getCentroid(
                                    m_primitiveIndices[node.leftFirst + i])[a] -
                                bbox.min()[a]) *
                               scale));
                bin[binIdx].primCount++;
                bin[binIdx].aabb.extend(
                    getBoundingBox(m_primitiveIndices[node.leftFirst + i]));
            }

            float leftArea[num_bins - 1], rightArea[num_bins - 1];
            int leftCount[num_bins - 1], rightCount[num_bins - 1];
            Bounds leftBox, rightBox;
            int leftSum = 0, rightSum = 0;

            std::cout << "Calculating left and right areas...\n";
            for (int i = 0; i < num_bins - 1; i++) {
                leftSum += bin[i].primCount;
                leftCount[i] = leftSum;
                leftBox.extend(bin[i].aabb);
                float leftHeight = leftBox.max()[0] - leftBox.min()[0];
                float leftWidth  = leftBox.max()[1] - leftBox.min()[1];
                float leftDepth  = leftBox.max()[2] - leftBox.min()[2];
                leftArea[i] = leftWidth * leftHeight + leftHeight * leftDepth +
                              leftWidth * leftDepth;

                rightSum += bin[num_bins - 1 - i].primCount;
                rightCount[num_bins - 2 - i] = rightSum;
                rightBox.extend(bin[num_bins - 1 - i].aabb);
                float rightHeight = rightBox.max()[0] - rightBox.min()[0];
                float rightWidth  = rightBox.max()[1] - rightBox.min()[1];
                float rightDepth  = rightBox.max()[2] - rightBox.min()[2];
                rightArea[num_bins - 2 - i] = rightWidth * rightHeight +
                                              rightHeight * rightDepth +
                                              rightWidth * rightDepth;
            }

            scale = (bbox.max()[a] - bbox.min()[a]) / num_bins;
            std::cout << "New scale after area calculation: " << scale << "\n";

            for (int i = 0; i < num_bins - 1; i++) {
                float planeCost =
                    leftCount[i] * leftArea[i] + rightCount[i] * rightArea[i];
                std::cout << "planeCost for bin " << i << " = " << planeCost
                          << "\n";

                if (planeCost < bestCost) {
                    bestSplitAxis     = a;
                    bestSplitPosition = bbox.min()[a] + scale * (i + 1);
                    bestCost          = planeCost;
                    std::cout << "New best cost found: " << bestCost
                              << " at axis " << bestSplitAxis
                              << " and position " << bestSplitPosition << "\n";
                }
            }
        }

        std::cout << "Binning process complete. Best split: axis "
                  << bestSplitAxis << " at position " << bestSplitPosition
                  << " with cost " << bestCost << "\n";
    }

    /// @brief Attempts to subdivide a given BVH node.
    void subdivide(Node &parent) {
        // only subdivide if enough children are available.
        if (parent.primitiveCount <= 2) {
            return;
        }

        // set to true when implementing binning
        static constexpr bool UseSAH = true;

        int splitAxis = -1;
        float splitPosition;
        if (UseSAH) {
            // pick split axis and position using binned SAH
            binning(parent, splitAxis, splitPosition);
        } else {
            // split in the middle of the longest axis
            splitAxis     = parent.aabb.diagonal().maxComponentIndex();
            splitPosition = parent.aabb.center()[splitAxis];
        }

        if (splitAxis == -1) {
            // a split axis of -1 indicates that no useful split exists
            return;
        }

        // the point at which to split (note that primitives must be
        // re-ordered so that all children of the left node will have a
        // smaller index than firstRightIndex, and nodes on the right will
        // have an index larger or equal to firstRightIndex)
        NodeIndex firstRightIndex = parent.firstPrimitiveIndex();
        NodeIndex lastLeftIndex   = parent.lastPrimitiveIndex();

        // partition algorithm (you might remember this from quicksort)
        while (firstRightIndex <= lastLeftIndex) {
            if (getCentroid(m_primitiveIndices[firstRightIndex])[splitAxis] <
                splitPosition) {
                firstRightIndex++;
            } else {
                std::swap(m_primitiveIndices[firstRightIndex],
                          m_primitiveIndices[lastLeftIndex--]);
            }
        }

        const NodeIndex firstLeftIndex = parent.firstPrimitiveIndex();
        const NodeIndex leftCount      = firstRightIndex - firstLeftIndex;
        const NodeIndex rightCount     = parent.primitiveCount - leftCount;

        if (leftCount == 0 || rightCount == 0) {
            // if either child gets no primitives, we abort subdividing
            return;
        }

        // the two children will always be contiguous in our m_nodes list
        const NodeIndex leftChildIndex  = (NodeIndex) (m_nodes.size() + 0);
        const NodeIndex rightChildIndex = (NodeIndex) (m_nodes.size() + 1);
        parent.primitiveCount = 0; // mark the parent node as internal node
        parent.leftFirst      = leftChildIndex;

        m_nodes.emplace_back();
        m_nodes[leftChildIndex].leftFirst      = firstLeftIndex;
        m_nodes[leftChildIndex].primitiveCount = leftCount;

        m_nodes.emplace_back();
        m_nodes[rightChildIndex].leftFirst      = firstRightIndex;
        m_nodes[rightChildIndex].primitiveCount = rightCount;

        // first, process the left child node (and all of its children)
        computeAABB(m_nodes[leftChildIndex]);
        subdivide(m_nodes[leftChildIndex]);
        // then, process the right child node (and all of its children)
        computeAABB(m_nodes[rightChildIndex]);
        subdivide(m_nodes[rightChildIndex]);
    }

protected:
    /// @brief Returns the number of children (individual shapes) that are
    /// part of this acceleration structure.
    virtual int numberOfPrimitives() const = 0;
    /// @brief Intersect a single child (identified by the index) with the
    /// given ray.
    virtual bool intersect(int primitiveIndex, const Ray &ray,
                           Intersection &its, Sampler &rng) const = 0;
    /// @brief Returns the axis aligned bounding box of the given child.
    virtual Bounds getBoundingBox(int primitiveIndex) const = 0;
    /// @brief Returns the centroid of the given child.
    virtual Point getCentroid(int primitiveIndex) const = 0;

    /// @brief Builds the acceleration structure.
    void buildAccelerationStructure() {
        Timer buildTimer;

        // fill primitive indices with 0 to primitiveCount - 1
        m_primitiveIndices.resize(numberOfPrimitives());
        std::iota(m_primitiveIndices.begin(), m_primitiveIndices.end(), 0);

        // create root node
        auto &root          = m_nodes.emplace_back();
        root.leftFirst      = 0;
        root.primitiveCount = numberOfPrimitives();
        computeAABB(root);
        subdivide(root);

        logger(EInfo,
               "built BVH with %ld nodes for %ld primitives in %.1f ms",
               m_nodes.size(),
               numberOfPrimitives(),
               buildTimer.getElapsedTime() * 1000);
    }

public:
    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        if (m_primitiveIndices.empty())
            return false; // exit early if no children exist
        if (intersectAABB(rootNode().aabb, ray) < its.t) // test root
                                                         // bounding box for
                                                         // potential hit
            return intersectNode(rootNode(), ray, its, rng);
        return false;
    }

    Bounds getBoundingBox() const override { return rootNode().aabb; }

    Point getCentroid() const override { return rootNode().aabb.center(); }
};

} // namespace lightwave

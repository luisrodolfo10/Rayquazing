#include <lightwave.hpp>

#include "../core/plyparser.hpp"
#include "accel.hpp"

namespace lightwave {

/**
 * @brief A shape consisting of many (potentially millions) of points.
 */
class PointCloud : public AccelerationStructure {
    /**
     * @brief
     */
    int n;   // PointCloud
    int dim; // Number of points in each dimension, "resolution"
    std::vector<Vector> point_cloud;

    inline void populate(SurfaceEvent &surf, const Point &position,
                         Vector normal, Vector shadingNormal,
                         Vector tangent) const {
        surf.position       = position;
        surf.geometryNormal = normal;
        surf.shadingNormal  = shadingNormal;
        surf.tangent        = tangent;
    }

protected:
    int numberOfPrimitives() const override { return int(point_cloud.size()); }

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        float maxDistance = 10.0f;   // Maximum ray marching distance
        float epsilon     = 0.05f;   // Threshold for intersection
        int maxSteps      = 5;       // Maximum steps for ray marching
        float t           = 0.0001f; // Accumulated distance along the ray

        for (int step = 0; step < maxSteps; ++step) {
            Point currentPoint = ray(t); // Current position along the ray

            // Get the closest distance to any point in the point cloud
            float minDistance = getClosestDistance(currentPoint);

            // If we're within epsilon, consider this an intersection
            if (minDistance < epsilon) {
                // Populate intersection information
                its.t              = t;
                its.position       = currentPoint;
                its.geometryNormal = Vector(currentPoint);
                its.shadingNormal  = Vector(currentPoint);
                return true;
            }

            // Advance by the minimum distance
            t += minDistance;
            if (t > maxDistance) {
                break;
            }
        }
        return false;
    }

    float getClosestDistance(const Point &point) const {
        float minDistance = std::numeric_limits<float>::infinity();
        for (const auto &p : point_cloud) {
            float distance = (p - point).length();
            minDistance    = std::min(minDistance, distance);
        }
        return minDistance;
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        // TODO
    }

    Point getCentroid(int primitiveIndex) const override {
        // TODO
    }

public:
    PointCloud(const Properties &properties) {
        n           = properties.get<float>("n", 8);
        dim         = properties.get<float>("dim", 8);
        float scale = 0.5f / (dim - 1);
        for (int i = 0; i < dim; i++) {
            for (int j = 0; j < dim; j++) {
                for (int k = 0; k < dim; k++) {
                    point_cloud.push_back(Vector(i, j, k));
                }
            }
        }
        buildAccelerationStructure();
    }

    AreaSample sampleArea(Sampler &rng) const override{ NOT_IMPLEMENTED }

    std::string toString() const override {
        return tfm::format(
            "PointCloud[\n"
            "  number of points = %d,\n"
            "  n = %d,\n"
            "  dim = %d\n"
            "]",
            point_cloud.size(),
            n,
            dim);
    }
};

} // namespace lightwave

REGISTER_SHAPE(PointCloud, "PointCloud")

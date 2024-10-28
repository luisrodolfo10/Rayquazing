#include <lightwave.hpp>

#include "../core/plyparser.hpp"
#include "accel.hpp"

namespace lightwave {

/**
 * @brief A shape consisting of many (potentially millions) of triangles, which
 * share an index and vertex buffer. Since individual triangles are rarely
 * needed (and would pose an excessive amount of overhead), collections of
 * triangles are combined in a single shape.
 */
class TriangleMesh : public AccelerationStructure {
    /**
     * @brief The index buffer of the triangles.
     * The n-th element corresponds to the n-th triangle, and each component of
     * the element corresponds to one vertex index (into @c m_vertices ) of the
     * triangle. This list will always contain as many elements as there are
     * triangles.
     */
    std::vector<Vector3i> m_triangles;
    /**
     * @brief The vertex buffer of the triangles, indexed by m_triangles.
     * Note that multiple triangles can share vertices, hence there can also be
     * fewer than @code 3 * numTriangles @endcode vertices.
     */
    std::vector<Vertex> m_vertices;
    /// @brief The file this mesh was loaded from, for logging and debugging
    /// purposes.
    std::filesystem::path m_originalPath;
    /// @brief Whether to interpolate the normals from m_vertices, or report the
    /// geometric normal instead.
    bool m_smoothNormals;

    // inline void populate(SurfaceEvent &surf, const Point &position) const {
    //     surf.position = position;

    //     surf.geometryNormal = Vector(position); // vector from sphere
    //     surf.shadingNormal  = surf.geometryNormal;
    // }

protected:
    int numberOfPrimitives() const override { return int(m_triangles.size()); }

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its,
                   Sampler &rng) const override {

        // hints:
        // * use m_triangles[primitiveIndex] to get the vertex indices of the
        // triangle that should be intersected
        Vector3i triangleIndices = m_triangles[primitiveIndex];
        Vertex v1                = m_vertices[triangleIndices[0]];
        Vertex v2                = m_vertices[triangleIndices[1]];
        Vertex v3                = m_vertices[triangleIndices[2]];

        Vector e1           = v2.position - v1.position;
        Vector e2           = v3.position - v1.position;
        Vector ray_cross_e2 = ray.direction.cross(e2);
        float det           = e1.dot(ray_cross_e2);

        if (det > -Epsilon && det < Epsilon)
            return false;

        float inv_det = 1.0 / det;
        Vector s      = ray.origin - v1.position;
        float u       = inv_det * s.dot(ray_cross_e2);

        if (u < 0 || u > 1)
            return false;

        Vector s_cross_e1 = s.cross(e1);
        float v           = inv_det * ray.direction.dot(s_cross_e1);

        if (v < 0 || u + v > 1)
            return false;

        float t = inv_det * e2.dot(s_cross_e1);

        if (t > Epsilon) {
            its.t = t;

            if (m_smoothNormals) {
                Vector2 bary(u, v);
                Vector interpolatedNormal =
                    interpolateBarycentric(
                        bary, v1.normal, v2.normal, v3.normal)
                        .normalized();
                its.geometryNormal = interpolatedNormal;
                its.shadingNormal  = its.geometryNormal;
            } else {
                its.geometryNormal = e1.cross(e2).normalized();
                its.shadingNormal  = its.geometryNormal;
            }

            Point intersectionPosition = ray.origin + t * ray.direction;
            return true;
        } else {
            return false;
        }
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        Vector3i indices = m_triangles[primitiveIndex];
        Vertex v1        = m_vertices[indices[0]];
        Vertex v2        = m_vertices[indices[1]];
        Vertex v3        = m_vertices[indices[2]];

        Bounds box;
        box.extend(v1.position);
        box.extend(v2.position);
        box.extend(v3.position);
        return box;
    }

    Point getCentroid(int primitiveIndex) const override {
        Vector3i indices = m_triangles[primitiveIndex];
        Point v1         = m_vertices[indices[0]].position;
        Point v2         = m_vertices[indices[1]].position;
        Point v3         = m_vertices[indices[2]].position;

        float x = (v1[0] + v2[0] + v3[0]) / 3.0f;
        float y = (v1[1] + v2[1] + v3[1]) / 3.0f;
        float z = (v1[2] + v2[2] + v3[2]) / 3.0f;

        return Point(x, y, z);
    }

public:
    TriangleMesh(const Properties &properties) {
        m_originalPath  = properties.get<std::filesystem::path>("filename");
        m_smoothNormals = properties.get<bool>("smooth", true);
        readPLY(m_originalPath, m_triangles, m_vertices);
        logger(EInfo,
               "loaded ply with %d triangles, %d vertices",
               m_triangles.size(),
               m_vertices.size());
        buildAccelerationStructure();
    }

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        PROFILE("Triangle mesh")
        return AccelerationStructure::intersect(ray, its, rng);
    }

    AreaSample sampleArea(Sampler &rng) const override{
        // only implement this if you need triangle mesh
        // area light sampling for
        // your rendering competition
        NOT_IMPLEMENTED
    }

    std::string toString() const override {
        return tfm::format(
            "Mesh[\n"
            "  vertices = %d,\n"
            "  triangles = %d,\n"
            "  filename = \"%s\"\n"
            "]",
            m_vertices.size(),
            m_triangles.size(),
            m_originalPath.generic_string());
    }
};

} // namespace lightwave

REGISTER_SHAPE(TriangleMesh, "mesh")

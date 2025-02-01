#include <lightwave/core.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

void Instance::transformFrame(SurfaceEvent &surf, const Vector &wo) const {
    Frame shadingFrame = surf.shadingFrame();

    if (m_normal) {
        // Reference:
        // https://learnopengl.com/Advanced-Lighting/Normal-Mapping
        Color normal_col = m_normal->evaluate(surf.uv);

        // Normal values are in [0, 1] and should be remaped to [-1, 1]
        Vector normal_vec =
            (2 * Vector(normal_col.data()) - Vector(1)).normalized();

        normal_vec = shadingFrame.toWorld(normal_vec.normalized());

        shadingFrame.normal = normal_vec;
    }
    shadingFrame.normal =
        m_transform->applyNormal(shadingFrame.normal).normalized();
    shadingFrame.tangent =
        m_transform->apply(shadingFrame.tangent).normalized();
    surf.tangent        = shadingFrame.tangent;
    surf.geometryNormal = shadingFrame.normal;
    surf.shadingNormal  = surf.geometryNormal;
}

inline void validateIntersection(const Intersection &its) {
    // use the following macros to make debugginer easier:
    // * assert_condition(condition, { ... });
    // * assert_normalized(vector, { ... });
    // * assert_ortoghonal(vec1, vec2, { ... });
    // * assert_finite(value or vector or color, { ... });

    // each assert statement takes a block of code to execute when it fails
    // (useful for printing out variables to narrow done what failed)

    assert_finite(its.t, {
        logger(
            EError,
            "  your intersection produced a non-finite intersection distance");
        logger(EError, "  offending shape: %s", its.instance->shape());
    });
    assert_condition(its.t >= Epsilon, {
        logger(EError,
               "  your intersection is susceptible to self-intersections");
        logger(EError, "  offending shape: %s", its.instance->shape());
        logger(EError,
               "  returned t: %.3g (smaller than Epsilon = %.3g)",
               its.t,
               Epsilon);
    });
}

bool Instance::intersect(const Ray &worldRay, Intersection &its,
                         Sampler &rng) const {
    if (!m_transform && !m_normal) {
        // fast path, if no transform is needed
        const Ray localRay        = worldRay;
        const bool wasIntersected = m_shape->intersect(localRay, its, rng);
        if (wasIntersected) {
            its.instance = this;
            validateIntersection(its);
        }
        return wasIntersected;
    }

    const float previousT = its.t;
    Ray localRay;

    // Transform the ray
    localRay = m_transform->inverse(worldRay);
    const float scale_t =
        (localRay.direction.length() / worldRay.direction.length());
    its.t              = its.t * scale_t;
    localRay.direction = localRay.direction.normalized();

    const bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (wasIntersected) {
        if (m_alpha) {
            // Evaluate the alpha channel at the intersection
            float alpha = m_alpha->evaluate(its.uv).r();
            std::cout << "Alpha value is " << alpha << '\n';
            if (alpha < rng.next()) {
                // Discard intersection
                its.t = previousT;
                return false;
            }
        }

        its.instance = this;
        validateIntersection(its);
        its.t = its.t / scale_t;

        its.position = m_transform->apply(its.position);
        transformFrame(its, -localRay.direction);
    } else {
        its.t = previousT;
    }

    return wasIntersected;
}

Bounds Instance::getBoundingBox() const {
    if (!m_transform) {
        // fast path
        return m_shape->getBoundingBox();
    }

    const Bounds untransformedAABB = m_shape->getBoundingBox();
    if (untransformedAABB.isUnbounded()) {
        return Bounds::full();
    }

    Bounds result;
    for (int point = 0; point < 8; point++) {
        Point p = untransformedAABB.min();
        for (int dim = 0; dim < p.Dimension; dim++) {
            if ((point >> dim) & 1) {
                p[dim] = untransformedAABB.max()[dim];
            }
        }
        p = m_transform->apply(p);
        result.extend(p);
    }
    return result;
}

Point Instance::getCentroid() const {
    if (!m_transform) {
        // fast path
        return m_shape->getCentroid();
    }

    return m_transform->apply(m_shape->getCentroid());
}

AreaSample Instance::sampleArea(Sampler &rng) const {
    AreaSample sample = m_shape->sampleArea(rng);
    transformFrame(sample, Vector());
    return sample;
}

} // namespace lightwave

REGISTER_CLASS(Instance, "instance", "default")

#include <lightwave.hpp>

namespace lightwave {

class PathTracerIntegrator : public SamplingIntegrator {
    // bool m_remap;
    int m_maxdepth;

public:
    PathTracerIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        // NOT NECESSARY (m_remap)
        // m_remap = properties.get<bool>("remap", true);
        m_maxdepth = properties.get<int>("depth", 2);
    }
    Color Li(const Ray &ray, Sampler &rng) override {
        // Determine if the ray intersects any surfaces in the scene.
        Color integrator(1.0f);
        Ray currentRay = ray;
        int depth      = 0;
        Color contribution;
        while (depth < m_maxdepth) {
            Intersection its = m_scene->intersect(currentRay, rng);
            if (its) {
                Color emission = its.evaluateEmission().value;
                if (!m_scene->hasLights()) {
                    contribution += integrator * emission;
                }
                LightSample lightSample = m_scene->sampleLight(rng);
                if (lightSample.probability > 0) {
                    DirectLightSample directSample =
                        lightSample.light->sampleDirect(its.position, rng);

                    // Trace a secondary ray in the direction of the light.
                    Ray secondaryRay;
                    secondaryRay.origin    = its.position;
                    secondaryRay.direction = directSample.wi;
                    Intersection secondaryIts =
                        m_scene->intersect(secondaryRay, rng);
                    if (!secondaryIts ||
                        secondaryIts.t > directSample.distance) {
                        BsdfEval bsdf = its.evaluateBsdf(directSample.wi);
                        if (!bsdf.isInvalid()) {
                            contribution += integrator * directSample.weight *
                                            bsdf.value /
                                            lightSample.probability;
                        }
                    }
                }

                depth++;
                if (depth == m_maxdepth) {
                    break;
                }

                BsdfSample bsdfSample = its.sampleBsdf(rng);
                if (!bsdfSample.isInvalid()) {
                    Ray bsdfRay;
                    bsdfRay.origin    = its.position;
                    bsdfRay.direction = bsdfSample.wi;
                    Intersection secondaryIts =
                        m_scene->intersect(bsdfRay, rng);

                    Color emissiveContribution =
                        secondaryIts.evaluateEmission().value;
                    contribution +=
                        integrator * bsdfSample.weight * emissiveContribution;
                    currentRay = Ray(its.position, bsdfSample.wi);
                } else {
                    break;
                }
                float cosTheta =
                    std::max(0.f, its.shadingNormal.dot(bsdfSample.wi));
                integrator *= bsdfSample.weight * cosTheta;
                // integrator /= 0.9f;
                depth++;

            } else {
                contribution += integrator * its.evaluateEmission().value;
                break;
            }
        }
        return contribution;
    }
    std::string toString() const override {
        return tfm::format(
            "DirectIntegrator[\n"
            // "  remap = %s,\n"
            "  sampler = %s,\n"
            "  image = %s,\n"
            "]",
            // m_remap,
            indent(m_sampler),
            indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(PathTracerIntegrator, "pathtracer")
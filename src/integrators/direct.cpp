#include <lightwave.hpp>

namespace lightwave {

class DirectIntegrator : public SamplingIntegrator {
    // bool m_remap;

public:
    DirectIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        // NOT NECESSARY (m_remap)
        // m_remap = properties.get<bool>("remap", true);
    }
    Color Li(const Ray &ray, Sampler &rng) override {
        // Determine if the ray intersects any surfaces in the scene.
        Intersection its = m_scene->intersect(ray, rng);
        if (its) {
            Color contribution;
            Color emission = its.evaluateEmission().value;
            if (!m_scene->hasLights()) {
                contribution += emission;
            }

            // If a surface intersection occurs, you will need to get a light
            // sample using the provided sampleLight function. Then, get its
            // direct contribution information (emission weight, direction 𝜔 and
            // distance) using the 𝑖 sampleDirect function of the light
            // associated with the light sample.
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

                // And if the light is not occluded add its contribution
                // weighted by the bsdf value at the first intersection.
                if (!secondaryIts || secondaryIts.t > directSample.distance) {
                    float cosTheta =
                        std::max(0.f, its.shadingNormal.dot(directSample.wi));
                    BsdfEval bsdf = its.evaluateBsdf(directSample.wi);
                    if (!bsdf.isInvalid()) {
                        contribution += directSample.weight * bsdf.value *
                                       cosTheta / lightSample.probability;
                    }
                }
            }

            // There is an intersection with a surface, but this surface also
            // emits light. (Soft shadow)
            BsdfSample bsdfSample = its.sampleBsdf(rng);
            if (!bsdfSample.isInvalid()) {
                // Trace a secondary ray in the sampled BSDF direction.
                Ray bsdfRay;
                bsdfRay.origin            = its.position;
                bsdfRay.direction         = bsdfSample.wi;
                Intersection secondaryIts = m_scene->intersect(bsdfRay, rng);

                if (secondaryIts) {
                    // If the secondary ray hits an emissive surface, evaluate
                    // its emission.
                    Color emissiveContribution =
                        secondaryIts.evaluateEmission().value;
                    contribution += bsdfSample.weight * emissiveContribution;
                }
            }
            return contribution;
        } else {
            // If no surface interaction was found, add the contribution of the
            // background environment map (if any). For this you can use the
            // evaluateEmission function of the intersection. Scene has an
            // optional background light, which provides color when rays exit
            // the scene. ref<BackgroundLight> m_background; BackgroundLight is
            // a light initialised with a direction
            return its.evaluateEmission().value;
        }
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

REGISTER_INTEGRATOR(DirectIntegrator, "direct")
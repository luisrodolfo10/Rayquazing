#include <cmath>
#include <lightwave.hpp>
#include <vector>

namespace lightwave {

class Bloom : public Postprocess {
    float threshold;
    float intensity;
    float blurRadius;

public:
    Bloom(const Properties &properties) : Postprocess(properties) {
        threshold  = properties.get<float>("threshold", 1.0f);
        intensity  = properties.get<float>("intensity", 0.0f);
        blurRadius = properties.get<float>("blur_radius", 5.0f);
    }

    void execute() override {
        Point2i resolution = m_input->resolution();
        int width          = resolution.x();
        int height         = resolution.y();

        m_output->initialize(resolution);

        // Image with bright areas
        Image brightAreas(resolution);

        // Extract bright areas where bloom is applied
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Point2i pixel(x, y);
                Color color      = m_input->get(pixel);
                float brightness = (color.r() + color.g() + color.b()) / 3.0f;
                if (brightness > threshold) {
                    brightAreas.get(pixel) = color;
                } else {
                    brightAreas.get(pixel) = Color(0.0f);
                }
            }
        }

        // Apply Gaussian blur to spread the bloom effect
        Image bloomEffect =
            applyGaussianBlur(brightAreas, width, height, blurRadius);

        // Combine bloom effect with original image
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Point2i pixel(x, y);
                Color original   = m_input->get(pixel);
                Color bloom      = bloomEffect.get(pixel);
                Color finalColor = original + bloom * intensity;

                m_output->get(pixel) = finalColor;
            }
        }

        m_output->save();
    }

    std::string toString() const override {
        return tfm::format(
            "Bloom[\n"
            "  threshold = %f\n"
            "  intensity = %f\n"
            "  blur_radius = %f\n"
            "]",
            threshold,
            intensity,
            blurRadius);
    }

private:
    Image applyGaussianBlur(const Image &input, int width, int height,
                            float radius) {
        Image output(Point2i(width, height));

        int kernelSize = std::ceil(radius) * 2 + 1;
        std::vector<float> kernel(kernelSize);
        float sigma = radius / 2.0f;
        float sum   = 0.0f;

        // Gaussian kernel
        for (int i = 0; i < kernelSize; i++) {
            int x     = i - kernelSize / 2;
            kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
            sum += kernel[i];
        }

        for (float &val : kernel) {
            val /= sum;
        }

        // Gaussian blur horizontally
        Image temp(Point2i(width, height));
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Color sumColor(0.0f);
                for (int k = 0; k < kernelSize; k++) {
                    int offset = k - kernelSize / 2;
                    int nx     = std::clamp(x + offset, 0, width - 1);
                    sumColor += input.get(Point2i(nx, y)) * kernel[k];
                }
                temp.get(Point2i(x, y)) = sumColor;
            }
        }

        // Gaussian blur vertically
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Color sumColor(0.0f);
                for (int k = 0; k < kernelSize; k++) {
                    int offset = k - kernelSize / 2;
                    int ny     = std::clamp(y + offset, 0, height - 1);
                    sumColor += temp.get(Point2i(x, ny)) * kernel[k];
                }
                output.get(Point2i(x, y)) = sumColor;
            }
        }

        return output;
    }
};

} // namespace lightwave

REGISTER_POSTPROCESS(Bloom, "bloom")

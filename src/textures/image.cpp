#include <lightwave.hpp>

namespace lightwave {

class ImageTexture : public Texture {
    enum class BorderMode {
        Clamp,
        Repeat,
    };

    enum class FilterMode {
        Nearest,
        Bilinear,
    };

    ref<Image> m_image;
    float m_exposure;
    BorderMode m_border;
    FilterMode m_filter;

    float clamp(float value) const {
        float min_val = 0.f;
        float max_val = 1.f;
        return max(min_val, min(value, max_val));
    }

    float repeat(float value) const { return value - floor(value); }

public:
    ImageTexture(const Properties &properties) {
        if (properties.has("filename")) {
            m_image = std::make_shared<Image>(properties);
        } else {
            m_image = properties.getChild<Image>();
        }
        m_exposure = properties.get<float>("exposure", 1);

        // clang-format off
        m_border = properties.getEnum<BorderMode>("border", BorderMode::Repeat, {
            { "clamp", BorderMode::Clamp },
            { "repeat", BorderMode::Repeat },
        });

        m_filter = properties.getEnum<FilterMode>("filter", FilterMode::Bilinear, {
            { "nearest", FilterMode::Nearest },
            { "bilinear", FilterMode::Bilinear },
        });
        // clang-format on
    }

    Color evaluate(const Point2 &uv) const override {
        float u = uv[0];
        float v = uv[1];
        if (m_border == BorderMode::Clamp) {
            u = clamp(u);
            v = clamp(v);
        } else if (m_border == BorderMode::Repeat) {
            u = repeat(u);
            v = repeat(v);
        } else {
            throw std::runtime_error("Invalid BorderMode value.");
        }

        int width  = m_image->resolution().x();
        int height = m_image->resolution().x();
        float x    = u * (width - 1);
        float y    = v * (height - 1);

        if (m_filter == FilterMode::Nearest) {
            int ix = round(x);
            int iy = round(y);
            return m_image->get(Point2i(ix, iy));
        } else if (m_filter == FilterMode::Bilinear) {
            int x0 = floor(x);
            int y0 = floor(y);
            int x1 = min(x0 + 1, width - 1);
            int y1 = min(y0 + 1, height - 1);

            float tx = x - x0;
            float ty = y - y0;

            // Interpolate
            Color c00 = m_image->get(Point2i(x0, y0));
            Color c10 = m_image->get(Point2i(x1, y0));
            Color c01 = m_image->get(Point2i(x0, y1));
            Color c11 = m_image->get(Point2i(x1, y1));

            Color c0 = c00 * (1 - tx) + c10 * tx;
            Color c1 = c01 * (1 - tx) + c11 * tx;

            return (c0 * (1 - ty) + c1 * ty) * m_exposure;
        } else {
            return Color(1.f, 0.f, 0.f);
            // throw std::runtime_error("Invalid FilterMode value.");
        }
    }

    std::string toString() const override {
        return tfm::format(
            "ImageTexture[\n"
            "  image = %s,\n"
            "  exposure = %f,\n"
            "]",
            indent(m_image),
            m_exposure);
    }
};

} // namespace lightwave

REGISTER_TEXTURE(ImageTexture, "image")

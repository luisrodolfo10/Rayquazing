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

    int clamp(int value, int max_val) const {
        int min_val = 0;
        return max(min_val, min(value, max_val));
    }

    int repeat(int value, int hw) const { return ((value % hw) + hw) % hw; }

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
        float u    = uv[0];
        float v    = -uv[1] + 1;
        int width  = m_image->resolution().x();
        int height = m_image->resolution().y();
        float x    = u * width;
        float y    = v * height;

        if (m_filter == FilterMode::Nearest) {
            int ix = floor(x);
            int iy = floor(y);
            if (m_border == BorderMode::Clamp) {
                ix = clamp(ix, width - 1);
                iy = clamp(iy, height - 1);
            } else if (m_border == BorderMode::Repeat) {
                ix = repeat(ix, width);
                iy = repeat(iy, height);
            } else {
                throw std::runtime_error("Invalid BorderMode value.");
            }
            return m_image->get(Point2i(ix, iy));
        } else if (m_filter == FilterMode::Bilinear) {
            x      = x - 0.5;
            y      = y - 0.5;
            int x0 = floor(x);
            int y0 = floor(y);
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            float tx = x - x0;
            float ty = y - y0;

            if (m_border == BorderMode::Clamp) {
                x0 = clamp(x0, width - 1);
                x1 = clamp(x1, width - 1);
                y0 = clamp(y0, height - 1);
                y1 = clamp(y1, height - 1);
            } else if (m_border == BorderMode::Repeat) {
                x0 = repeat(x0, width);
                x1 = repeat(x1, width);
                y0 = repeat(y0, height);
                y1 = repeat(y1, height);
            } else {
                throw std::runtime_error("Invalid BorderMode value.");
            }

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

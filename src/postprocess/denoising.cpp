#include <OpenImageDenoise/oidn.hpp>
#include <lightwave.hpp>

namespace lightwave {
// Reference: https://www.openimagedenoise.org/documentation.html
class Denoising : public Postprocess {
    // normals and albedo are images in exporter.py
    ref<Image> m_normals;
    ref<Image> m_albedo;

public:
    Denoising(const Properties &properties) : Postprocess(properties) {
        // m_input   = properties.get<Image>("input");
        m_normals = properties.getOptional<Image>("normal");
        m_albedo  = properties.getOptional<Image>("albedo");
    }

    void execute() override {
        Point2i m_resolution = m_input->resolution();
        int width            = m_resolution.x();
        int height           = m_resolution.y();

        // Create an Open Image Denoise device
        oidn::DeviceRef device = oidn::newDevice(); // CPU or GPU if available
        // oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
        device.commit();

        // Create buffers for input/output images accessible by both host (CPU)
        // and device (CPU/GPU)
        // oidn::BufferRef colorBuf = device.newBuffer(
        //     m_input->data(), width * height * 3 * sizeof(float));
        // oidn::BufferRef albedoBuf = device.newBuffer(
        //     m_albedo->data(), width * height * 3 * sizeof(float));
        // oidn::BufferRef normalBuf = device.newBuffer(
        //     m_normals->data(), width * height * 3 * sizeof(float));
        // oidn::BufferRef outputBuf = device.newBuffer(
        //     m_output->data(), width * height * 3 * sizeof(float));
        m_output->initialize(m_resolution);

        // Create a filter for denoising a beauty (color) image using optional
        // auxiliary images too This can be an expensive operation, so try no to
        // create a new filter for every image!
        oidn::FilterRef filter =
            device.newFilter("RT"); // generic ray tracing filter
        filter.setImage(
            "color", m_input->data(), oidn::Format::Float3, width, height);
        if (!m_albedo) {
            std::cout << "Albedo not available for postprocessing" << std::endl;

        } else {
            filter.setImage("albedo",
                            m_albedo->data(),
                            oidn::Format::Float3,
                            width,
                            height);
        }
        if (!m_normals) {
            std::cout << "Normals not available for postprocessing"
                      << std::endl;
        } else {
            filter.setImage("normals",
                            m_normals->data(),
                            oidn::Format::Float3,
                            width,
                            height);
        }

        filter.setImage("output",
                        m_output->data(),
                        oidn::Format::Float3,
                        width,
                        height); // denoised

        filter.set("hdr", true);
        filter.commit();

        // Filter the beauty image
        filter.execute();

        // Check for errors
        const char *errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            std::cout << "Error: " << errorMessage << std::endl;
        }

        m_output->save();
    };

    std::string toString() const override {
        return tfm::format(
            "Denoising[\n"
            "  m_normals = %s\n"
            "  m_albedo = %s\n"
            "]",
            m_normals,
            m_albedo);
    }
};

} // namespace lightwave

REGISTER_POSTPROCESS(Denoising, "denoising")

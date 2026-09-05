#include <amrexplorer/data/LocalDatasetSession.hpp>
#include <amrexplorer/core/Request.hpp>
#include <amrexplorer/core/Result.hpp>
#include <amrexplorer/render2d/ScalarRenderer.hpp>
#include <amrexplorer/render2d/ImageBuffer.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <plotfile_path>\n";
        return 1;
    }

    std::filesystem::path plotfilePath = argv[1];
    std::string outputFilename = "slice_z0.png";

    try {
        uint64_t cacheBudget = 1024 * 1024 * 1024; 
        amrvis::LocalDatasetSession session(plotfilePath, amrvis::DatasetId{1}, cacheBudget, amrvis::StopToken{});

        amrvis::FieldId fieldId{0};
        const auto& meta = session.metadata();
        if (meta.fields.empty()) {
            std::cerr << "No fields found in plotfile\n";
            return 1;
        }
        
        amrvis::SliceRequest request;
        request.dataset = session.id();
        request.field = fieldId;
        request.normalDirection = 2; 
        request.physicalPosition = 0.0;
        request.outputSize = {512, 512};
        request.visibleRegion = meta.physicalDomain;
        request.sampling = amrvis::SamplingPolicy::Linear;
        request.composition = amrvis::CompositionPolicy::FinestAvailable;

        amrvis::ViewDataRequest viewReq{request};
        amrvis::ViewDataResult result = session.requestView(viewReq, amrvis::StopToken{});

        if (std::holds_alternative<amrvis::SliceQueryResult>(result)) {
            const auto& sliceRes = std::get<amrvis::SliceQueryResult>(result);
            const auto& plane = sliceRes.plane;

            double minVal = 1e30;
            double maxVal = -1e30;
            bool hasValid = false;
            for (size_t i = 0; i < plane.values.size(); ++i) {
                if (plane.valid[i]) {
                    minVal = std::min(minVal, (double)plane.values[i]);
                    maxVal = std::max(maxVal, (double)plane.values[i]);
                    hasValid = true;
                }
            }

            if (!hasValid) {
                std::cerr << "No valid data in slice\n";
                return 1;
            }

            amrvis::ScalarRenderSettings settings;
            settings.minimum = minVal;
            settings.maximum = maxVal;
            if (std::abs(maxVal - minVal) < 1e-6) {
                settings.minimum = minVal - 1.0;
                settings.maximum = maxVal + 1.0;
            }
            
            amrvis::ImageBuffer img = amrvis::renderScalarPlane(plane, settings);

            // Save using stb_image_write
            // img.rgba contains uint32_t (RGBA), which is exactly what stbi_write_png expects
            if (stbi_write_png(outputFilename.c_str(), img.width, img.height, 4, img.rgba.data(), img.width * 4)) {
                std::cout << "Slice saved to " << outputFilename << "\n";
            } else {
                std::cerr << "Failed to save PNG to " << outputFilename << "\n";
                return 1;
            }
        } else {
            std::cerr << "Failed to get slice result\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

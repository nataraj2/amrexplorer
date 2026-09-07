#include <amrexplorer/data/LocalDatasetSession.hpp>
#include <amrexplorer/core/Request.hpp>
#include <amrexplorer/core/Result.hpp>
#include <amrexplorer/render2d/ScalarRenderer.hpp>
#include <amrexplorer/render2d/ImageBuffer.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <cmath>
#include <string>
#include <algorithm>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace fs = std::filesystem;

void processPlotfile(const std::string& plotfilePath, const fs::path& outputDir) {
    fs::path inputPath(plotfilePath);
    std::string stem = inputPath.stem().string();
    std::string outputFilename = (outputDir / (stem + ".png")).string();

    try {
        uint64_t cacheBudget = 1024 * 1024 * 1024;
        amrvis::LocalDatasetSession session(plotfilePath, amrvis::DatasetId{1}, cacheBudget, amrvis::StopToken{});

        amrvis::FieldId fieldId{0};
        const auto& meta = session.metadata();
        if (meta.fields.empty()) {
            std::cerr << "No fields found in plotfile: " << plotfilePath << "\n";
            return;
        }

        double dx = meta.physicalDomain.upper[0] - meta.physicalDomain.lower[0];
        double dy = meta.physicalDomain.upper[1] - meta.physicalDomain.lower[1];

        int baseRes = 512;
        int outW = baseRes;
        int outH = baseRes;

        if (dx > dy) {
            outH = std::max(1, (int)(baseRes * (dy / dx)));
        } else if (dy > dx) {
            outW = std::max(1, (int)(baseRes * (dx / dy)));
        }

        amrvis::SliceRequest request;
        request.dataset = session.id();
        request.field = fieldId;
        request.normalDirection = 2;
        request.physicalPosition = 0.0;
        request.outputSize = {outW, outH};
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
                std::cerr << "No valid data in slice: " << plotfilePath << "\n";
                return;
            }

            amrvis::ScalarRenderSettings settings;
            settings.minimum = minVal;
            settings.maximum = maxVal;
            if (std::abs(maxVal - minVal) < 1e-6) {
                settings.minimum = minVal - 1.0;
                settings.maximum = maxVal + 1.0;
            }

            amrvis::ImageBuffer img = amrvis::renderScalarPlane(plane, settings);

            // FLIP ONLY Y-AXIS:
            // Data origin is bottom-left, PNG is top-left.
            int w = img.width;
            int h = img.height;
            std::vector<uint32_t> flippedRgba(w * h);

            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    // Correct the Y-axis (vertical flip)
                    // Keep X as it is (no horizontal flip)
                    flippedRgba[y * w + x] = img.rgba[(h - 1 - y) * w + x];
                }
            }

            if (stbi_write_png(outputFilename.c_str(), w, h, 4,
                               reinterpret_cast<const unsigned char*>(flippedRgba.data()),
                               w * 4)) {
                std::cout << "Saved: " << outputFilename << " (" << w << "x" << h << ")\n";
            } else {
                std::cerr << "Failed to save PNG: " << outputFilename << "\n";
            }
        } else {
            std::cerr << "Failed to get slice result for: " << plotfilePath << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing " << plotfilePath << ": " << e.what() << "\n";
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filelist.txt>\n";
        return 1;
    }

    std::string listFilePath = argv[1];
    std::ifstream fileList(listFilePath);
    if (!fileList.is_open()) {
        std::cerr << "Could not open file list: " << listFilePath << "\n";
        return 1;
    }

    fs::path outputDir = "images";
    try {
        if (!fs::exists(outputDir)) {
            fs::create_directory(outputDir);
            std::cout << "Created directory: " << outputDir << "\n";
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << "\n";
        return 1;
    }

    std::string line;
    std::vector<std::string> plotfiles;
    while (std::getline(fileList, line)) {
        if (!line.empty()) {
            plotfiles.push_back(line);
        }
    }

    std::cout << "Found " << plotfiles.size() << " plotfiles in list.\n";
    for (const auto& plotfile : plotfiles) {
        std::cout << "Processing: " << plotfile << "...\n";
        processPlotfile(plotfile, outputDir);
    }

    return 0;
}

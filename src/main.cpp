#include <iostream>
#include <fstream>
#include <cstring>
#include "MetadataLoader.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_global-metadata.dat>" << std::endl;
        return 1;
    }

    std::string metadataPath = argv[1];
    MetadataLoader loader;

    std::cout << "Target: " << metadataPath << std::endl;

    if (argc >= 3) {
        std::string libPath = argv[2];
        std::cout << "Library: " << libPath << std::endl;
        if (!loader.LoadLibrary(libPath)) {
            std::cerr << "Warning: Failed to load library." << std::endl;
        }
    }

    std::cout << "Loading metadata file..." << std::endl;
    if (loader.LoadFile(metadataPath)) {
        std::cout << "Metadata loaded successfully!" << std::endl;

        // Print basic header information
        const Il2CppGlobalMetadataHeader* hdr = loader.getHeader();
        std::cout << "Header sanity: 0x" << std::hex << hdr->sanity << std::dec << std::endl;
        std::cout << "Header version: " << hdr->version << std::endl;
        std::cout << "String count: " << hdr->stringCount << std::endl;
        std::cout << "Type definition count: " << hdr->typeDefinitionsCount << std::endl;
        std::cout << "Method count: " << hdr->methodsCount << std::endl;

        // Test string access to see if we can retrieve any strings
        std::cout << "Testing string access..." << std::endl;
        const char* testString = nullptr;
        for (int i = 0; i < 100; i++) {
            testString = loader.GetStringFromIndexPublic(i);
            if (testString && strlen(testString) > 0) {
                std::cout << "String at index " << i << ": " << testString << std::endl;
                break;
            }
        }
        if (!testString || strlen(testString) == 0) {
            std::cout << "No valid strings found in first 100 indices" << std::endl;
        }

        std::cout << "Basic tests completed, now starting processing..." << std::endl;
        loader.Process();
        std::cout << "Processing completed!" << std::endl;
    } else {
        std::cerr << "Failed to load metadata." << std::endl;
        std::string log = get_verbose_log();
        std::cerr << log << std::endl;
        return 1;
    }

    // Print verbose log
    std::string log = get_verbose_log();
    std::cout << log << std::endl;

    return 0;
}
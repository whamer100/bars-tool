#ifndef BARS_TOOL_BARSTYPES_H
#define BARS_TOOL_BARSTYPES_H

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include "logging.h"

const uint32_t ZSTD_MAGIC = 4247762216;
const uint32_t BARS_MAGIC = 1397899586;
const uint32_t AMTA_MAGIC = 1096043841;

const uint32_t BWAV_MAGIC = 1447122754;

std::string getExt(uint32_t magic)
{
    switch (magic)
    {
        case BWAV_MAGIC: return "bwav";
        default: {
            logging::warn("Unknown magic {}", (unsigned int)magic);
            return "unk";
        }
    }
}

struct BARSFOSEntry {
    uint32_t amta_offset;
    uint32_t asset_offset;
};

struct BARSFileEntry {
    std::string fname;
    uint32_t format;
    uint32_t size;
    char* data;
};

struct AMTAEntry {
    union {
        uint16_t version;
        struct {
            uint8_t version_low;
            uint8_t version_high;
        };
    };
    uint32_t size;
    uint32_t fname_offset;
    uint32_t crc;
    std::string fname;
};

struct BARSFile {
    uint32_t fsize;
    union {
        uint16_t version;
        struct {
            uint8_t version_low;
            uint8_t version_high;
        };
    };
    uint32_t entry_count;
    std::vector<uint32_t> hash_table{};
    std::vector<BARSFOSEntry> file_offset_table{};
    std::vector<AMTAEntry> amta_table{};
    std::vector<BARSFileEntry> file_entries{};
};

struct BARSFileContext {
    std::filesystem::path path;
    std::filesystem::path target;
    std::stringstream f;
    BARSFile file;
};

#endif //BARS_TOOL_BARSTYPES_H

#ifndef BARS_TOOL_HANDLERS_H
#define BARS_TOOL_HANDLERS_H

namespace zstd {
#include <zstd.h>
} // namespace zstd

#include <cassert>
#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <ranges>
#include <filesystem>

#include "barstypes.h"
#include "macros.h"
#include "logging.h"
#include "crc32.h"

namespace fs = std::filesystem;

void main_handler(const std::string& input, const std::string& output);
void magic_handler(BARSFileContext& fileContext);
void zlib_handler(BARSFileContext& fileContext);
void bars_handler(BARSFileContext& fileContext);

void main_handler(const std::string& input, const std::string& output)
{
    if (!fs::exists(input))
    {
        logging::fatal("Input file/folder does not exist!");
        exit(1);
    }

    auto target = fs::path(output);

    fs::create_directories(target);

    std::vector<fs::path> files{};

    if (fs::is_directory(input))
        for (const auto& entry : fs::directory_iterator(input))
        {
            if (!fs::is_directory(entry))
                files.emplace_back(entry.path());
        }
    else
        files.emplace_back(input);

    logging::info("Files to analyze: {}", files.size());

    for (const auto& i : files)
    {
        logging::info("Parsing file \"{}\"", i.filename().string());
        auto p = i.string();
        auto f = std::ifstream(p, std::ifstream::binary);
        f.seekg(0, f.end);
        uint32_t fsize = f.tellg();
        f.seekg(0, f.beg);

        char* tmpdata = new char[fsize];
        f.read(tmpdata, fsize);
        f.seekg(0, f.beg);

        BARSFileContext fileContext{
            i,
            target,
            std::stringstream(),
            {}
        };
        fileContext.f.write(tmpdata, fsize);
        delete [] tmpdata;

        magic_handler(fileContext);
    }
}

void magic_handler(BARSFileContext& fileContext)
{
    std::stringstream& f = fileContext.f;
    f.seekg(0, f.beg);

    uint32_t magic;
    f.tread(magic, uint32_t);
    // std::cout << magic << std::endl;
    if (magic == ZSTD_MAGIC)
        zlib_handler(fileContext);
    else if (magic == BARS_MAGIC)
        bars_handler(fileContext);
    else
    {
        logging::fatal("Unknown file type found! (Magic: {})", (unsigned int)magic);
        exit(1);
    }
}

void zlib_handler(BARSFileContext& fileContext)
{
    std::stringstream& f = fileContext.f;

    f.seekg(0, f.end);
    uint32_t fsize = f.tellg();
    f.seekg(0, f.beg);

    char* zsdata = new char[fsize];
    f.read(static_cast<char *>(zsdata), fsize);

    uint32_t decsize = zstd::ZSTD_getFrameContentSize(zsdata, fsize);

    logging::info("Decompressing {} bytes -> {} bytes.", (unsigned long long)fsize, (unsigned long long)decsize);

    char* decdata = new char[decsize];
    size_t result = zstd::ZSTD_decompress(decdata, decsize, zsdata, fsize);
    if (zstd::ZSTD_isError(result))
    {
        logging::fatal("AAAAAAA WHAT IS HAPPENING !panic {}", zstd::ZSTD_getErrorName(result));
    }

    f.seekg(0, f.beg);
    ssclear(f);
    f.write(decdata, decsize);

    delete [] zsdata;
    delete [] decdata;

    magic_handler(fileContext);
}

void bars_handler(BARSFileContext& fileContext)
{
    logging::info("BARS File identified.");
    std::stringstream& f = fileContext.f;
    BARSFile& file = fileContext.file;
    std::string outpath = (fileContext.target /= (fileContext.path.filename().string() + "_out")).string();
    fs::create_directories(outpath);

    f.seekg(0, f.end);
    uint32_t fsize = f.tellg();
    f.seekg(0, f.beg);
    uint32_t magic;
    uint16_t bom;
    f.tread(magic, uint32_t);
    assert(magic == BARS_MAGIC); // this should never abort :monkaS:
    f.tread(file.fsize, uint32_t);
    assert(fsize == file.fsize);
    f.tread(bom, uint16_t);
    if (bom == 0xFFFE)
    {
        logging::fatal("Only little-endian formatting supported!");
        exit(1);
    }
    f.tread(file.version, uint16_t);
    logging::info("BARS Version {}.{}", (int)file.version_high, (int)file.version_low);
    f.tread(file.entry_count, uint32_t);
    logging::info("BARS Entries to parse: {}", (unsigned int)file.entry_count);
    // parse hash table
    for (auto _ : range(file.entry_count))
    {
        uint32_t crc;
        f.qread(crc);
        file.hash_table.emplace_back(crc);
    }
    // parse file offset table
    for (auto _ : range(file.entry_count))
    {
        BARSFOSEntry fos{};
        f.qread(fos.amta_offset);
        f.qread(fos.asset_offset);
        file.file_offset_table.emplace_back(fos);
    }
    // parse amta entries
    for (auto i : range(file.entry_count))
    {
        AMTAEntry amta{};
        uint32_t amta_magic;
        auto amta_offset = file.file_offset_table[i].amta_offset;
        f.seekg(amta_offset, f.beg);
        f.qread(amta_magic);
        assert(amta_magic == AMTA_MAGIC);
        f.ignore(sizeof(uint16_t)); // ignore bom mark
        f.qread(amta.version);
        if (amta.version_high < 5)
        {
            logging::warn("AMTA Version < 5, file names may not be accurate! (index={},off={})", i, amta_offset);
        }
        f.qread(amta.size);
        f.seekg(amta_offset + 0x24);
        f.qread(amta.fname_offset);
        f.qread(amta.crc);
        uint32_t fname_offset = amta_offset + 0x24 + amta.fname_offset;
        f.seekg(fname_offset);
        char* amta_fname = new char[256];
        f.get(amta_fname, 256, '\0');
        amta.fname = std::string(amta_fname);

        file.amta_table.emplace_back(amta);

        auto crc = crc32(amta.fname);

        // logging::log("[{}] {} -> {}", amta.fname, crc, amta.crc);
        if (crc != amta.crc)
        {
            logging::fatal("Something went horribly wrong at: {}, with subfile {}", (unsigned int)amta_offset, amta.fname);
            logging::fatal("Additional info: {}, {}, v{}.{}", (unsigned int)fname_offset, (unsigned int)amta.size,
                           (int)amta.version_high, (int)amta.version_low);
        }

        delete [] amta_fname;
    }
    // parse file entries
    size_t total_bytes = 0;
    for (auto i : range(file.entry_count))
    {
        BARSFileEntry fileEntry{};
        uint32_t file_magic;
        auto file_offset = file.file_offset_table[i].asset_offset;
        AMTAEntry& amta = file.amta_table[i];
        f.seekg(file_offset, f.beg);
        auto eHash = file.hash_table[i];
        auto eCRC = amta.crc;
        if (eHash == 2203811915 && eCRC == 3422978971)
        {
            logging::funny("CRC mismatch has occured! But this is actually Nintendo's fault.");
            logging::funny("If you are curious as to what is going on, open handlers.h and read starting at line 227");
            /*
             * At offsets 0x1D80 and 0x84 of Object_BillboardNpcBowling.bars (decompress the file using zstd)
             * from Nintendo Switch Sports (as of v1.2), someone at Nintendo made a single capitalization typo
             * in one of the entries somehow.
             * Specifically the lowercase m in "SE_NPC_Lv2_M_middleA_16" which produces the CRC32 CC06839B.
             * Changing the text to say "SE_NPC_Lv2_M_MiddleA_16" produces the correct CRC, being 835B804B.
             *
             * Just typical Nintendo being Nintendo.
             *
             * Nintendo, if you're seeing this, hi.
             * I love you guys, but fix your games lmao
             * */
        } else {
            assert(eHash == eCRC);
        }
        f.seekg(file_offset);
        std::array<char, 4> asset_magic{};
        f.read(asset_magic.data(), 4);
        std::stringstream ss{};
        ss.write(asset_magic.data(), 4);

        std::array<char, 4> temp{};
        while (true)
        {
            f.read(temp.data(), 4);
            if (std::equal(asset_magic.begin(), asset_magic.end(),
                           temp.begin(), temp.end()) || f.eof())
            {
                break;
            }
            ss.write(temp.data(), 4);
        }

        ss.seekg(0, ss.end);
        size_t outfsize = ss.tellg();
        ss.seekg(0, ss.beg);
        total_bytes += outfsize;

        uint32_t asset_magic_i{};
        memcpy(&asset_magic_i, &asset_magic[0], sizeof(asset_magic_i));
        std::string outfname = amta.fname + "." + getExt(asset_magic_i);
        std::string outfilepath = (fs::path(outpath) /= outfname).string();

        std::ofstream os(outfilepath, std::ofstream::binary);
        os << ss.rdbuf();
    }
    logging::info("Finished writing to disk.");
    logging::info("Total bytes written: {}", (unsigned int)total_bytes);
}

#endif //BARS_TOOL_HANDLERS_H

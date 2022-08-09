#include <iostream>
#include <argparse/argparse.hpp>
#include "handlers.h"

int main(int argc, char** argv) {
    auto parser = argparse::ArgumentParser(argc, argv)
        .prog("bars-tool")
        .description("A utility to extract BARS files. "
                     "(currently only supporting Nintendo Switch Sports and other v5 AMTA metadata files)");
    
    parser.add_argument("input").help("Input file or directory");
    parser.add_argument("output").help("Output directory (creates subdirectory for each input file)");

    if (argc <= 1)
    {
        parser.print_help();
        exit(1);
    }

    auto const result = parser.parse_args();

    main_handler(
            result.get<std::string>("input"),
            result.get<std::string>("output")
    );

    return 0;
}

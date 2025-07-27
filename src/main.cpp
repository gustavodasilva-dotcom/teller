#include <fstream>
#include <iostream>
#include <variant>

#include "./tokenization.hpp"
#include "./parser.hpp"
#include "./generation.hpp"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Incorrect usage." << std::endl;
        std::cerr << "hydro <input.hy>" << std::endl;
        return EXIT_FAILURE;
    }

    std::filesystem::path file_path = argv[1];

    if (!std::filesystem::exists(file_path))
    {
        std::cerr << "The file " << file_path << " does not exist." << std::endl;
        return EXIT_FAILURE;
    }

    if (!file_path.has_extension() || file_path.extension() != ".hy")
    {
        std::cerr << "Invalid Hydrogen file." << std::endl;
        return EXIT_FAILURE;
    }

    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(file_path, std::ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();

    Parser parser(std::move(tokens));
    std::optional<NodeProg> prog = parser.parse_prog();

    if (!prog.has_value())
    {
        std::cerr << "Invalid program." << std::endl;
        exit(EXIT_FAILURE);
    }

    Generator generator(prog.value());
    {
        std::fstream file("out.asm", std::ios::out);
        file << generator.gen_prog();
    }

    system("nasm -felf64 out.asm");
    system("ld out.o -o out");

    return EXIT_SUCCESS;
}

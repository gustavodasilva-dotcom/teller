#include <fstream>
#include <iostream>

#include "./generation.hpp"
#include "./tokenization.hpp"
#include "./parser.hpp"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Incorrect usage." << std::endl;
        std::cerr << "hydro <input.hy>" << std::endl;
        return EXIT_FAILURE;
    }

    std::filesystem::path file_path = argv[1];

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
    std::optional<NodeExit> tree = parser.parse();

    if (!tree.has_value())
    {
        std::cerr << "No exit statement found." << std::endl;
        exit(EXIT_FAILURE);
    }

    Generator generator(tree.value());
    {
        std::fstream file("out.asm", std::ios::out);
        file << generator.generate();
    }

    system("nasm -felf64 out.asm");
    system("ld out.o -o out");

    return EXIT_SUCCESS;
}

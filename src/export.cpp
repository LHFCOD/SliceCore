#include "export.h"
#include <iostream>

ComDocIO *ReadComDocFile(const char *file_name) {
    ComDocIO *io = nullptr;
    try {
        io = new ComDocIO(file_name);
    }
    catch (std::exception &ex) {
        std::cout << "ReadComDocFile exception occur: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception occur: " << std::endl;
    }
    return io;
}

void ReleaseComDocFile(ComDocIO *io) {
    if (io != nullptr) {
        delete io;
        io = nullptr;
    }
}

FileBlock *ReadFromPath(ComDocIO *io, const char *path) {
    FileBlock *pointer = nullptr;
    try {
        if (io != nullptr) {
            pointer = io->ReadFromPath(path);
        }
    }
    catch (std::exception &ex) {
        std::cout << "ReadFromPath exception occur: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception occur: " << std::endl;
    }
    return pointer;
}

uint32_t GetBlockLength(FileBlock *block) {
    try {
        if (block != nullptr) {
            return block->length_;
        }
    }
    catch (std::exception &ex) {
        std::cout << "GetBlockLength exception occur: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception occur: " << std::endl;
    }
    return 0;
}

char *GetBlockData(FileBlock *block) {
    try {
        if (block != nullptr) {
            return block->pointer_;
        }
    }
    catch (std::exception &ex) {
        std::cout << "GetBlockData exception occur: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cout << "Unknown exception occur: " << std::endl;
    }
    return nullptr;
}

void ReleaseFileBlock(FileBlock *block) {
    if (block != nullptr) {
        delete block;
        block = nullptr;
    }
}
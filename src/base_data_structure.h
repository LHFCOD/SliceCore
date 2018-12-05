/**
 \* Created with CLion.
 \* User: lihongfei
 \* Date: 18-12-5
 \* Time: 下午3:07
 \* To change this template use File | Settings | File Templates.
 \* Description: 文件块结构，表示复合文档中的某一个文件
 \*/
#pragma once

#include <stdlib.h>
#include <cstring>
#include <cstdint>

namespace compound_doc_io {
    class FileBlock {
    public:
        char *pointer_ = nullptr;
        uint32_t length_ = 0;

        FileBlock(char *pointer, uint32_t length) {
            this->pointer_ = pointer;
            this->length_ = length;
        };

        FileBlock() {};

        FileBlock(const FileBlock &block) {
            length_ = block.length_;
            if (block.pointer_ != nullptr && block.length_ > 0) {
                pointer_ = (char *) malloc(length_);
                memcpy(pointer_, block.pointer_, length_);
            }
        }

        FileBlock &operator=(const FileBlock &block) {
            if (this != &block) {
                if (this->pointer_ != nullptr) {
                    free(this->pointer_);
                }
                this->length_ = block.length_;
                this->pointer_ = (char *) malloc(length_);
                memcpy(pointer_, block.pointer_, length_);
            }
            return *this;
        }

        ~FileBlock() {
            if (pointer_ != nullptr) {
                free(pointer_);
                pointer_ = nullptr;
            }
        };
    };
}

/**
 \* Created with CLion.
 \* User: lihongfei
 \* Date: 18-12-5
 \* Time: 下午3:07
 \* To change this template use File | Settings | File Templates.
 \* Description: 复合文档头文件
 \*/
#pragma once

#include <cstdint>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/algorithm/string.hpp>
#include "base_data_structure.h"
#include <string>
#include <vector>
#include <algorithm>

#define GET_DATA(ptr_, off) (((const char *)ptr_) + off)
#define GET_BYTE(ptr_, off) (*(const char *)GET_DATA(ptr_, off))
#define GET_INT16(ptr_, off) (*(const short *)GET_DATA(ptr_, off))
#define GET_UINT16(ptr_, off) (*(const unsigned short *)GET_DATA(ptr_, off))
#define GET_INT32(ptr_, off) (*(const int32_t *)GET_DATA(ptr_, off))
#define GET_UINT32(ptr_, off) (*(const uint32_t *)GET_DATA(ptr_, off))
#define GET_INT64(ptr_, off) (*(const int64_t *)GET_DATA(ptr_, off))
#define GET_UINT64(ptr_, off) (*(const uint64_t *)GET_DATA(ptr_, off))

#define MAKE_UI32(n) n##u
#define MAKE_UI64(n) n##ull

namespace compound_doc_io {
    struct Header {
        char file_id[8];
        char unique_id[16];
        unsigned short revision_number;
        unsigned short version_number;
        unsigned short byte_order;
        uint32_t sector_size;
        uint32_t short_sector_size;
        uint32_t sector_number;
        int32_t directory_first_sid;
        uint32_t standard_minimum_size;
        int32_t SSAT_first_sid;
        uint32_t SSAT_sector_size;
        int32_t MSAT_first_sid;
        uint32_t MSAT_sector_size;
        int32_t MSAT_first_part[109];
    };
    struct Directory {
        char entry_name[64] = {0};
        unsigned short name_size = 0;
        char entry_type;
        char node_color;
        int32_t left_DID;
        int32_t right_DID;
        int32_t root_DID;
        char unique_ID[16];
        int32_t user_tag;
        int64_t entry_create_timestamp;
        int64_t entry_last_modify_timestamp;
        int32_t entry_first_sid;
        uint32_t stream_size;
        int32_t DID;
    };

    class ComDocIO {
    public:
        ComDocIO();

        ComDocIO(std::string file_path);

        ~ComDocIO();

    public:
        Header header_;
        const char *kFilePtr_;
        boost::iostreams::mapped_file_source file;
        char *ptr_;
        std::vector<int32_t> short_stream_sid_array_;
        std::vector<int32_t> SSAT_first_sid_array_;
        std::vector<int32_t> MSAT_first_sid_array_;
        int sid_count_per_sector_ = 0;

        void ConfigureHeader();

        void ReadFile(std::string FilePath);

        Directory ReadDirectory(uint32_t DID);

        int32_t FindNextSID(int32_t SID);

        int32_t FindSID(int32_t SID, uint32_t Offset);

        void ConfigureSSATFirstSID();

        void ConfigureShortStreamStorage();

        int32_t FindNextShortSID(int32_t SID);

        char *GetAddressFromShortSID(int32_t SID);

        char *ReadStreamFromSID(int32_t SID, uint64_t len);

        char *ReadShortStreamFromSID(int32_t ssid, uint32_t len);

        void ConfigureMSATFirstSID();

        void SearchMSATFirstSID(int32_t SID);

        FileBlock *ReadFromPath(std::string path);

        Directory FindDirectoryFromName(uint32_t RootDID, std::string name);

        inline int CompareString(std::string str1, std::string str2) {
            if (str1.size() < str2.size())
                return -1;
            else if (str1.size() > str2.size())
                return 1;
            else if (str1.size() == str2.size()) {
                boost::to_upper(str1);
                boost::to_upper(str2);
                if (str1 < str2)
                    return -1;
                else if (str1 > str2)
                    return 1;
                else
                    return 0;
            }
        };
    };
}

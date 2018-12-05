#include "compound_doc_io.h"
#include <boost/algorithm/string.hpp>
#include <stdexcept>

using namespace compound_doc_io;

ComDocIO::ComDocIO() {
}

/**
 * 传入文件路径，解析复合文档的基本信息
 * @param file_path
 */
ComDocIO::ComDocIO(std::string file_path) {
    ReadFile(std::move(file_path));
    ConfigureHeader();
    ConfigureMSATFirstSID();
    ConfigureSSATFirstSID();
    ConfigureShortStreamStorage();
}

/**
 * 传入复合文档的虚拟路径，得到文件块
 * @param path
 * @return
 */
FileBlock *ComDocIO::ReadFromPath(std::string path) {
    std::vector<std::string> sub_path_array;
    std::string sub_path = path.substr(1);
    boost::split(sub_path_array, sub_path, boost::is_any_of("/"));
    auto last_path_directory = ReadDirectory(0);
    for (auto &child_path : sub_path_array) {
        if (last_path_directory.root_DID < 0) {
            throw std::logic_error("last_path_directory.root_DID less than zero");
        }
        last_path_directory = FindDirectoryFromName((uint32_t) last_path_directory.root_DID, child_path);
    }
    char *p;
    if (last_path_directory.stream_size < header_.standard_minimum_size)
        p = ReadShortStreamFromSID(last_path_directory.entry_first_sid, last_path_directory.stream_size);
    else
        p = ReadStreamFromSID(last_path_directory.entry_first_sid, last_path_directory.stream_size);
    auto *block = new FileBlock(p, last_path_directory.stream_size);
    return block;
}

/**
 * 传入入口的目录id，搜索目录红黑树，查找指定名字的目录
 * @param root_DID
 * @param name
 * @return
 */
Directory ComDocIO::FindDirectoryFromName(uint32_t root_DID, std::string name) {
    auto directory = ReadDirectory(root_DID);
    std::string entry;
    for (int i = 0; i < directory.name_size; i++) {
        if (i % 2 == 0 && directory.entry_name[i] != '\0') {
            entry.insert(entry.end(), directory.entry_name[i]);
        }
    }
    int comp = CompareString(name, entry);
    if (comp == 0)
        return directory;
    else if (comp == -1) {
        if (directory.left_DID < 0) {
            throw std::logic_error("Can not find specified directory");
        } else {
            auto dir = FindDirectoryFromName((uint32_t) directory.left_DID, name);
            return dir;
        }
    } else {
        if (directory.right_DID < 0) {
            throw std::logic_error("Can not find specified directory");
        } else {
            auto dir = FindDirectoryFromName((uint32_t) directory.right_DID, name);
            return dir;
        }
    }
}

/**
 * 析构函数，关闭内存映射的文件
 */
ComDocIO::~ComDocIO() {
    if (file.is_open()) {
        file.close();
    }
}

/**
 * 配置复合文档头部
 */
void ComDocIO::ConfigureHeader() {
    if (kFilePtr_ == nullptr) {
        throw std::logic_error("kFilePtr_ is nullptr");
    }
    unsigned short offset = 0;
    memcpy(header_.file_id, kFilePtr_, sizeof(header_.file_id));
    offset += sizeof(header_.file_id);
    ptr_ = const_cast<char *>(kFilePtr_) + offset;
    memcpy(header_.unique_id, ptr_, sizeof(header_.unique_id));
    offset += sizeof(header_.unique_id);
    header_.revision_number = GET_UINT16(kFilePtr_, offset);
    offset += sizeof(uint16_t);
    header_.version_number = GET_UINT16(kFilePtr_, offset);
    offset += sizeof(uint16_t);
    header_.byte_order = GET_UINT16(kFilePtr_, offset);
    offset += sizeof(uint16_t);
    if (header_.byte_order != 0xFFFE) {
        throw std::logic_error("The current file is not in proper byte order.");
    }
    header_.sector_size = MAKE_UI32(1) << GET_UINT16(kFilePtr_, offset);
    offset += sizeof(uint16_t);
    if (header_.sector_size != 512) {
        throw std::logic_error("The sector size is not 512.");
    }
    header_.short_sector_size = MAKE_UI32(1) << GET_UINT16(kFilePtr_, offset);
    offset += sizeof(uint16_t);
    offset += 10;
    header_.sector_number = GET_UINT32(kFilePtr_, offset);
    offset += sizeof(uint32_t);
    header_.directory_first_sid = GET_INT32(kFilePtr_, offset);
    offset += sizeof(int32_t);
    offset += 4;
    header_.standard_minimum_size = GET_UINT32(kFilePtr_, offset);
    offset += sizeof(uint32_t);
    header_.SSAT_first_sid = GET_INT32(kFilePtr_, offset);
    offset += sizeof(int32_t);
    header_.SSAT_sector_size = GET_UINT32(kFilePtr_, offset);
    offset += sizeof(uint32_t);
    header_.MSAT_first_sid = GET_INT32(kFilePtr_, offset);
    offset += sizeof(int32_t);
    header_.MSAT_sector_size = GET_UINT32(kFilePtr_, offset);
    offset += sizeof(uint32_t);
    ptr_ = const_cast<char *>(kFilePtr_) + offset;
    memcpy(header_.MSAT_first_part, ptr_, sizeof(header_.MSAT_first_part));
    sid_count_per_sector_ = header_.sector_size / 4 - 1;

}

/**
 * 传入文件路径，对文件做内存映射
 * @param file_path
 */
void ComDocIO::ReadFile(std::string file_path) {
    boost::iostreams::mapped_file_params params;
    params.path = std::move(file_path);
    params.flags = boost::iostreams::mapped_file::mapmode::readonly;
    this->file.open(params);
    kFilePtr_ = this->file.data();
}

/**
 * 根据目录id，找到目录
 * @param DID
 * @return
 */
Directory ComDocIO::ReadDirectory(uint32_t DID) {
    Directory directory;
    uint32_t storage_sector_order = DID * 128 / header_.sector_size;
    uint32_t offset = DID * 128 % header_.sector_size;
    int32_t current_sid = 0;
    if (header_.directory_first_sid < 0) {
        throw std::logic_error("directory_first_sid less than error");
    }
    current_sid = FindSID(header_.directory_first_sid, storage_sector_order);

    ptr_ = const_cast<char *>(kFilePtr_) + (current_sid + 1) * header_.sector_size + offset;
    memcpy(directory.entry_name, ptr_, sizeof(directory.entry_name));
    offset = sizeof(directory.entry_name) / sizeof(*directory.entry_name);
    directory.name_size = GET_UINT16(ptr_, offset);
    offset += sizeof(directory.name_size);
    directory.entry_type = GET_BYTE(ptr_, offset);
    offset += sizeof(directory.entry_type);
    directory.node_color = GET_BYTE(ptr_, offset);
    offset += sizeof(directory.node_color);
    directory.left_DID = GET_INT32(ptr_, offset);
    offset += sizeof(directory.left_DID);
    directory.right_DID = GET_INT32(ptr_, offset);
    offset += sizeof(directory.right_DID);
    directory.root_DID = GET_INT32(ptr_, offset);
    offset += sizeof(directory.root_DID);
    memcpy(directory.unique_ID, ptr_, sizeof(directory.unique_ID));
    offset += sizeof(directory.unique_ID) / sizeof(*directory.unique_ID);
    directory.user_tag = GET_INT32(ptr_, offset);
    offset += sizeof(directory.user_tag);
    directory.entry_create_timestamp = GET_INT64(ptr_, offset);
    offset += sizeof(directory.entry_create_timestamp);
    directory.entry_last_modify_timestamp = GET_INT64(ptr_, offset);
    offset += sizeof(directory.entry_last_modify_timestamp);
    directory.entry_first_sid = GET_INT32(ptr_, offset);
    offset += sizeof(directory.entry_first_sid);
    directory.stream_size = GET_UINT32(ptr_, offset);
    directory.DID = DID;
    return directory;
}

/**
 * 根据扇区id，找到下一个扇区id
 * @param sid
 * @return
 */
int32_t ComDocIO::FindNextSID(int32_t sid) {
    if (sid < 0) {
        return sid;
    }
    int32_t next_sid;
    int32_t MSAT_location = sid / (header_.sector_size / 4);
    int32_t SAT_offset = sid % (header_.sector_size / 4);
    if (MSAT_location < 109) {
        ptr_ = const_cast<char *>(kFilePtr_) + header_.MSAT_first_part[MSAT_location] * header_.sector_size +
               header_.sector_size;
        next_sid = GET_INT32(ptr_, SAT_offset * 4);
    } else {
        int32_t MSAT_order = (MSAT_location - 109) / sid_count_per_sector_;
        int32_t MSAT_offset = (MSAT_location - 109) % sid_count_per_sector_;
        ptr_ = const_cast<char *>(kFilePtr_) + MSAT_first_sid_array_.at(MSAT_order) * header_.sector_size +
               header_.sector_size;
        int32_t entry_sid = GET_INT32(ptr_, MSAT_offset * 4);
        ptr_ = const_cast<char *>(kFilePtr_) + entry_sid * header_.sector_size + header_.sector_size;
        next_sid = GET_INT32(ptr_, SAT_offset * 4);
    }
    return next_sid;
}

/**
 * 根据扇区id，偏移量，找到目标扇区id
 * @param sid
 * @param offset
 * @return
 */
int32_t ComDocIO::FindSID(int32_t sid, uint32_t offset = 1) {
    if (sid < 0) {
        return sid;
    }
    int32_t current_sid = sid;
    for (uint32_t index = 0; index < offset; index++)
        current_sid = FindNextSID(current_sid);
    return current_sid;
}

/**
 * 配置短流扇区配置表的sid链
 */
void ComDocIO::ConfigureSSATFirstSID() //
{
    if (kFilePtr_ == nullptr) {
        throw std::logic_error("kFilePtr_ is nullptr");
    }
    int32_t current_sid = header_.SSAT_first_sid;
    while (current_sid > 0) {
        SSAT_first_sid_array_.push_back(current_sid);
        current_sid = FindNextSID(current_sid);
    }
}

/**
 * 短流存放的扇区链由入口目录给出，找到短流存放流后，就可以根据短流配置表对短流存放流进行检索
 */
void ComDocIO::ConfigureShortStreamStorage() {
    Directory root_directory = ReadDirectory(0);
    if (root_directory.entry_type != 0x5) {
        throw std::logic_error("Entry type of root directory is not 05H");
    }
    int32_t current_sid = root_directory.entry_first_sid;
    while (current_sid > 0) {
        short_stream_sid_array_.push_back(current_sid);
        current_sid = FindNextSID(current_sid);
    }
}

/**
 * 传入短流扇区的id，得到下一个短流扇区的id
 * @param ssid
 * @return
 */
int32_t ComDocIO::FindNextShortSID(int32_t ssid) {
    if (SSAT_first_sid_array_.empty()) {
        throw std::logic_error("SSAT_first_sid_array_ is empty");
    }
    int32_t nex_sid;
    int32_t serial_number = ssid / (header_.sector_size / 4);
    int32_t off = ssid % (header_.sector_size / 4);
    ptr_ = const_cast<char *>(kFilePtr_) + SSAT_first_sid_array_.at(serial_number) * header_.sector_size +
           header_.sector_size;
    nex_sid = GET_INT32(ptr_, off * 4);
    return nex_sid;
}

/**
 * 传入ssid，得到在文件中的偏移位置
 * @param ssid
 * @return
 */
char *ComDocIO::GetAddressFromShortSID(int32_t ssid) {
    if (short_stream_sid_array_.empty()) {
        throw std::logic_error("Size of short_stream_sid_array_ less than zero");
    }
    int32_t serial_number = ssid * header_.short_sector_size / header_.sector_size;
    int32_t offset = ssid * header_.short_sector_size % header_.sector_size;
    ptr_ = const_cast<char *>(kFilePtr_) + short_stream_sid_array_[serial_number] * header_.sector_size +
           header_.sector_size +
           offset;
    return ptr_;
}

/**
 * 根据sid和长度，把流拷贝出来，得到拷贝后的指针
 * @param sid
 * @param len
 * @return
 */
char *ComDocIO::ReadStreamFromSID(int32_t sid, uint64_t len) {
    char *p = (char *) malloc(len);
    char *current_pointer = p;
    uint64_t serial_number = len / header_.sector_size;
    uint64_t offset = len % header_.sector_size;
    int32_t current_sid = sid;
    while (current_sid != -2) {
        ptr_ = const_cast<char *>(kFilePtr_) + current_sid * header_.sector_size + header_.sector_size;
        if (serial_number == 0) {
            memcpy(current_pointer, ptr_, offset);
            return p;
        } else {
            memcpy(current_pointer, ptr_, header_.sector_size);
            current_pointer += header_.sector_size;
            serial_number--;
            current_sid = FindNextSID(current_sid);
        }
    }
    return p;
}

/**
 * 根据ssid和长度，把短流拷贝出来，得到拷贝后的指针
 * @param ssid
 * @param len
 * @return
 */
char *ComDocIO::ReadShortStreamFromSID(int32_t ssid, uint32_t len) {
    char *p = (char *) malloc(len);
    char *current_pointer = p;
    int32_t serial_number = len / header_.short_sector_size;
    int32_t offset = len % header_.short_sector_size;
    int32_t current_ssid = ssid;

    while (current_ssid != -2) {
        ptr_ = GetAddressFromShortSID(current_ssid);
        if (serial_number == 0) {
            memcpy(current_pointer, ptr_, offset);
            return p;
        } else {
            memcpy(current_pointer, ptr_, header_.short_sector_size);
            current_pointer += header_.short_sector_size;
            serial_number--;
            current_ssid = FindNextShortSID(current_ssid);
        }
    }
    return p;
}

/**
 * 主扇区配置表，一部分存在于头部，另一部分存在于扇区中。用于配置存在于扇区中的第一个sid，这些sid构成一个数组
 */
void ComDocIO::ConfigureMSATFirstSID() {
    if (header_.MSAT_first_sid > 0) {
        MSAT_first_sid_array_.push_back(header_.MSAT_first_sid);
        SearchMSATFirstSID(header_.MSAT_first_sid);
    }
}

/**
 * 每个扇区的最后4字节指向下一个扇区的sid，形成扇区链。根据头部主扇区配置表的第一个sid，就可以搜索到完整的扇区链
 * @param sid
 */
void ComDocIO::SearchMSATFirstSID(int32_t sid) {
    ptr_ = const_cast<char *>(kFilePtr_) + sid * header_.sector_size + header_.sector_size;
    int32_t current_sid = GET_INT32(ptr_, header_.sector_size - 4);
    if (current_sid > 0) {
        MSAT_first_sid_array_.push_back(current_sid);
        SearchMSATFirstSID(current_sid);
    }
}

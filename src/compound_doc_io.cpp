#include "compound_doc_io.h"
#include <boost/algorithm/string.hpp>

using namespace compound_doc_io;

ComDocIO::ComDocIO() {
}

ComDocIO::ComDocIO(std::string file_path) {
    ReadFile(std::move(file_path));
    ConfigureHeader();
    ConfigureMSATFirstSID();
    ConfigureSSATFirstSID();
    ConfigureShortStreamStorage();
}

FileBlock *ComDocIO::ReadFromPath(std::string path) {
    std::vector<std::string> sub_path_array;
    std::string sub_path = path.substr(1);
    boost::split(sub_path_array, sub_path, boost::is_any_of("/"));
    Directory last_path_directory = ReadDirectory(0);
    for (std::string &child_path : sub_path_array) {
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

Directory ComDocIO::FindDirectoryFromName(uint32_t root_DID, std::string name) {
    Directory directory = ReadDirectory(root_DID);
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
            Directory dir = FindDirectoryFromName((uint32_t) directory.left_DID, name);
            return dir;
        }
    } else {
        if (directory.right_DID < 0) {
            throw std::logic_error("Can not find specified directory");
        } else {
            Directory dir = FindDirectoryFromName((uint32_t) directory.right_DID, name);
            return dir;
        }
    }
}

ComDocIO::~ComDocIO() {
    if (file.is_open()) {
        file.close();
    }
}

void ComDocIO::ConfigureHeader() {
    if (dat != nullptr) {
        ushort offset = 0;
        memcpy(header_.file_id, dat, sizeof(header_.file_id));
        offset += sizeof(header_.file_id);
        ptr = const_cast<char *>(dat) + offset;
        memcpy(header_.unique_id, ptr, sizeof(header_.unique_id));
        offset += sizeof(header_.unique_id);
        header_.revision_number = GET_UINT16(dat, offset);
        offset += sizeof(uint16_t);
        header_.version_number = GET_UINT16(dat, offset);
        offset += sizeof(uint16_t);
        header_.byte_order = GET_UINT16(dat, offset);
        offset += sizeof(uint16_t);
        if (header_.byte_order != 0xFFFE) {
            throw std::logic_error("The current file is not in proper byte order.");
        }
        header_.sector_size = MAKE_UI32(1) << GET_UINT16(dat, offset);
        offset += sizeof(uint16_t);
        if (header_.sector_size != 512) {
            throw std::logic_error("The sector size is not 512.");
        }
        header_.short_sector_size = MAKE_UI32(1) << GET_UINT16(dat, offset);
        offset += sizeof(uint16_t);
        offset += 10;
        header_.sector_number = GET_UINT32(dat, offset);
        offset += sizeof(uint32_t);
        header_.directory_first_sid = GET_INT32(dat, offset);
        offset += sizeof(int32_t);
        offset += 4;
        header_.standard_minimum_size = GET_UINT32(dat, offset);
        offset += sizeof(uint32_t);
        header_.SSAT_first_sid = GET_INT32(dat, offset);
        offset += sizeof(int32_t);
        header_.SSAT_sector_size = GET_UINT32(dat, offset);
        offset += sizeof(uint32_t);
        header_.MSAT_first_sid = GET_INT32(dat, offset);
        offset += sizeof(int32_t);
        header_.MSAT_sector_size = GET_UINT32(dat, offset);
        offset += sizeof(uint32_t);
        ptr = const_cast<char *>(dat) + offset;
        memcpy(header_.MSAT_first_part, ptr, sizeof(header_.MSAT_first_part));
        sid_count_per_sector_ = header_.sector_size / 4 - 1;
    } else {
        throw std::logic_error("dat is nullptr");
    }
}

void ComDocIO::ReadFile(std::string file_path) {
    boost::iostreams::mapped_file_params params;
    params.path = std::move(file_path);
    params.flags = boost::iostreams::mapped_file::mapmode::readonly;
    this->file.open(params);
    dat = this->file.data();
}

Directory ComDocIO::ReadDirectory(uint32_t DID) {
    Directory directory;
    uint32_t storage_sector_order = DID * 128 / header_.sector_size;
    uint32_t offset = DID * 128 % header_.sector_size;
    int32_t current_sid = 0;
    if (header_.directory_first_sid < 0) {
        throw std::logic_error("directory_first_sid less than error");
    }
    current_sid = FindSID(header_.directory_first_sid, storage_sector_order);

    ptr = const_cast<char *>(dat) + (current_sid + 1) * header_.sector_size + offset;
    memcpy(directory.entry_name, ptr, sizeof(directory.entry_name));
    offset = sizeof(directory.entry_name) / sizeof(*directory.entry_name);
    directory.name_size = GET_UINT16(ptr, offset);
    offset += sizeof(directory.name_size);
    directory.entry_type = GET_BYTE(ptr, offset);
    offset += sizeof(directory.entry_type);
    directory.node_color = GET_BYTE(ptr, offset);
    offset += sizeof(directory.node_color);
    directory.left_DID = GET_INT32(ptr, offset);
    offset += sizeof(directory.left_DID);
    directory.right_DID = GET_INT32(ptr, offset);
    offset += sizeof(directory.right_DID);
    directory.root_DID = GET_INT32(ptr, offset);
    offset += sizeof(directory.root_DID);
    memcpy(directory.unique_ID, ptr, sizeof(directory.unique_ID));
    offset += sizeof(directory.unique_ID) / sizeof(*directory.unique_ID);
    directory.user_tag = GET_INT32(ptr, offset);
    offset += sizeof(directory.user_tag);
    directory.entry_create_timestamp = GET_INT64(ptr, offset);
    offset += sizeof(directory.entry_create_timestamp);
    directory.entry_last_modify_timestamp = GET_INT64(ptr, offset);
    offset += sizeof(directory.entry_last_modify_timestamp);
    directory.entry_first_sid = GET_INT32(ptr, offset);
    offset += sizeof(directory.entry_first_sid);
    directory.stream_size = GET_UINT32(ptr, offset);
    directory.DID = DID;
    return directory;
}

int32_t ComDocIO::FindNextSID(int32_t sid) {
    if (sid < 0) {
        return sid;
    }
    int32_t next_sid;
    int32_t MSAT_location = sid / (header_.sector_size / 4);
    int32_t SAT_offset = sid % (header_.sector_size / 4);
    if (MSAT_location < 109) {
        ptr = const_cast<char *>(dat) + header_.MSAT_first_part[MSAT_location] * header_.sector_size +
              header_.sector_size;
        next_sid = GET_INT32(ptr, SAT_offset * 4);
    } else {
        int32_t MSAT_order = (MSAT_location - 109) / sid_count_per_sector_;
        int32_t MSAT_offset = (MSAT_location - 109) % sid_count_per_sector_;
        ptr = const_cast<char *>(dat) + MSAT_fisrt_sid_array_.at(MSAT_order) * header_.sector_size +
              header_.sector_size;
        int32_t entry_sid = GET_INT32(ptr, MSAT_offset * 4);
        ptr = const_cast<char *>(dat) + entry_sid * header_.sector_size + header_.sector_size;
        next_sid = GET_INT32(ptr, SAT_offset * 4);
    }
    return next_sid;
}

int32_t ComDocIO::FindSID(int32_t sid, uint32_t offset = 1) {
    if (sid < 0) {
        return sid;
    }
    int32_t current_sid = sid;
    for (uint32_t index = 0; index < offset; index++)
        current_sid = FindNextSID(current_sid);
    return current_sid;
}

void ComDocIO::ConfigureSSATFirstSID() //
{
    if (dat == nullptr) {
        throw std::logic_error("dat is nullptr");
    }
    int32_t current_sid = header_.SSAT_first_sid;
    while (current_sid > 0) {
        SSAT_first_sid_array_.push_back(current_sid);
        current_sid = FindNextSID(current_sid);
    }
}

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

int32_t ComDocIO::FindNextShortSID(int32_t ssid) {
    if (SSAT_first_sid_array_.empty()) {
        throw std::logic_error("SSAT_first_sid_array_ is empty");
    }
    int32_t NextSID;
    int32_t serial_number = ssid / (header_.sector_size / 4);
    int32_t off = ssid % (header_.sector_size / 4);
    ptr = const_cast<char *>(dat) + SSAT_first_sid_array_.at(serial_number) * header_.sector_size + header_.sector_size;
    NextSID = GET_INT32(ptr, off * 4);
    return NextSID;
}

char *ComDocIO::GetAddressFromShortSID(int32_t ssid) {
    if (short_stream_sid_array_.empty()) {
        throw std::logic_error("Size of short_stream_sid_array_ less than zero");
    }
    int32_t serial_number = ssid * header_.short_sector_size / header_.sector_size;
    int32_t offset = ssid * header_.short_sector_size % header_.sector_size;
    ptr = const_cast<char *>(dat) + short_stream_sid_array_[serial_number] * header_.sector_size + header_.sector_size +
          offset;
    return ptr;
}

char *ComDocIO::ReadStreamFromSID(int32_t sid, uint64_t len) {
    char *p = (char *) malloc(len);
    char *current_pointer = p;
    uint64_t serial_number = len / header_.sector_size;
    uint64_t offset = len % header_.sector_size;
    int32_t current_sid = sid;
    while (current_sid != -2) {
        ptr = const_cast<char *>(dat) + current_sid * header_.sector_size + header_.sector_size;
        if (serial_number == 0) {
            memcpy(current_pointer, ptr, offset);
            return p;
        } else {
            memcpy(current_pointer, ptr, header_.sector_size);
            current_pointer += header_.sector_size;
            serial_number--;
            current_sid = FindNextSID(current_sid);
        }
    }
    return p;
}

char *ComDocIO::ReadShortStreamFromSID(int32_t ssid, uint32_t len) {
    char *p = (char *) malloc(len);
    char *current_pointer = p;
    int32_t serial_number = len / header_.short_sector_size;
    int32_t offset = len % header_.short_sector_size;
    int32_t current_ssid = ssid;

    while (current_ssid != -2) {
        ptr = GetAddressFromShortSID(current_ssid);
        if (serial_number == 0) {
            memcpy(current_pointer, ptr, offset);
            return p;
        } else {
            memcpy(current_pointer, ptr, header_.short_sector_size);
            current_pointer += header_.short_sector_size;
            serial_number--;
            current_ssid = FindNextShortSID(current_ssid);
        }
    }
    return p;
}

void ComDocIO::ConfigureMSATFirstSID() {
    if (header_.MSAT_first_sid > 0) {
        MSAT_fisrt_sid_array_.push_back(header_.MSAT_first_sid);
        SearchMSATFirstSID(header_.MSAT_first_sid);
    }
}

void ComDocIO::SearchMSATFirstSID(int32_t sid) {
    ptr = const_cast<char *>(dat) + sid * header_.sector_size + header_.sector_size;
    int32_t current_sid = GET_INT32(ptr, header_.sector_size - 4);
    if (current_sid > 0) {
        MSAT_fisrt_sid_array_.push_back(current_sid);
        SearchMSATFirstSID(current_sid);
    }
}

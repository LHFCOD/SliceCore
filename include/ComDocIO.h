#pragma once
#include "BaseDataStruct.h"
using namespace BaseDataStruct;
#include <iostream>
#include <stdexcept>
#include <map>
/*È¡Êý¾Ý */
#define get_data(ptr, off)  (((const byte_t *)ptr) + off)
#define get_byte(ptr, off)  (*(const byte_t *)get_data(ptr, off))
#define get_ui16(ptr, off)  (*(const ui16_t *)get_data(ptr, off))
#define get_ui32(ptr, off)  (*(const ui32_t *)get_data(ptr, off))
#define get_ui64(ptr, off)  (*(const ui64_t *)get_data(ptr, off))

#define get_i32(ptr, off)  (*(const i32_t *)get_data(ptr, off))

#define MAKE_UI32(n) n##u
#define MAKE_UI64(n) n##ull

#include <boost/iostreams/device/mapped_file.hpp>
using namespace std;
using namespace boost::iostreams;
namespace ComDoc
{
	struct Header
	{
		byte_t FileID[8];
		byte_t UID[16];
		ui16_t ReviNum;
		ui16_t VerNum;
		ui16_t ByteOrder;
		ui64_t SectorSize;
		ui16_t ShortSectSize;
		ui32_t SectNum;
		i32_t DirectSID;
		ui64_t MiniSize;
		i32_t ShortSectSID;
		ui32_t ShortSectNum;
		i32_t MSectSID;
		ui32_t MSectNum;
		i32_t MSectPart[109];
	};
	struct Directory
	{
		byte_t EntryName[64] = { 0 };
		ui16_t NameSize=0;
		byte_t EntryType;
		byte_t NodeColor;
		i32_t LeftDID;
		i32_t RightDID;
		i32_t RootDID;
		i32_t SID;
		ui32_t StreamSize;
		i32_t DID;
	};
	class ComDocIO
	{
	public:
		ComDocIO();
		ComDocIO(string FilePath);
		~ComDocIO();
	public:
		Header m_Header;
		const char* dat;
        mapped_file_source file;
		char* ptr;

		vector<i32_t> ShortSreamCon;
		vector<i32_t> ShortSectAllo;
		vector<i32_t> SATAllo;
		void ReadHeader();
		void ReadFile(string FilePath);
		Directory ReadDirectory(ui32_t DID);
		inline i32_t FindNextSID(i32_t SID);
		inline i32_t FindSID(i32_t SID, ui32_t Offset);
		void ConfigureShorStream();
		inline i32_t FindNextShortSID(i32_t SID);
		inline char* GetAddressFromShortSID(i32_t SID);
		inline byte_t* ReadStreamFromSID(i32_t SID, ui64_t len);
		inline byte_t*  ReadShortStreamFromSID(i32_t SID, ui64_t len);///

		map<ui32_t, vector<Directory>*> FilePool;//´æ·ÅÎÄ¼þ×éÖ¯½á¹¹
		i32_t FileCount = 0;//´æ·ÅÎÄ¼þÊýÁ¿
		void CreateFilePool(ui32_t RootDID, vector<Directory> *vec);
		void SerchTree(ui32_t RootDID, vector<Directory> *vec);//²éÕÒºìºÚÊ÷
		void ConfigureSAT();
		void SerchSAT(i32_t SID);
		FileBlock* ReadFromPath(string path);
		Directory FindDirectoryFromWName(i32_t RootDID, std::wstring name);
		Directory FindDirectoryFromName(i32_t RootDID, std::string name);
		inline int CompareWstring(std::wstring str1, std::wstring str2) {
			if (str1.size() < str2.size())
				return -1;
			else if (str1.size() > str2.size())
				return 1;
			else if (str1.size() == str2.size())
			{
				std::transform(str1.begin(), str1.end(), str1.begin(), static_cast<int(*)(int)>(std::toupper));
				std::transform(str2.begin(), str2.end(), str2.begin(), static_cast<int(*)(int)>(std::toupper));
				if (str1 < str2)
					return -1;
				else if (str1 > str2)
					return 1;
				else
					return 0;
			}
		};
		inline int Comparestring(std::string str1, std::string str2) {
			if (str1.size() < str2.size())
				return -1;
			else if (str1.size() > str2.size())
				return 1;
			else if (str1.size() == str2.size())
			{
				std::transform(str1.begin(), str1.end(), str1.begin(), static_cast<int(*)(int)>(std::toupper));
				std::transform(str2.begin(), str2.end(), str2.begin(), static_cast<int(*)(int)>(std::toupper));
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

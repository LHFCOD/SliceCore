#include "../include/ComDocIO.h"
#include "../include/Utils.h"

using namespace ComDoc;

ComDocIO::ComDocIO()
{
}

ComDocIO::ComDocIO(string FilePath)
{
    ReadFile(FilePath);
    ReadHeader();
    ConfigureSAT();
    ConfigureShorStream();
}

FileBlock *ComDocIO::ReadFromPath(string path)
{
    vector<string> dirList;
    Utils::splitString(path, dirList, "/");
    for (int i = 0; i < dirList.size(); i++)
    {
        if (i == 0)
        {
            if (!dirList[i].empty())
            {
                std::logic_error excep("path has error!");
                throw std::exception(excep);
            }
        }
        else
        {
            if (dirList[i].empty())
            {
                std::logic_error excep("path has error!");
                throw std::exception(excep);
            }
        }
    }
    Directory lastRoot = ReadDirectory(0);

    for (int i = 1; i < dirList.size(); i++)
    {
#ifdef _WIN32
        lastRoot = FindDirectoryFromWName(lastRoot.RootDID, dirList[i].toStdWString());
#else
        lastRoot = FindDirectoryFromName(lastRoot.RootDID, dirList[i]);
#endif
        if (lastRoot.NameSize == 0)
        {
            std::logic_error excep("can't find the specified directory!");
            throw std::exception(excep);
        }
    }
    byte_t *p;
    if (lastRoot.StreamSize < m_Header.MiniSize)
        p = ReadShortStreamFromSID(lastRoot.SID, lastRoot.StreamSize);
    else
        p = ReadStreamFromSID(lastRoot.SID, lastRoot.StreamSize); //Ò»´ÎÐÔ°ÑËùÐèÒªÐ´µÄ×Ö·ûÁ÷¶ÁÈ¡£¬È»ºóÐ´ÈëÎÄ¼þ
    FileBlock *block = new FileBlock(p, lastRoot.StreamSize);
    return block;
}

Directory ComDoc::ComDocIO::FindDirectoryFromWName(i32_t RootDID, std::wstring name)
{
    Directory direct = ReadDirectory(RootDID);

    std::wstring entry((wchar_t *)direct.EntryName);
    int comp = CompareWstring(name, entry);
    if (comp == 0)
        return direct;
    else if (comp == -1)
    {
        if (direct.LeftDID < 0)
            return Directory();
        else
        {
            Directory dir = FindDirectoryFromWName(direct.LeftDID, name);
            return dir;
        }
    }
    else
    {
        if (direct.RightDID < 0)
            return Directory();
        else
        {
            Directory dir = FindDirectoryFromWName(direct.RightDID, name);
            return dir;
        }
    }
}

Directory ComDoc::ComDocIO::FindDirectoryFromName(i32_t RootDID, std::string name)
{
    Directory direct = ReadDirectory(RootDID);

    std::string entry;
    for (int i = 0; i < direct.NameSize; i++)
    {
        if (i % 2 == 0 && direct.EntryName[i] != '\0')
        {
            entry.insert(entry.end(), direct.EntryName[i]);
        }
    }
    int comp = Comparestring(name, entry);
    if (comp == 0)
        return direct;
    else if (comp == -1)
    {
        if (direct.LeftDID < 0)
            return Directory();
        else
        {
            Directory dir = FindDirectoryFromName(direct.LeftDID, name);
            return dir;
        }
    }
    else
    {
        if (direct.RightDID < 0)
            return Directory();
        else
        {
            Directory dir = FindDirectoryFromName(direct.RightDID, name);
            return dir;
        }
    }
}

ComDocIO::~ComDocIO()
{
    if (file.is_open())
    {
        file.close();
    }
    map<ui32_t, vector<Directory> *>::iterator iter=FilePool.begin();
    while (iter != FilePool.end())
    {
        if(iter->second)
        delete iter->second;
    }
}

void ComDocIO::ReadHeader()
{
    if (dat)
    {
        memcpy(m_Header.FileID, dat, sizeof(m_Header.FileID));
        ptr = const_cast<char *>(dat) + 8; //Æ«ÒÆ8×Ö½Ú
        memcpy(m_Header.UID, dat, sizeof(m_Header.UID));
        m_Header.ReviNum = get_ui16(dat, 24);
        m_Header.VerNum = get_ui16(dat, 26);
        m_Header.ByteOrder = get_ui16(dat, 28);
        if (m_Header.ByteOrder != 0xFFFE)
        {
            //Èç¹û²»ÊÇµÍ¶Ë×Ö½ÚÐò
            return;
        }
        m_Header.SectorSize = MAKE_UI64(1) << get_ui16(dat, 30);
        if (m_Header.SectorSize != 512)
        {
            //Èç¹ûÉÈÇø´óÐ¡Òì³£
        }
        m_Header.ShortSectSize = MAKE_UI64(1) << get_ui16(dat, 32);
        if (m_Header.ShortSectSize == 0)
        {
            //Èç¹û¶ÌÉÈÇø¹ý´ó
        }
        m_Header.SectNum = get_ui32(dat, 44);
        m_Header.DirectSID = get_i32(dat, 48);
        m_Header.MiniSize = get_ui32(dat, 56);
        m_Header.ShortSectSID = get_i32(dat, 60);
        m_Header.ShortSectNum = get_ui32(dat, 64);
        m_Header.MSectSID = get_i32(dat, 68);
        if (m_Header.MSectSID != -2)
        {
            //Èç¹ûMSATÓÐ¸½¼Ó
        }
        m_Header.MSectNum = get_ui32(dat, 72);
        //Æ«ÒÆ76×Ö½Ú
        ptr = const_cast<char *>(dat) + 76;
        memcpy(m_Header.MSectPart, ptr, sizeof(m_Header.MSectPart));
    }
}

void ComDocIO::ReadFile(string FilePath)
{
    mapped_file_params params;
    params.path = FilePath;
    params.flags = mapped_file::mapmode::readonly;
    this->file.open(params);
    dat = this->file.data();
}

Directory ComDocIO::ReadDirectory(ui32_t DID)
{
    Directory direct;
    i32_t SeriNum = DID * 128 / m_Header.SectorSize;
    i32_t off = DID * 128 % m_Header.SectorSize;
    //i32_t NowSID = FindSID(m_Header.DirectSID, SeriNum);
    i32_t NowSID = 0;
    ///////
    if (m_Header.DirectSID < 0)
    {
        //SIDÒÑ½áÊø
        //NowSID= m_Header.DirectSID;
        //»ñÈ¡µÄSIDÓÐÎÊÌâ
        return Directory();
    }
    else
    {
        NowSID = m_Header.DirectSID;
        for (ui32_t index = 0; index < SeriNum; index++)
            NowSID = FindNextSID(NowSID);
    }
    //if (NowSID < 0)
    //{
    //	//»ñÈ¡µÄSIDÓÐÎÊÌâ
    //	return Directory();
    //}

    ptr = const_cast<char *>(dat) + (NowSID + 1) * m_Header.SectorSize + off; //Æ«ÒÆµ½Ä¿Â¼Èë¿Ú
    memcpy(direct.EntryName, ptr, sizeof(direct.EntryName));
    direct.NameSize = get_ui16(ptr, 64);
    direct.EntryType = get_byte(ptr, 66);
    direct.NodeColor = get_byte(ptr, 67);
    direct.LeftDID = get_i32(ptr, 68);
    direct.RightDID = get_i32(ptr, 72);
    direct.RootDID = get_i32(ptr, 76);
    direct.SID = get_i32(ptr, 116);
    direct.StreamSize = get_ui32(ptr, 120);
    direct.DID = DID;
    return direct;
}

i32_t ComDocIO::FindNextSID(i32_t SID)
{
    if (SID < 0)
    {
        //SIDÒÑ½áÊø
        return SID;
    }

    i32_t NextSID;
    i32_t seriNum = SID / (m_Header.SectorSize / 4);
    i32_t off = SID % (m_Header.SectorSize / 4);
    if (seriNum < 109)
    {
        ptr = const_cast<char *>(dat) + m_Header.MSectPart[seriNum] * m_Header.SectorSize + m_Header.SectorSize;
        NextSID = get_i32(ptr, off * 4);
    }
    else
    {
        int SIDCount = m_Header.SectorSize / 4 - 1;
        i32_t seri = (seriNum - 109) / SIDCount;
        i32_t offset = (seriNum - 109) % SIDCount;
        ptr = const_cast<char *>(dat) + SATAllo.at(seri) * m_Header.SectorSize + m_Header.SectorSize;
        i32_t EntrySID = get_i32(ptr, offset * 4);
        ptr = const_cast<char *>(dat) + EntrySID * m_Header.SectorSize + m_Header.SectorSize;
        NextSID = get_i32(ptr, off * 4);
    }
    return NextSID;
}

i32_t ComDocIO::FindSID(i32_t SID, ui32_t Offset = 1) //Æ«ÒÆÁ¿ÏÂµÄÉÈÇøSID²éÕÒ
{
    if (SID < 0)
    {
        //SIDÒÑ½áÊø
        return SID;
    }
    i32_t NowSID = SID;
    for (ui32_t index = 0; index < Offset; index++)
        NowSID = FindNextSID(NowSID);
    return NowSID;
}

void ComDocIO::ConfigureShorStream() //ÅäÖÃ¶ÌÁ÷´æ·ÅÁ÷±íºÍÏàÓ¦µÄ´æ·ÅÉÈÇø
{
    if (dat == 0)
        return;
    i32_t NowSID = m_Header.ShortSectSID;
    if (NowSID < 0)
        return;
    do
    {
        ShortSectAllo.push_back(NowSID);
        NowSID = FindNextSID(NowSID);
        if (ShortSectAllo.size() > 10000000) //±ÜÃâ³öÏÖÉÈÇøÁ´²»ÖÐ¶ÏµÄÇé¿ö
            return;
    } while (NowSID != -2);

    Directory RootDirect = ReadDirectory(0);
    if (RootDirect.EntryType != 5)
    {
        //DIDÎª0µÄ²»ÎªRootEntry£¬ÔòÍË³ö
        return;
    }
    NowSID = RootDirect.SID;
    do
    {
        ShortSreamCon.push_back(NowSID);
        NowSID = FindNextSID(NowSID);
        if (ShortSectAllo.size() > 10000000) //±ÜÃâ³öÏÖÉÈÇøÁ´²»ÖÐ¶ÏµÄÇé¿ö
            return;
    } while (NowSID != -2);
}

i32_t ComDocIO::FindNextShortSID(i32_t SID)
{
    if (ShortSectAllo.size() == 0)
    {
        //Èç¹ûÅäÖÃ±íÎª¿ÕÔò·µ»Ø
        return 0;
    }
    i32_t NextSID;
    i32_t SeriNum = SID / (m_Header.SectorSize / 4);
    i32_t off = SID % (m_Header.SectorSize / 4);
    ptr = const_cast<char *>(dat) + ShortSectAllo.at(SeriNum) * m_Header.SectorSize + m_Header.SectorSize;
    NextSID = get_i32(ptr, off * 4);
    return NextSID;
}

char *ComDocIO::GetAddressFromShortSID(i32_t SID)
{
    if (ShortSreamCon.size() == 0)
    {
        //Èç¹û¶ÌÁ÷´æ·ÅÇøÎª¿Õ
        return NULL;
    }
    i32_t SeriNum = SID * m_Header.ShortSectSize / m_Header.SectorSize;
    i32_t off = SID * m_Header.ShortSectSize % m_Header.SectorSize;
    ptr = const_cast<char *>(dat) + ShortSreamCon[SeriNum] * m_Header.SectorSize + m_Header.SectorSize + off;
    return ptr;
}

byte_t *ComDocIO::ReadStreamFromSID(i32_t SID, ui64_t len)
{
    byte_t *p = (byte_t *)malloc(len);
    byte_t *Nowp = p;
    i32_t SeriNum = len / m_Header.SectorSize;
    i32_t off = len % m_Header.SectorSize;
    i32_t NowSID = SID;
    do
    {
        ptr = const_cast<char *>(dat) + NowSID * m_Header.SectorSize + m_Header.SectorSize;
        if (SeriNum == 0)
        {
            memcpy(Nowp, ptr, off);
            return p;
        }
        else
        {
            memcpy(Nowp, ptr, m_Header.SectorSize);
            Nowp += m_Header.SectorSize;
            SeriNum--;
            NowSID = FindNextSID(NowSID);
        }
    } while (NowSID != -2);
    return p;
}

byte_t *ComDocIO::ReadShortStreamFromSID(i32_t SID, ui64_t len)
{
    byte_t *p = (byte_t *)malloc(len);
    byte_t *Nowp = p;
    i32_t SeriNum = len / m_Header.ShortSectSize;
    i32_t off = len % m_Header.ShortSectSize;
    i32_t NowSID = SID;

    do
    {
        ptr = GetAddressFromShortSID(NowSID);
        if (SeriNum == 0)
        {
            memcpy(Nowp, ptr, off);
            return p;
        }
        else
        {
            memcpy(Nowp, ptr, m_Header.ShortSectSize);
            Nowp += m_Header.ShortSectSize;
            SeriNum--;
            NowSID = FindNextShortSID(NowSID);
        }
    } while (NowSID != -2);
    return p;
}

void ComDocIO::CreateFilePool(ui32_t RootDID, vector<Directory> *vec) //´´½¨ÎÄ¼þ³Ø
{
    if (RootDID < 0)
        return;
    Directory root = ReadDirectory(RootDID);
    SerchTree(root.RootDID, vec);
    FilePool.insert(pair<ui32_t, vector<Directory>*>(RootDID, vec));
    for (auto sub : *vec)
    {
        if (sub.EntryType == 1)
        {
            vector<Directory> *subvec = new vector<Directory>();
            CreateFilePool(sub.DID, subvec);
        }
    }
}

void ComDocIO::SerchTree(ui32_t RootDID, vector<Directory> *vec)
{
    Directory direct = ReadDirectory(RootDID);
    FileCount++;
    vec->push_back(direct);
    if (direct.LeftDID > 0)
    {
        SerchTree(direct.LeftDID, vec);
    }
    if (direct.RightDID > 0)
    {
        SerchTree(direct.RightDID, vec);
    }
    return;
}

void ComDocIO::ConfigureSAT()
{
    if (m_Header.MSectSID < 0)
        return;
    SATAllo.push_back(m_Header.MSectSID);
    SerchSAT(m_Header.MSectSID);
}

void ComDocIO::SerchSAT(i32_t SID)
{
    ptr = const_cast<char *>(dat) + SID * m_Header.SectorSize + m_Header.SectorSize;
    i32_t NowSID = get_i32(ptr, m_Header.SectorSize - 4);
    if (NowSID >= 0)
    {
        SATAllo.push_back(NowSID);
        SerchSAT(NowSID);
    }
}

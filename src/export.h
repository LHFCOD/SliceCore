/**
 \* Created with CLion.
 \* User: lihongfei
 \* Date: 18-12-5
 \* Time: 下午3:07
 \* To change this template use File | Settings | File Templates.
 \* Description: 接口文件，可被java的jna调用
 \*/
#pragma once
#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#include "compound_doc_io.h"

using namespace compound_doc_io;
extern "C"
{
/**
 * 传入文件路径，返回返回复合文档对象指针
 * @param file_name
 * @return
 */
DLL_EXPORT ComDocIO *ReadComDocFile(const char *file_name);
/**
 * 释放复合文档
 * @param io
 */
DLL_EXPORT void ReleaseComDocFile(ComDocIO *io);
/**
 * 对指定的复合文档，传入虚拟路径，得到文件块
 * @param io
 * @param path
 * @return
 */
DLL_EXPORT FileBlock *ReadFromPath(ComDocIO *io, const char *path);
/**
 **根据文件块指针，得到文件长度
 * @param block
 * @return
 */
DLL_EXPORT uint32_t GetBlockLength(FileBlock *block);
/**
 * 根据文件块指针，得到文件内存首地址
 * @param block
 * @return
 */
DLL_EXPORT char *GetBlockData(FileBlock *block);
/**
 * 释放文件块
 * @param block
 */
DLL_EXPORT void ReleaseFileBlock(FileBlock *block);
}

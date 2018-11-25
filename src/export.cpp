#include "export.h"
ComDocIO *ReadComDocFile(const char *fileName)
{
	ComDocIO *io = nullptr;
	try
	{
		io = new ComDocIO(fileName);
	}
	catch (...)
	{
	}
	return io;
}
void ReleaseComDocFile(ComDocIO *io)
{
	if (io != nullptr)
	{
		delete io;
		io = nullptr;
	}
}
FileBlock *ReadFromPath(ComDocIO *io, const char *path)
{
	FileBlock *pointer = nullptr;
	try
	{
		if (io != nullptr)
		{
			pointer = io->ReadFromPath(path);
		}
	}
	catch (...)
	{
	}
	return pointer;
}
uint32_t GetBlockLength(FileBlock *block)
{
	try
	{
		if (block != nullptr)
		{
			return block->length_;
		}
		else
		{
			return 0;
		}
	}
	catch (...)
	{
		return 0;
	}
}
char *GetBlockData(FileBlock *block)
{
	try
	{
		if (block != nullptr)
		{
			return block->pointer_;
		}
	}
	catch (...)
	{
	}
	return nullptr;
}
void ReleaseFileBlock(FileBlock *block)
{
	if (block != nullptr)
	{
		delete block;
		block = nullptr;
	}
}
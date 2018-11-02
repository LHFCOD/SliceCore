#include "MicroComDocIO.h"
using namespace MicroComDoc;
#include <vector>

ComDocIO::ComDocIO(QString RootDir)
{
	::OleInitialize(NULL);
	wchar_t wpath[100] = { 0 };
	RootDir.toWCharArray(wpath);
	HRESULT hr = StgOpenStorage(wpath, NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &rootSt);
	if (FAILED(hr))
	{
		throw std::exception("Read MDS file error!");
	}
}
FileBlock* ComDocIO::ReadFromPath(QString path)
{
	QStringList dirList = path.split("\\");
	//对路径合法性进行检查
	for (int i = 0; i < dirList.count(); i++)
	{
		if (i == 0)
		{
			if (!dirList[i].isEmpty())
				throw std::exception("path has error!");
		}
		else
		{
			if (dirList[i].isEmpty())
				throw std::exception("path has error!");
		}
	}
	IStorage *parentSt = rootSt;
	std::vector<IStorage*> StVec;
	for (int i = 1; i < dirList.count() - 1; i++)
	{
		IStorage *childSt;
		//将目录名字转化为宽字符
		wchar_t wpath[100] = { 0 };
		dirList[i].toWCharArray(wpath);
		HRESULT hr = parentSt->OpenStorage(wpath, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, 0, &childSt);
		if (FAILED(hr))
		{
			throw std::exception("Read MDS file error!");
		}
		parentSt = childSt;
		//添加Storage到父母录链中
		StVec.insert(StVec.end(), parentSt);
	}
	wchar_t wpath[100] = { 0 };
	dirList.last().toWCharArray(wpath);
	IStream* pStream;
	HRESULT hr = parentSt->OpenStream(wpath, 0, STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);
	if (FAILED(hr))
	{
		throw std::exception("Read MDS file error!");
	}
	//获取流信息
	STATSTG stg;
	pStream->Stat(&stg, NULL);
	char* content = (char*)malloc(stg.cbSize.QuadPart);
	unsigned long readEd = 0;
	pStream->Read(content, stg.cbSize.QuadPart, &readEd);
	//把流释放掉
	pStream->Release();
	//把父目录释放掉
	for(auto *st : StVec)
	{
		if (st != nullptr)
			st->Release();
	}
	return new FileBlock((byte_t*)content, stg.cbSize.QuadPart);
}

ComDocIO::~ComDocIO()
{
	if (rootSt != nullptr)
	{
		rootSt->Release();
		rootSt = nullptr;
	}
	::OleUninitialize();
}

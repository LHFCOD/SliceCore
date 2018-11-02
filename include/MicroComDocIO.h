#pragma once
#include <objbase.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include "BaseDataStruct.h"
using namespace BaseDataStruct;
namespace MicroComDoc
{
	class ComDocIO
	{
	public:
		ComDocIO(QString RootDir);
		~ComDocIO();
	public:
		IStorage *rootSt = nullptr;
		FileBlock* ReadFromPath(QString path);
	};
}


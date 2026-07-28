#pragma once

#include "CoreMinimal.h"
#include "FastXml.h"
#include "CityGMLDataType.h"
#include "CityGmlIFastXmlCallback.h"

class CITYGMLIMPORTEREDITOR_API GMLInterpreter
{
public:
	int64 PolyIndex;
	CityGmlIFastXmlCallback* cllback;
	GMLInterpreter();
	void Interpret(FString GMLCode, FString filename);
};

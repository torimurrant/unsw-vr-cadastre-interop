#include "GMLInterpreter.h"
#include "FastXml.h"

GMLInterpreter::GMLInterpreter()
{
}

void GMLInterpreter::Interpret(FString GMLCode, FString filename)
{
	FText errormsg;
	int32 errorline;
	cllback = nullptr;
	cllback = new CityGmlIFastXmlCallback(filename);
	FFastXml::ParseXmlFile((IFastXmlCallback*)cllback,TEXT(""), (TCHAR*) *GMLCode, GWarn,true,false,errormsg,errorline);
}


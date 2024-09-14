#include "ParseJson.h"
#include "DebugTool/Log.h"
#include "OS/MyWindow.h"
Document ParseJson(std::string JsonFileName)
{
	std::ifstream fin(JsonFileName);
	if (!fin)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL, "there is no %s\n", JsonFileName.data());
		DebugBreak();
	}
	std::string json((std::istreambuf_iterator<char>(fin)), (std::istreambuf_iterator<char>()));
	fin.close();
	Document d;
	rapidjson::ParseResult parseResult = d.Parse(json.data());
	if (!parseResult) {
		fprintf(stderr, "JSON parse error: %s (%d)", GetParseError_En(parseResult.Code()), parseResult.Offset());
		DebugBreak();
	}
    return d;
}

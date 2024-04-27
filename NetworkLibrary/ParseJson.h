#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
using namespace rapidjson;

Document ParseJson(std::string JsonFileName);
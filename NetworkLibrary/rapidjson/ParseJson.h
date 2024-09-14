#include <string>
#include "document.h"
#include "writer.h"
#include "stringbuffer.h"
#include "error/en.h"
using namespace rapidjson;

Document ParseJson(std::string JsonFileName);
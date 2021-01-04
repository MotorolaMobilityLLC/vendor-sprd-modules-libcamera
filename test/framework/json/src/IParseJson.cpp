#include "IParseJson.h"
#include <algorithm> // sort
#include <iostream>
#include <sstream>
#include <stddef.h>
#include <stdio.h>
IParseJson *IParseJson::CreateJsonItem(string strKey) { return this; }

void IParseJson::PrintValueTree(Json::Value &value, IParseJson *pParent,
                                IParseJson *pobj, const std::string strkey) {
    IParseJson *pTmpJsonNode = NULL;
    switch (value.type()) {
    case Json::nullValue: {
        break;
    }
    case Json::intValue:
        pobj->DealJsonNode(strkey, value.asInt());
        break;
    case Json::uintValue:
        pobj->DealJsonNode(strkey, value.asUInt());
        break;
    case Json::realValue:
        pobj->DealJsonNode(strkey, value.asFloat());
        break;
    case Json::stringValue:
        pobj->DealJsonNode(strkey, value.asString());
        break;
    case Json::booleanValue:
        pobj->DealJsonNode(strkey, value.asBool());
        break;
    case Json::arrayValue: {
        int size = value.size();

        for (int index = 0; index < size; ++index) {
            pTmpJsonNode = pParent->CreateJsonItem(strkey);
            PrintValueTree(value[index], pParent, pTmpJsonNode, strkey);
        }

    } break;
    case Json::objectValue: {
        Json::Value::Members members(value.getMemberNames());
        std::sort(members.begin(), members.end());
        std::string suffix = "";
        for (Json::Value::Members::iterator it = members.begin();
             it != members.end(); ++it) {
            const std::string &name = *it;

            PrintValueTree(value[name], pobj, pobj, name);
        }
    } break;
    default:
        break;
    }
}

BOOL IParseJson::ParseJson(const std::string &json) {
    Json::Features features;
    Json::Reader reader(features);
    Json::Value root;
    bool parsingSuccessful =
        reader.parse(json.data(), json.data() + json.size(), root);
    if (!parsingSuccessful) {
        std::cout << "parsing json failed!!!!!" << std::endl;
        return 0;
    }
    PrintValueTree(root, this, this, "");
    return 1;
}

std::string IParseJson::readInputTestFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file)
        return std::string("");
    fseek(file, 0, SEEK_END);
    long const size = ftell(file);
    unsigned long const usize = static_cast<unsigned long>(size);
    fseek(file, 0, SEEK_SET);
    std::string text;
    char *buffer = new char[size + 1];
    buffer[size] = 0;
    if (fread(buffer, 1, usize, file) == usize)
        text = buffer;
    fclose(file);
    delete[] buffer;
    // IT_LOGD("%s",text.data());
    return text;
}
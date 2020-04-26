#include "json.h"

void dump(json::Document &doc) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    doc.Accept(writer);
    std::cout << s.GetString() << std::endl;
}

int main_json(Argument& cmd) {
    duobei::JsonBuilder jb;
    {
#if 0
        auto object = jb.object();
        object["userId"] = "jz";
        object["userName"] = "哈哈哈";
        object["userRole"] = 2;
#else
        auto array = jb.array();
        array.addString("{\"userRole\":\"2\",\"userId\":\"jz1ece67c98abf4171a2dd0b6d4d7fde3c\",\"userName\":\"zzz\"}");
#endif
    }
    std::cout << jb.toString() << std::endl;
    return 0;

#if 0
    PingHistory pingHistory;
    pingHistory.range.entryRTT.push_back(1);
    pingHistory.range.entryRTT.push_back(2);
    pingHistory.range.entryRTT.push_back(3);
    auto doc_6 = pingHistory.dump();
    dump(doc_6);
#endif
    // https://rapidjson.org/md_doc_stream.html
    using namespace rapidjson;

    std::fstream ifs("/Users/guochao/DbyClientTool_profile.json", std::ios::binary | std::ios::in);
    if (!ifs.is_open()) {
        std::cerr << "Could not open file for reading!\n";
        return EXIT_FAILURE;
    }

    IStreamWrapper isw{ifs};

    Document doc;
    doc.ParseStream(isw);

    StringBuffer buffer{};
    Writer<StringBuffer> writer{buffer};
    doc.Accept(writer);

    if (doc.HasParseError()) {
        std::cout << "Error  : " << doc.GetParseError() << '\n'
                  << "Offset : " << doc.GetErrorOffset() << '\n';
        return EXIT_FAILURE;
    }

    const std::string jsonStr{buffer.GetString()};

    std::cout << jsonStr << '\n';

    return 0;
}
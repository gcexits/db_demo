#include "json.h"

void dump(json::Document &doc) {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    doc.Accept(writer);
    std::cout << s.GetString() << std::endl;
}

int main_json() {
    duobei::JsonBuilder jb;
    {
        auto object = jb.object();
        object["room"] = "jz";
    }
    std::cout << jb.toString() << std::endl;

#if 0
    using Pair = std::pair<std::string, std::string>;
    std::list<Pair> list;
    Pair pair;
    pair = std::make_pair("1", "");
    list.push_back(pair);
    for (auto & l : list) {
        if (!l.second.empty()) {
            std::cout << l.first << " " << l.second << std::endl;
        }
    }

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

    Document doc{};
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
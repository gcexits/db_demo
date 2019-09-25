#include <iostream>
#include <string>
#include <map>
#include <variant>

#define LEN4_(dataTmp) ((dataTmp[0] & 0x000000FF) << 24 | (dataTmp[1] & 0x000000FF) << 16 | (dataTmp[2] & 0x000000FF) << 8 | (dataTmp[3] & 0x000000FF))
#define LEN3_(dataTmp) ((dataTmp[0] & 0x000000FF) << 16 | (dataTmp[1] & 0x000000FF) << 8 | (dataTmp[2] & 0x000000FF))
#define LEN2_(dataTmp) ((dataTmp[0] & 0x000000FF) << 8 | (dataTmp[1] & 0x000000FF))
#define LEN1_(dataTmp) ((dataTmp[0] & 0x000000FF))
#define TIME4_(dataTmp) ((dataTmp[3] & 0x000000FF) << 24 | (dataTmp[0] & 0x000000FF) << 16 | (dataTmp[1] & 0x000000FF) << 8 | (dataTmp[2] & 0x000000FF))

int main(int argc, char *argv[]) {
//    std::cout << "****** multimap ******" << std::endl;
//    std::multimap<std::string, int> multimap;
//    multimap.emplace("1", 1);
//    multimap.emplace("1", 2);
//    multimap.emplace("1", 3);
//    std::cout << multimap.count("1") << std::endl;
//    auto rang = multimap.equal_range("1");
//    for (auto &item = rang.first; item != rang.second; item++) {
//        std::cout << item->first << ": " << item->second << std::endl;
//    }
//
//    std::cout << "****** variant ******" << std::endl;
//    std::variant<int, std::string> variant;
//    variant.emplace<int>(1);
//    variant.emplace<std::string >("123");
//    try {
//        std::cout << std::get<std::string>(variant) << std::endl;
//    }
//    catch (const std::bad_variant_access&) {}
//    variant.emplace<int>(2);
//    std::cout << std::get<int>(variant) << std::endl;
//
//
//    std::cout << "****** end ******" << std::endl;
    uint8_t buf[] = {0x46, 0x4c, 0x56, 0x01, 0x04, 0x00, 0x00, 0x00, 0x09};
    return 0;
}
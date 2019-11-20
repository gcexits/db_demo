#include <iostream>
#include <list>
#include <regex>

int main(int argc, char *argv[]) {
    std::list<std::string> list;
    std::list<std::string> list_;
    for (auto &l : list) {
        list_.push_back(l);
    }
    return 0;
}
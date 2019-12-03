#include "c++_test.h"
template <typename T>
struct Pair {
    T a;
    T b;
};

struct Person {
    Person(std::string name_, int age_) : name(name_), age(age_) {};
    std::string name;
    int age;
};

//#define TEMPLATE
#if defined(TEMPLATE)
template <typename T>
std::multimap<T, Person> GroupBy(std::vector<Person> &vt) {
    std::multimap<T, Person> multimap;
    for(auto &item : vt) {
        multimap.emplace(item.age, item);
    }
    return multimap;
}
#else
std::multimap<std::any, Person> GroupBy(std::vector<Person> &vt) {
    // todo: error map<key, value>, key can not use std::any, but value can user std::any
    std::multimap<std::any, Person> multimap;
    for(auto &item : vt) {
//        multimap.emplace(item.age, item);
    }
    return multimap;
}
#endif

struct Out {
    std::string out1{""};
    std::string out2{""};
};

std::pair<int, Out> func(std::string in) {
    Out out;
    if (in.empty()) {
        return {0, out};
    }
    out.out1 = "hello";
    out.out2 = "world";
    return {1, out};
}

std::optional<Out> func_(std::string in) {
    Out out;
    if (in.empty()) {
        return std::nullopt;
    }
    out.out1 = "hello optional";
    out.out2 = "world";
    return out;
}

namespace collect {
struct Remote {
    int video;
};
void Rem(Remote re);
}

template<typename T, typename ValueFn>
std::multimap<T, Person> GroupBy(const std::vector<Person>& vt, const ValueFn& keySlector) {
    std::multimap<T, Person> multimap_;
    std::for_each(vt.begin(), vt.end(), [&multimap_, &keySlector](const Person &p)
    {
        multimap_.emplace(keySlector(p), p);
    });
    return multimap_;
}

template<typename R>
class Range {
public:
    typedef typename R::value_type value_type;
    Range(R& range) : m_range(range) {}

    template<typename Fn>
    std::multimap<typename std::result_of<Fn(value_type)>::type, value_type> GroupBy(const Fn& fnk) {
        typedef typename std::result_of<Fn(value_type)>::type ketype;
        std::multimap<ketype, value_type> multimap_;
        std::for_each(m_range.begin(), m_range.end(), [&multimap_, &fnk](value_type item)
        {
          multimap_.emplace(fnk(item), item);
        });
        return multimap_;
    }
private:
    R m_range;
};


void TestGroupBy(std::vector<Person> &vec_) {
//    auto multimap_ = GroupBy<int>(vec_, [](const Person &p) -> int { return p.age;});
    Range rang(vec_);
    rang.GroupBy([](const Person &p) -> int { return p.age;});
//    auto multimap_ = GroupBy<std::string>(vec_, [](const Person &p) -> std::string
//    {
//        return p.name;
//    });
//    for (auto &item : multimap_) {
//        std::cout << item.first << std::endl;
//    }
}

template <bool T>
int testFun(int a, int b) {
    if (T) {
        std::cout << a << std::endl;
    } else {
        std::cout << b << std::endl;
    }
}

int c_test() {
    std::queue<int> q1;
    q1.push(1);
    q1.emplace(2);
    q1.push(3);
    q1.emplace(4);
    while (!q1.empty()) {
        std::cout << q1.front() << std::endl;
        q1.pop();
    }
    return 0;
    std::vector<int> v1 {1, 1, 2 ,3, 3, 3, 3, 5};
    // todo: vector 去重
    std::sort(v1.begin(), v1.end());
    v1.erase(std::unique(v1.begin(), v1.end()), v1.end());
    for (auto &v : v1) {
        std::cout << v << std::endl;
    }
    std::vector<int> v2 {4};
    std::vector<int> diff;
    // todo: vector 如果不去重，会有多个相同元素
    std::set_difference(v1.begin(), v1.end(), v2.begin(), v2.end(), std::back_inserter(diff));
//    for (auto &v : diff) {
//        std::cout << v << std::endl;
//    }
    return 0;
//    std::map<std::string, std::string> map_;
//    map_.emplace("1", "1");
//    map_.emplace("1", "2");
//    map_["1"] = "3";
//    for (auto &item : map_) {
//        std::cout << item.second << std::endl;
//    }
    std::cout << "****** template ******" << std::endl;
//    testFun<false>(1, 2);
    std::cout << "****** lambda ******" << std::endl;
//    std::cout << "return : " << [=](int i) -> int { return i > 10 && i < 20 ? i : -1;}(11) << std::endl;

    std::cout << "****** multimap ******" << std::endl;
//    std::multimap<std::string, int> multimap;
//    multimap.emplace("1", 1);
//    multimap.emplace("1", 2);
//    multimap.emplace("1", 3);
//    std::cout << multimap.count("1") << std::endl;
//    auto rang = multimap.equal_range("1");
//    for (auto &item = rang.first; item != rang.second; item++) {
//        std::cout << item->first << ": " << item->second << std::endl;
//    }

    std::cout << "****** variant ******" << std::endl;
    std::variant<int, std::string> variant;
    variant.emplace<int>(1);
    variant.emplace<std::string >("123");
    try {
        std::cout << std::get<std::string>(variant) << std::endl;
    }
    catch (const std::bad_variant_access&) {}
    variant.emplace<int>(2);
    std::cout << std::get<int>(variant) << std::endl;
    return 0;

    std::cout << "****** any ******" << std::endl;
//    std::unordered_map<std::string, std::any> m_;
//    m_.emplace("1", 1);
//    m_.emplace("2", std::string("12"));
//    std::cout << std::any_cast<int>(m_["1"]) << std::endl;
//    std::cout << std::any_cast<std::string>(m_["2"]) << std::endl;

//    std::unordered_map<std::string, int> map_;
//    map_.emplace("1", 1);
//    map_.emplace("1", 2);
//    map_.emplace("1", 3);
//    for (auto &[k, v] : map_) {
//        std::cout << k << " : " << v << std::endl;
//    }
#if 1
    std::cout << "****** template模版 ******" << std::endl;
    std::vector<Person> vec;
    vec.push_back(Person("a", 1));
    vec.push_back(Person("b", 1));
    vec.push_back(Person("c", 3));
    TestGroupBy(vec);
    return 0;
#if defined(TEMPLATE)
    std::multimap<int, Person>multimap = GroupBy<int>(vec);
#else
    std::multimap<std::any, Person> multimap = GroupBy(vec);
#endif
    for (auto &item : multimap) {
#if defined(TEMPLATE)
        std::cout << item.second.name << " : " << item.first << std::endl;
#else
        std::cout << item.second.name << " : " << std::any_cast<int>(item.first) << std::endl;
#endif
    }
#endif
    std::cout << "****** std::optional ******" << std::endl;
//    if (auto [status, msg] = func("hi"); status == 1) {
//        std::cout << msg.out1 << std::endl;
//        std::cout << msg.out2 << std::endl;
//    }
//    if (auto ret = func_(""); ret) {
//        std::cout << ret->out1 << std::endl;
//        std::cout << (*ret).out2 << std::endl;
//    }
    std::cout << "****** end ******" << std::endl;
    return 0;
}
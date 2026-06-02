#include <iostream>
#include "nlohmann/json.hpp"
using json = nlohmann :: json;
struct Stu{
    std::string name;
    int age;
    // 告诉JSON库如何转换结构体
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Stu, name, age);
};
int main() {
    Stu stu = {"xiaoming", 23};
    json j = stu;
    std::string json_str = j.dump();
    json j2 = json::parse(json_str);
}
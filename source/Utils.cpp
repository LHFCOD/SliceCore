//
// Created by mi on 18-9-22.
//

#include <fstream>
#include <ostream>
#include <sstream>
#include "../include/Utils.h"

void Utils::splitString(const string &s, vector<std::string> &v, const string &c) {
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        v.push_back(s.substr(pos1, pos2-pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if(pos1 != s.length())
        v.push_back(s.substr(pos1));
}
string Utils::readFileIntoString(const char *filename) {
    ifstream ifile(filename); //将文件读入到ostringstream对象buf中
    std::ostringstream buf;
    char ch;
    while (buf && ifile.get(ch))
        buf.put(ch); //返回与流对象buf关联的字符串
    return buf.str();
}

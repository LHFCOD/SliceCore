//
// Created by mi on 18-9-22.
//

#ifndef SLICEAPI_UTILS_H
#define SLICEAPI_UTILS_H

#include <string>
#include <vector>
using namespace std;
class Utils {
public:
    static void splitString(const string& s, vector<std::string>& v, const string& c);
    static string readFileIntoString(const char *filename);
};


#endif //SLICEAPI_UTILS_H

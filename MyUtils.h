//
// Created by wei on 2021/6/18.
//

#ifndef ONLINE_SALES_SYSTEM_MYUTILS_H
#define ONLINE_SALES_SYSTEM_MYUTILS_H


#include <string>
#include <vector>

class MyUtils {
public:
    static std::string double2str(double d, int fixed_precision);

    static std::vector<std::string> split(std::string str, std::string token);
};


#endif //ONLINE_SALES_SYSTEM_MYUTILS_H

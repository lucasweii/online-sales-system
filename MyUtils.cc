//
// Created by wei on 2021/6/18.
//

#include "MyUtils.h"
#include <iomanip>
using namespace std;
std::string MyUtils::double2str(double d, int fixed_precision) {
    std::stringstream ss_temp;
    ss_temp << setiosflags(ios::fixed) << setprecision(fixed_precision) << d;
    return ss_temp.str();
}

std::vector<std::string> MyUtils::split(std::string str, std::string token) {
    vector<string>result;
    while(str.size()){
        int index = str.find(token);
        if(index!=string::npos){
            result.push_back(str.substr(0,index));
            str = str.substr(index+token.size());
            if(str.size()==0)result.push_back(str);
        }else{
            result.push_back(str);
            str = "";
        }
    }
    return result;
}

int MyUtils::str2int(std::string str) {
    stringstream ss;
    ss << str;
    int num;
    ss >> num;
    return num;
}
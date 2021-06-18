#include "User.h"
#include <fmt/format.h>
using namespace fmt;
using namespace std;
using namespace orm;
//add definition of your processing function here
void User::login(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto res = HttpResponse::newHttpViewResponse("user_login.csp");
        callback(res);
    } else if (req->method() == Post) {
        auto param = req->getParameters();
        print("user id is {} \n", param["uid"]);
        print("user pwd is {}\n", param["pwd"]);
        auto clientPtr = drogon::app().getDbClient();
        Result ret = clientPtr->execSqlSync(
                "select * from `user` where `uid`=?",
                param["uid"]
                );
        assert(ret.size() <= 1);
        // check the correctness of password
        if(ret.size() == 1 && ret[0]["pwd"].as<string>() == param["pwd"]) {
            req->session()->insert("login", true);
            req->session()->insert("uid", param["uid"]);
            req->session()->insert("user_name", ret[0]["name"].as<string>());
            req->session()->insert("user_pic", ret[0]["profile_pic_path"].as<string>());
            // jump according to user type
            bool isRetailer = ret[0]["type"].as<bool>();
            req->session()->insert("is_retailer", isRetailer);
            LOG_DEBUG << "jump";
            if(isRetailer){
                auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
                callback(resp);
            } else {
                auto resp = HttpResponse::newRedirectionResponse("/index_customer");
                callback(resp);
            }

        } else {
            HttpViewData data;
            data.insert("wrong", true);
            auto res = HttpResponse::newHttpViewResponse("user_login.csp", data);
            callback(res);
        }
    }
}

void User::registerUser(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto res = HttpResponse::newHttpViewResponse("user_register.csp");
        callback(res);
    } else if(req->method() == Post) {
        auto param = req->getParameters();
        print("new user id is {} \n", param["uid"]);
        print("new user pwd is {}\n", param["pwd"]);
        print("new user name is {}\n", param["name"]);
        print("new user account is {}\n", param["account"]);
        bool isRetailer = false;

        if(param.find("retailer") != param.end()) {
            print("new user is retailer\n");
            isRetailer = true;
        }

        auto DbPtr = app().getDbClient();
        *DbPtr  << "insert into `user`(`uid`, `pwd`, `name`, `type`, `account`) values(?, ?, ?, ?, ?)"
                << param["uid"] << param["pwd"] << param["name"] << static_cast<int>(isRetailer) << param["account"]
                >> [=,uid=param["uid"], name=param["name"]](const Result &r)
                {
                    print("insert into user success!\n");
                    req->session()->insert("login", true);
                    req->session()->insert("uid", uid);
                    req->session()->insert("name", name);
                    req->session()->insert("is_retailer", isRetailer);
                    req->session()->insert("user_pic", "./resources/profile_pic/default.jpeg");
                    if(isRetailer){
                        Result ret = DbPtr->execSqlSync(
                                "select * from `shop` where `shop`.`uid`=?",
                                uid);
                        assert(ret.size() <= 1);
                        if(ret.empty()){
                            auto resp = HttpResponse::newRedirectionResponse("/new_shop");
                            callback(resp);
                        } else {
                            auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
                            callback(resp);
                        }
                    } else {
                        auto resp = HttpResponse::newRedirectionResponse("/index_customer");
                        callback(resp);
                    }
                }
                >> [=](const DrogonDbException &e)
                {
                    HttpViewData data;
                    data.insert("wrong", true);
                    auto res = HttpResponse::newHttpViewResponse("user_register", data);
                    callback(res);
                };
    }
}

void User::newShop(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if (req->method() == Post) {
        LOG_DEBUG << "new shop";
        auto uid = req->session()->get<string>("uid");
        auto param = req->getParameters();
        auto DbPtr = app().getDbClient();
        *DbPtr  << "insert into `shop` (`name`, `description`, `uid`) values (?,?,?)"
                << param["name"] << param["description"] << uid
                >> [=](const Result &r)
                {
                    LOG_DEBUG <<"creat new shop success!";
                    auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
                    callback(resp);
                }
                >> [=](const DrogonDbException &e)
                {
                    LOG_DEBUG <<"create new shop failed!";
                    HttpViewData data;
                    data.insert("wrong", true);
                    auto resp = HttpResponse::newHttpViewResponse("new_shop.csp", data);
                    callback(resp);
                };
    } else {
        auto resp = HttpResponse::newHttpViewResponse("new_shop.csp");
        callback(resp);
    }
}

void User::logout(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    req->session()->clear();
    auto resp = HttpResponse::newRedirectionResponse("/login");
    callback(resp);
}

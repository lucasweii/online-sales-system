#include "User.h"
#include <fmt/format.h>
using namespace fmt;
using namespace std;
using namespace orm;
//add definition of your processing function here
void User::login(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto res = HttpResponse::newHttpViewResponse("login.csp");
        callback(res);
    } else if (req->method() == Post) {
        auto param = req->getParameters();
        print("user id is {} \n", param["uid"]);
        print("user pwd is {}\n", param["pwd"]);
        Result ret(nullptr);
        auto clientPtr = drogon::app().getDbClient();
        {
            auto txn = clientPtr->newTransaction();
            *txn << "select * from `user` where `uid`=?"
                 << param["uid"]
                 << Mode::Blocking;
        }
        assert(ret.size() <= 1);
        if(ret.size() == 1 && ret[0]["pwd"].as<string>() == param["pwd"]) {
            auto res = HttpResponse::newHttpResponse();
            req->session()->insert("login", true);
            req->session()->insert("uid", param["uid"]);
            res->setStatusCode(k200OK);
            res->setBody("Login Success!");
            // TODO login success jump
            callback(res);
        } else {
            HttpViewData data;
            data.insert("wrong", true);
            auto res = HttpResponse::newHttpViewResponse("login.csp", data);
            callback(res);
        }
    }
}

void User::registerUser(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto res = HttpResponse::newHttpViewResponse("register_new.csp");
        callback(res);
    } else if(req->method() == Post) {
        auto param = req->getParameters();
        print("new user id is {} \n", param["uid"]);
        print("new user pwd is {}\n", param["pwd"]);
        print("new user name is {}\n", param["name"]);
        print("new user account is {}\n", param["account"]);
        int isRetailer = 0;

        if(param.find("retailer") != param.end()) {
            print("new user is retailer\n");
            isRetailer = 1;
        }

        auto clientPtr = drogon::app().getDbClient();
        {
            auto txn = clientPtr->newTransaction();
            *txn    << "insert into `user` values(?, ?, ?, ?, ?)"
                    << param["uid"] << param["pwd"] << param["name"] << isRetailer << param["account"]
                    >> [=](const Result &r)
                        {
                            print("insert into user success!\n");
                            auto res = HttpResponse::newHttpResponse();
                            res->setStatusCode(k200OK);
                            res->setBody("registration success!");
                            callback(res);
                        }
                    >> [=](const DrogonDbException &e)
                        {
                            HttpViewData data;
                            data.insert("wrong", true);
                            auto res = HttpResponse::newHttpViewResponse("register_new.csp", data);
                            callback(res);
                        };
        }
    }
}

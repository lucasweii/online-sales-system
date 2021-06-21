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
                    req->session()->insert("user_name", name);
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

void
User::profile(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, const string &action) {
    if (req->method() == Get) {
        auto DbPtr = app().getDbClient();
        Result r = DbPtr->execSqlSync(
                "select * from `user` where `uid`=?",
                req->session()->get<string>("uid")
                );
        assert(r.size() == 1);
        HttpViewData data;
        data.insert("isRetailer", req->session()->get<bool>("is_retailer"));
        data.insert("uid", req->session()->get<string>("uid"));
        data.insert("user_name", r[0]["name"].as<string>());
        data.insert("user_pic", r[0]["profile_pic_path"].as<string>());
        data.insert("account", r[0]["account"].as<string>());
        data.insert("person_info", r[0]["person_info"].as<string>());
        data.insert("pwd", r[0]["pwd"].as<string>());
        if(action == "display"){
            auto resp = HttpResponse::newHttpViewResponse("profile.csp", data);
            callback(resp);
        } else {
            auto resp = HttpResponse::newHttpViewResponse("profile_modify.csp", data);
            callback(resp);
        }
    } else if(req->method() == Post) {
        auto param = req->parameters();
        for(auto &it : param){
            fmt::print("{}:{}\n", it.first, it.second);
        }
        auto DbPtr = app().getDbClient();
        if(param["profile_pic"].empty()) {
            *DbPtr  << "update `user` set `name`=?, `pwd`=?, `account`=?, `person_info`=?"
                       "where `uid`=?"
                    << param["user_name"] << param["pwd"] << param["account"] << param["person_info"] << req->session()->get<string>("uid")
                    >> [=, name=param["user_name"], id=param["user_id"]](const Result &r)
                    {
                        LOG_DEBUG << "modify user <name:" << name << "> profile information";
                        req->session()->modify<string>("user_name",[=](string &old_name){
                            old_name = name;
                        });
                    }
                    >> [](const DrogonDbException &e)
                    {
                        LOG_DEBUG << e.base().what();
                    };
        } else {
            *DbPtr  << "update `user` set `name`=?, `pwd`=?, `account`=?, `person_info`=?, `profile_pic_path`=?"
                       "where `uid`=?"
                    << param["user_name"] << param["pwd"] << param["account"] << param["person_info"] <<"./resources/profile_pic/" + param["profile_pic"]
                    << req->session()->get<string>("uid")
                    >> [=, name=param["user_name"]](const Result &r)
                    {
                        LOG_DEBUG << "modify user <name:" << name << "> profile information";
                        req->session()->modify<string>("user_name",[=](string &old_name){
                            old_name = name;
                        });
                    }
                    >> [](const DrogonDbException &e)
                    {
                        LOG_DEBUG << e.base().what();
                    };
        }
        auto resp = HttpResponse::newRedirectionResponse("/profile?action=display");
        callback(resp);
    }
}

void User::bill(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    auto DbPtr = app().getDbClient();
    Result user = DbPtr->execSqlSync(
            "select `account` from `user` where `uid`=?",
            req->session()->get<string>("uid")
    );
    assert(user.size() == 1);

    auto account = user[0]["account"].as<string>();

    Result r = DbPtr->execSqlSync(
            "select * from `bill` where `account_out`=? or `account_in`=? order by `date` desc",
            account, account
    );

    HttpViewData data;
    vector<unordered_map<string, string>> bill_data;
    data.insert("isRetailer", req->session()->get<bool>("is_retailer"));
    data.insert("user_name", req->session()->get<string>("user_name"));
    data.insert("user_pic", req->session()->get<string>("user_pic"));
    for(auto & bill : r){
        bill_data.emplace_back();
        bill_data.back()["date"] = bill["date"].as<string>();
        bill_data.back()["id"] = bill["id"].as<string>();
        bill_data.back()["money"] = bill["money"].as<string>();
        bill_data.back()["account_out"] = bill["account_out"].as<string>();
        bill_data.back()["account_in"] = bill["account_in"].as<string>();
        if(account == bill_data.back()["account_out"]) {
            bill_data.back()["comment"] = "支出";
        } else {
            bill_data.back()["comment"] = "收入";
        }
    }
    data.insert("bill_data", move(bill_data));
    auto resp = HttpResponse::newHttpViewResponse("bill_history.csp", data);
    callback(resp);
}

#pragma once

#include <drogon/HttpController.h>

#define USER_TYPE_CUSTOMER (0)
#define USER_TYPE_RETAILER (1)
using namespace drogon;

class User : public drogon::HttpController<User> {
public:
    METHOD_LIST_BEGIN
        //use METHOD_ADD to add your custom processing function here;
        ADD_METHOD_TO(User::login, "/", Get, "Log");
        ADD_METHOD_TO(User::login, "/login", Get, Post, "Log");
        ADD_METHOD_TO(User::logout, "/logout", Get, "Log");
        ADD_METHOD_TO(User::registerUser, "/register", Get, Post, "Log");
        ADD_METHOD_TO(User::newShop, "/new_shop", Get, Post, "Log", "SessionControl");
        ADD_METHOD_TO(User::profile, "/profile?action={}", Get, Post, "Log", "SessionControl", "RemoveOrders");
        //METHOD_ADD(User::your_method_name,"/{1}/{2}/list",Get);//path is /User/{arg1}/{arg2}/list
        //ADD_METHOD_TO(User::your_method_name,"/absolute/path/{1}/{2}/list",Get);//path is /absolute/path/{arg1}/{arg2}/list

    METHOD_LIST_END

    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req,std::function<void (const HttpResponsePtr &)> &&callback,int p1,std::string p2);
    // void your_method_name(const HttpRequestPtr& req,std::function<void (const HttpResponsePtr &)> &&callback,double p1,int p2) const;
    void login(const HttpRequestPtr &req,
               std::function<void(const HttpResponsePtr &)> &&callback
    );

    void registerUser(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback
    );

    void newShop(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback
    );

    void logout(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback
    );

    void profile(const HttpRequestPtr &req,
                 std::function<void(const HttpResponsePtr &)> &&callback, const std::string &action
    );
};

#include "Retailer.h"
#include "User.h"
#include <fmt/format.h>
#include <iomanip>
#include "../MyUtils.h"
//add definition of your processing function here
using namespace std;
using namespace orm;

void Retailer::newGoods(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        HttpViewData data;
        vector<string> category;
        auto DbPtr = app().getDbClient();
        Result ret = DbPtr->execSqlSync("select distinct `category` from `goods`");
        for(auto &it : ret){
            category.push_back(it["category"].as<string>());
        }
        data.insert("category", category);
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        data.insert("shop_name", req->session()->get<string>("shop_name"));
        data.insert("user_name", req->session()->get<string>("user_name"));
        auto resp = HttpResponse::newHttpViewResponse("new_goods.csp", data);
        callback(resp);
    } else if(req->method() == Post) {
        auto param = req->getParameters();
        for(const auto &it : param){
            fmt::print("{}:{}\n", it.first, it.second);
        }
        auto DbPtr = app().getDbClient();

        string category = param["select_category"];
        if(param["select_category"] == "none") {
            category = param["new_category"];
        }

        string pic_path = param["goods_pic"].empty() ? "./resources/goods_pic/default_goods_pic.png" : "./resources/goods_pic/" + param["goods_pic"];
        string discount = param["discount"].empty() ? "0" : param["discount"];

        *DbPtr  << "insert into `goods` (`name`,`price`,`category`, `stock` , `shop_id`, `pic_path` ,`discount`,`description`) values (?,?,?,?,?,?,?,?)"
                << param["goods_name"] << param["goods_price"] << category << param["goods_stock"]
                << req->session()->get<int>("shop_id") << pic_path << discount << param["description"]
                >> [=](const Result &r)
                {
                    LOG_DEBUG << "upload new goods success!";
                }
                >> [=](const DrogonDbException &e)
                {
                    LOG_DEBUG << e.base().what();
                };

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setBody("add new goods success!");
        callback(resp);
    }
}

void Retailer::retailerIndex(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Post){
        auto param = req->getParameters();
        assert(param.size() == 1);
        auto goods_operation = MyUtils::split(param.begin()->first, "@");
        auto op = goods_operation[0];
        auto goods_id = goods_operation[1];

        auto DbPtr = app().getDbClient();
        if(op == "stock") {
            *DbPtr  << "update `goods` set `stock`=? where `id`=?"
                    << param.begin()->second << goods_id
                    >> [=](const Result &r)
                    {
                        LOG_DEBUG << "modify goods <id:" << goods_id <<  "> stock success";
                    }
                    >> [=](const DrogonDbException &e)
                    {
                        LOG_DEBUG << e.base().what();
                    };
        } else if(op == "delete"){
            *DbPtr  << "delete from `goods` where `id`=?"
                    << goods_id
                    >> [=](const Result &r)
                    {
                        LOG_DEBUG << "delete goods <id:" << goods_id << "> success";
                    }
                    >> [](const DrogonDbException &e)
                    {
                        LOG_DEBUG << e.base().what();
                    };
        }
        auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
        callback(resp);
    } else {
        auto DbPtr = app().getDbClient();
        Result shop_ret = DbPtr->execSqlSync(
                "select `name`, `id` from `shop` where `uid` = ?",
                req->session()->get<string>("uid")
                );
        assert(shop_ret.size() == 1);
        Result goods_ret = DbPtr->execSqlSync(
                "select `id`, `name`, `price`, `stock`,`discount`, `pic_path`, `description` from `goods` where `shop_id` = ?",
                shop_ret[0]["id"].as<int>()
                );
        HttpViewData data;
        vector<unordered_map<string, string>> goods_data;
        for(auto &goods : goods_ret){
            goods_data.emplace_back();
            goods_data.back()["id"] = to_string(goods["id"].as<int>());
            goods_data.back()["name"] = goods["name"].as<string>();
            goods_data.back()["price"] = MyUtils::double2str(goods["price"].as<double>(), 2);
            goods_data.back()["discount"] = MyUtils::double2str(goods["discount"].as<double>(), 1);
            goods_data.back()["stock"] = to_string(goods["stock"].as<int>());
            goods_data.back()["pic_path"] = goods["pic_path"].as<string>();
            auto description = goods["description"].as<string>();
            if(description.length() > 30) {
                description =  description.substr(30);
                description += "...";
            }
            goods_data.back()["description"] =description;
        }
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        data.insert("shop_name", shop_ret[0]["name"].as<string>());
        req->session()->insert("shop_name", shop_ret[0]["name"].as<string>());
        req->session()->insert("shop_id", shop_ret[0]["id"].as<int>());
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("goods_data", goods_data);
        auto resp = HttpResponse::newHttpViewResponse("index_retailer.csp", data);
        callback(resp);
    }
}

void Retailer::goodsDetail(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback,
                            const string &id) {
    auto DbPtr = app().getDbClient();
    if(req->method() == Get) {
        Result ret = DbPtr->execSqlSync(
                "select * from `goods` where `id`=?",
                id
        );
        assert(ret.size() == 1);
        HttpViewData data;
        unordered_map<string, string> goods_data;
        goods_data["id"] = id;
        goods_data["name"] = ret[0]["name"].as<string>();
        goods_data["price"] = MyUtils::double2str(ret[0]["price"].as<double>(), 2);
        goods_data["discount"] = MyUtils::double2str(ret[0]["discount"].as<double>(), 1);
        goods_data["stock"] = to_string(ret[0]["stock"].as<int>());
        goods_data["pic_path"] = ret[0]["pic_path"].as<string>();
        goods_data["description"] = ret[0]["description"].as<string>();
        goods_data["category"] = ret[0]["category"].as<string>();
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        data.insert("shop_name", req->session()->get<string>("shop_name"));
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("goods_data", goods_data);
        auto resp = HttpResponse::newHttpViewResponse("goods_detail.csp", data);
        callback(resp);
    } else {
        auto param = req->getParameters();
        for(auto &it : param) {
            fmt::print("{}:{}\n", it.first, it.second);
        }
        if(param["goods_pic"].empty()) {
            *DbPtr  << "update `goods` set `name`=?, `price`=?, `category`=?, `discount`=?, `description`=?, `stock`=? "
                       "where `id`=?"
                    << param["goods_name"] << param["goods_price"] << param["category"] << param["discount"]
                    << param["description"] << param["goods_stock"] << id
                    >> [=](const Result &r)
                    {
                        LOG_DEBUG << "modify goods <id:" << id << "> detail success";
                    }
                    >> [](const DrogonDbException &e)
                    {
                        LOG_DEBUG << e.base().what();
                    };
        } else {
            *DbPtr  << "update `goods` set `name`=?, `price`=?, `category`=?, `discount`=?, "
                       "`description`=?, `stock`=?, `pic_path`=? where `id`=?"
                    << param["goods_name"] << param["goods_price"] << param["category"] << param["discount"]
                    << param["description"] << param["goods_stock"] << "./resources/goods_pic/" + param["goods_pic"] << id
                    >> [=](const Result &r)
                    {
                        LOG_DEBUG << "modify goods <id:" << id << "> detail success";
                    }
                    >> [](const DrogonDbException &e)
                    {
                        LOG_DEBUG << e.base().what();
                    };
        }
        auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
        callback(resp);
    }
}


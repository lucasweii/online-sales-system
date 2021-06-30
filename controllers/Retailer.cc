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

        auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
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
    } else if(req->method() == Get){
        auto DbPtr = app().getDbClient();
        Result shop_ret = DbPtr->execSqlSync(
                "select `name`, `id` from `shop` where `uid` = ?",
                req->session()->get<string>("uid")
                );
        assert(shop_ret.size() == 1);
        Result goods_ret = DbPtr->execSqlSync(
                "select * from `goods` where `shop_id` = ?",
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
            goods_data.back()["sales_num"] = to_string(goods["sales_num"].as<int>());
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

void Retailer::shop(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto DbPtr = app().getDbClient();
        Result r = DbPtr->execSqlSync("select * from `shop` where `uid` = ?", req->session()->get<string>("uid"));
        assert(r.size() == 1);
        HttpViewData data;
        data.insert("shop_name", r[0]["name"].as<string>());
        data.insert("description", r[0]["description"].as<string>());
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        auto resp = HttpResponse::newHttpViewResponse("shop_manage.csp", data);
        callback(resp);
    }
}

void Retailer::shopModify(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    auto DbPtr = app().getDbClient();
    if(req->method() == Get) {
        Result r = DbPtr->execSqlSync("select * from `shop` where `uid` = ?", req->session()->get<string>("uid"));
        assert(r.size() == 1);
        HttpViewData data;
        data.insert("shop_name", r[0]["name"].as<string>());
        data.insert("description", r[0]["description"].as<string>());
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        auto resp = HttpResponse::newHttpViewResponse("shop_modify.csp", data);
        callback(resp);
    } else if(req->method() == Post){
        auto param = req->parameters();
        for(const auto &it : param) {
            fmt::print("{}:{}\n", it.first, it.second);
        }
        {
            auto txn = DbPtr->newTransaction();
            *txn    << "select * from `shop` where `name`=? and `name` != ?"
                    << param["new_name"] << req->session()->get<string>("shop_name")
                    >> [=, name=param["new_name"], description=param["description"], id=req->session()->get<int>("shop_id")](const Result &r)
                    {
                        if(!r.empty()) {
                            LOG_DEBUG << "shop name : "<< name << " is repeated";
                            *txn    << "select * from `shop` where `id`=?"
                                    << id
                                    >> [=](const Result &r)
                                    {
                                        assert(r.size() == 1);
                                        HttpViewData data;
                                        data.insert("shop_name", r[0]["name"].as<string>());
                                        data.insert("description", r[0]["description"].as<string>());
                                        LOG_DEBUG << "break0";
                                        data.insert("user_name", req->session()->get<string>("user_name"));
                                        data.insert("user_pic", req->session()->get<string>("user_pic"));
                                        LOG_DEBUG << "break1";
                                        data.insert("wrong", true);
                                        data.insert("new_name", name);
                                        auto resp = HttpResponse::newHttpViewResponse("shop_modify.csp", data);
                                        callback(resp);
                                    }
                                    >> [=](const DrogonDbException &e)
                                    {
                                        LOG_DEBUG << e.base().what();
                                        auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
                                        callback(resp);
                                    };
                        } else {
                            *txn    << "update `shop` set `name` = ?, `description`=? where `id`=?"
                                    << name << description << id
                                    >> [=](const Result &r)
                                    {
                                        req->session()->modify<string>("shop_name",[=](string &old_name){
                                            old_name = name;
                                        });
                                        LOG_DEBUG << "modify shop <id:" << id << "> information success!";
                                        auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
                                        callback(resp);
                                    }
                                    >> [=](const DrogonDbException &e)
                                    {
                                        LOG_DEBUG << e.base().what();
                                        auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
                                        callback(resp);
                                    };
                        }
                    }
                    >> [=](const DrogonDbException &e)
                    {
                        LOG_DEBUG << e.base().what();
                        auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
                        callback(resp);
                    }
                    ;
        }
    }
}

void Retailer::orderProcess(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto DbPtr = app().getDbClient();
        HttpViewData data;
        vector<unordered_map<string, string>> orders;
        Result r_shop = DbPtr->execSqlSync("select `id` from `shop` where `uid`=?", req->session()->get<string>("uid"));
        assert(r_shop.size() == 1);
        auto shop_id = r_shop[0]["id"].as<string>();
        Result r = DbPtr->execSqlSync(
                "select `date`,`order`.`id`, `uid`,`goods_id`,`num` from `order` inner join `goods` on `goods`.`id`=`order`.`goods_id`"
                "where `state`=1 and `shop_id`=?"
                "order by `date` desc",
                shop_id
                        );
        for(const auto &order : r) {
            orders.emplace_back();
            orders.back()["date"] = order["date"].as<string>();
            orders.back()["order_id"] = order["id"].as<string>();
            orders.back()["uid"] = order["uid"].as<string>();
            auto goods_id = order["goods_id"].as<string>();
            orders.back()["goods_id"] = goods_id;
            Result goods = DbPtr->execSqlSync("select `name` from `goods` where `id`=?", goods_id);
            assert(goods.size() == 1);
            orders.back()["num"] = order["num"].as<string>();
        }
        data.insert("order_data", orders);
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("shop_name", req->session()->get<string>("shop_name"));
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        auto resp = HttpResponse::newHttpViewResponse("order_process.csp", data);
        callback(resp);
    } else if(req->method() == Post) {
        auto param = req->getParameters();
        auto DbPtr = app().getDbClient();
        Result r = DbPtr->execSqlSync("select `account` from `user` where`uid`=?", req->session()->get<string>("uid"));
        assert(r.size() == 1);
        auto account = r[0]["account"].as<string>();
        for(auto &it : param) {
            auto id = MyUtils::split(it.first, "@")[1];
            fmt::print("{}\n",it.first);
            if(it.second == "acc") {
                *DbPtr  << "update `order` set `state` = 2 where `id`=?"
                        << id
                        >>[=](const Result &r)
                        {
                            LOG_DEBUG << "deal success <order_id:" <<id <<" > !";
                        }
                        >>[=](const DrogonDbException &e)
                        {
                            LOG_DEBUG << e.base().what();
                        };
            } else {
                auto txn = DbPtr->newTransaction();
                *txn    << "update `order` set `state` = 3 where`id`=?"
                        << id
                        >> [=](const Result &r)
                        {
                            *txn    << "select * from `order` where `id`=?"
                                    << id
                                    >>[=](const Result &r)
                                    {
                                        assert(r.size() == 1);
                                        auto customer_uid = r[0]["uid"].as<string>();
                                        auto num = r[0]["num"].as<int>();
                                        *txn    << "select `account`, `price`, `discount`"
                                                   "from `user`, `goods` "
                                                   "where `goods`.`shop_id`=? and `user`.`uid`=?"
                                                << req->session()->get<int>("shop_id") << customer_uid
                                                >> [=](const Result &r)
                                                {
                                                    if(!r.empty()) {
                                                        auto price = r[0]["price"].as<double>();
                                                        auto discount = r[0]["discount"].as<double>() / 10;
                                                        if(discount == 0) {
                                                            discount = 1.0;
                                                        }
                                                        auto total = price * discount * num;
                                                        auto date = trantor::Date::now();
                                                        *txn    << "insert into `bill` (`account_out`, `account_in`, `date`, `money`) values (?,?,?,?)"
                                                                << account << r[0]["account"].as<string>() << date << total
                                                                >> [=](const Result &r)
                                                                {
                                                                    LOG_DEBUG << "create bill <id:" << r.insertId() <<  " > success !";
                                                                }
                                                                >> [=](const DrogonDbException &e)
                                                                {
                                                                    LOG_DEBUG << e.base().what();
                                                                };
                                                    } else {
                                                        LOG_ERROR << "There should have been an account of user <id:" << customer_uid << ">";
                                                        txn->rollback();
                                                    }
                                                }
                                                >> [=](const DrogonDbException &e)
                                                {
                                                    LOG_DEBUG << e.base().what();
                                                };

                                    }
                                    >>[=](const DrogonDbException &e)
                                    {
                                        LOG_DEBUG << e.base().what();
                                    };
                        }
                        >>[=](const DrogonDbException &e)
                        {
                            LOG_DEBUG << e.base().what();
                        };
            }

        }
        auto resp = HttpResponse::newRedirectionResponse("/index_retailer");
        callback(resp);
    }
}

void Retailer::orderHistory(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    auto DbPtr = app().getDbClient();
    HttpViewData data;
    vector<unordered_map<string, string>> orders;
    Result r_shop = DbPtr->execSqlSync("select `id` from `shop` where `uid`=?", req->session()->get<string>("uid"));
    assert(r_shop.size() == 1);
    auto shop_id = r_shop[0]["id"].as<string>();
    Result r = DbPtr->execSqlSync(
            "select `date`, `order`.`id`, `uid`, `goods_id`, `num`, `state` from `order` inner join `goods` on `goods`.`id`=`order`.`goods_id`"
            "where `state`>2 and `shop_id`=?"
            "order by `date` desc",
            shop_id
    );
    for(const auto &order : r) {
        orders.emplace_back();
        orders.back()["date"] = order["date"].as<string>();
        orders.back()["id"] = order["id"].as<string>();
        orders.back()["uid"] = order["uid"].as<string>();
        auto goods_id = order["goods_id"].as<string>();
        orders.back()["goods_id"] = goods_id;
        Result goods = DbPtr->execSqlSync("select `name` from `goods` where `id`=?", goods_id);
        assert(goods.size() == 1);
        orders.back()["num"] = order["num"].as<string>();
        auto state = order["state"].as<int>();
        string state_str;
        switch (state) {
            case 0: state_str="未支付";break;
            case 1: state_str="支付成功";break;
            case 2: state_str="物流中";break;
            case 3: state_str="已退款";break;
            case 4: state_str="交易完成";break;
            default:break;
        }
        orders.back()["state_str"] = state_str;
    }
    data.insert("order_data", orders);
    data.insert("user_name", req->session()->get<string>("user_name"));
    data.insert("shop_name", req->session()->get<string>("shop_name"));
    data.insert("user_pic", req->session()->get<string>("user_pic"));
    auto resp = HttpResponse::newHttpViewResponse("order_history.csp", data);
    callback(resp);
}


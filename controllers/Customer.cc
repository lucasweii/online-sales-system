#include "Customer.h"
#include "../MyUtils.h"
#include <fmt/format.h>
using namespace std;
using namespace fmt;
using namespace orm;
//add definition of your processing function here
void Customer::index(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto DbPtr = app().getDbClient();
    Result goods_ret = DbPtr->execSqlSync("select * from `goods`");
    vector<unordered_map<string, string>> goods_data;
    for(auto &goods : goods_ret){
        goods_data.emplace_back();
        goods_data.back()["goods_id"] = to_string(goods["id"].as<int>());
        goods_data.back()["goods_name"] = goods["name"].as<string>();
        double price = goods["price"].as<double>();
        double discount = goods["discount"].as<double>() / 10;
        if(discount == 0) {
            discount = 1;
        }
        price *= discount;
        goods_data.back()["goods_price"] = MyUtils::double2str(price, 2);
        goods_data.back()["goods_stock"] = to_string(goods["stock"].as<int>());
        goods_data.back()["goods_pic"] = goods["pic_path"].as<string>();
        goods_data.back()["goods_category"] = goods["category"].as<string>();
        goods_data.back()["goods_sales"] = to_string(goods["sales_num"].as<int>());
    }

    HttpViewData data;
    data.insert("user_pic", req->session()->get<string>("user_pic"));
    data.insert("user_name", req->session()->get<string>("user_name"));
    data.insert("goods_data", goods_data);
    auto resp = HttpResponse::newHttpViewResponse("index_customer.csp", data);
    callback(resp);
}

void Customer::goodsDetail(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, int id) {
    if(req->method() == Get) {
        auto DbPtr = app().getDbClient();
        Result r = DbPtr->execSqlSync("select * from `goods` where `id` = ?", id);
        assert(r.size() == 1);
        const auto &goods = r[0];
        HttpViewData data;
        data.insert("goods_id", to_string(goods["id"].as<int>()));
        data.insert("goods_name", goods["name"].as<string>());
        data.insert("goods_pic", goods["pic_path"].as<string>());
        data.insert("old_price", MyUtils::double2str(goods["price"].as<double>(),2));
        double price = goods["price"].as<double>();
        double discount = goods["discount"].as<double>() / 10;
        if(discount == 0) {
            discount = 1;
        }
        data.insert("goods_price", MyUtils::double2str(discount * price, 2));
        data.insert("goods_stock", to_string(goods["stock"].as<int>()));
        data.insert("goods_description", goods["description"].as<string>());
        data.insert("sales_num", to_string(goods["sales_num"].as<int>()));
        r = DbPtr->execSqlSync("select * from `shop` where `id`=?", goods["shop_id"].as<int>());
        assert(r.size() == 1);
        data.insert("shop_name", r[0]["name"].as<string>());
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("user_pic", req->session()->get<string>("user_pic"));

        auto resp = HttpResponse::newHttpViewResponse("goods_detail_customer.csp", data);
        callback(resp);
    } else if(req->method() == Post) {
        auto num = req->getParameter("number");
        auto purchase = req->getParameter("purchase");
        auto cart = req->getParameter("cart");
        auto param = req->getParameters();
        for(const auto & it : param) {
            fmt::print("{}:{}\n", it.first, it.second);
        }
        if(!purchase.empty()) {
            auto resp = HttpResponse::newRedirectionResponse("/purchase?id=" + to_string(id) + "&num=" + num);
            callback(resp);
        } else {
            auto resp = HttpResponse::newRedirectionResponse("/add_to_cart?id=" + to_string(id) + "&num=" + num);
            callback(resp);
        }
    }
}

void
Customer::purchase(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, int id, int num) {
    Order order(to_string(id), to_string(num));
    insertOrder(req, order, false);

    vector<Order> orders;
    orders.push_back(order);

    req->session()->insert("orders", orders);
    auto resp = HttpResponse::newRedirectionResponse("/order");
    callback(resp);
}

void
Customer::addToCart(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, int id, int num) {
    auto DbPtr = app().getDbClient();
    *DbPtr  << "select * from `cart` where `goods_id`=? and `uid`=?"
            << id << req->session()->get<string>("uid")
            >> [=](const Result &r)
            {
                if(r.empty()) {
                    *DbPtr  << "insert into `cart` (`uid`, `goods_id`, `num`) values(?,?,?)"
                            << req->session()->get<string>("uid") << id << num
                            >> [=](const Result &r)
                            {
                                LOG_DEBUG << "add goods <id:" << id << ", num:" << num << "> to cart success!";
                            }
                            >> [=](const DrogonDbException &e)
                            {
                                LOG_DEBUG << e.base().what();
                            };
                } else {
                    *DbPtr  << "update `cart` set `num`=`num`+ ? where `goods_id`=? and `uid`=?"
                            << num << id << req->session()->get<string>("uid")
                            >> [=](const Result &r)
                            {
                                LOG_DEBUG << "add goods <id:" << id << ", num:" << num << "> to cart success!";
                            }
                            >> [=](const DrogonDbException &e)
                            {
                                LOG_DEBUG << e.base().what();
                            };
                }
            }
            >> [=](const DrogonDbException &e)
            {
                LOG_DEBUG << e.base().what();
            };
    auto resp = HttpResponse::newRedirectionResponse("/index_customer");
    callback(resp);
}

void Customer::order(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto orders = req->session()->get<vector<Order>>("orders");
        HttpViewData data;
        vector<unordered_map<string, string>> order_data;
        auto DbPtr = app().getDbClient();
        for(auto &order : orders) {
            Result r = DbPtr->execSqlSync(
                    "select `order`.`id` as `id`, `date`, `name`, `price`, `num`, `discount` from "
                    "`order` inner join `goods` on `goods`.`id`=`order`.`goods_id` where `order`.`goods_id`=? and `state`=0",
                    order.goods_id);
            if(!r.empty()) {
                order_data.emplace_back();

                order_data.back()["order_id"] = to_string(r[0]["id"].as<int>());
                order_data.back()["order_date"] = r[0]["date"].as<string>();
                order_data.back()["goods_name"] = r[0]["name"].as<string>();
                double price = r[0]["price"].as<double>();
                double discount = r[0]["discount"].as<double>() / 10;
                int num = r[0]["num"].as<int>();
                if(discount == 0) {
                    discount = 1;
                }
                order_data.back()["goods_price"] =  MyUtils::double2str(price * discount, 2);
                order_data.back()["cost"] = MyUtils::double2str(price * discount * num, 2);
                order_data.back()["num"] = order.num;
                order.order_id = to_string(r[0]["id"].as<int>());
                order.total = order_data.back()["cost"];
            }
        }
        req->session()->modify<vector<Order>>("orders", [&](vector<Order> &old_orders){old_orders = orders;});
        data.insert("order_data", order_data);
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        auto resp = HttpResponse::newHttpViewResponse("ordering.csp", data);
        callback(resp);
    } else if(req->method() == Post) {
        auto DbPtr = app().getDbClient();
        Result r = DbPtr->execSqlSync("select `account` from `user` where`uid`=?", req->session()->get<string>("uid"));
        assert(r.size() == 1);
        auto account = r[0]["account"].as<string>();
        auto orders = req->session()->get<vector<Order>>("orders");
        for(auto &order : orders) {
            if(!order.order_id.empty()) {
                auto txn = app().getDbClient()->newTransaction();
                // update order state
                *txn    << "update `order` set `state`= 1 where `id`=?"
                        << order.order_id
                        >> [=](const Result &r)
                        {
                            LOG_DEBUG << "update order <id:" << order.order_id <<  " > state to 'pay success'";
                        }
                        >> [=](const DrogonDbException &e)
                        {
                            LOG_DEBUG << e.base().what();
                        };
                // pay
                *txn    << "select `account`"
                           "from `user` inner join ( `shop` inner join `goods` on `goods`.`shop_id` = `shop`.`id` ) "
                           "on `user`.`uid`=`shop`.`uid`"
                           "where `goods`.`id`=?"
                       << order.goods_id
                       >> [=](const Result &r)
                       {
                            if(!r.empty()) {
                                auto date = trantor::Date::now();
                                *txn    << "insert into `bill` (`account_out`, `account_in`, `date`, `money`) values (?,?,?,?)"
                                        << account << r[0]["account"].as<string>() << date << order.total
                                        >> [=](const Result &r)
                                        {
                                            LOG_DEBUG << "create bill <id:" << r.insertId() <<  " > success !";
                                        }
                                        >> [=](const DrogonDbException &e)
                                        {
                                            LOG_DEBUG << e.base().what();
                                        };
                            } else {
                                LOG_ERROR << "There should have been an account of goods <id:" << order.goods_id << ">";
                                txn->rollback();
                            }
                       }
                       >> [=](const DrogonDbException &e)
                       {
                            LOG_DEBUG << e.base().what();
                       };
            }
        }
        auto resp = HttpResponse::newRedirectionResponse("/my_order");
        callback(resp);
    }
}

void Customer::cart(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto DbPtr = app().getDbClient();
        Result goods_ret = DbPtr->execSqlSync(
                "select * from `cart` inner join `goods` on `goods`.`id` = `cart`.`goods_id` where `uid`=?",
                req->session()->get<string>("uid")
                        );

        vector<unordered_map<string, string>> goods_data;
        HttpViewData data;

        for(auto &goods : goods_ret){
            goods_data.emplace_back();
            goods_data.back()["goods_id"] = to_string(goods["goods_id"].as<int>());
            goods_data.back()["num"] = to_string(goods["num"].as<int>());
            goods_data.back()["goods_name"] = goods["name"].as<string>();
            double price = goods["price"].as<double>();
            double discount = goods["discount"].as<double>() / 10;
            if(discount == 0) {
                discount = 1;
            }
            price *= discount;
            goods_data.back()["goods_price"] = MyUtils::double2str(price, 2);
            goods_data.back()["goods_pic"] = goods["pic_path"].as<string>();
        }
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        data.insert("goods_data", goods_data);
        auto resp = HttpResponse::newHttpViewResponse("shopping_cart.csp", data);
        callback(resp);
    } else if(req->method() == Post) {
        auto param = req->getParameters();
        vector<Order> orders;
        unordered_map<string, string> items;

        // get goods_id
        for(const auto &it : param) {
            auto temp = MyUtils::split(it.first, "@");
            if(temp[0] == "select_item"){
                items[temp[1]];
            }
        }

        // get num
        for(const auto &it : param) {
            auto temp = MyUtils::split(it.first, "@");
            if(temp[0] == "num" && items.count(temp[1])){
                items[temp[1]] = it.second;
            }
        }

        // add to orders
        orders.reserve(items.size());
        for(const auto &it : items) {
            orders.emplace_back(it.first, it.second);
        }

        auto DbPtr = app().getDbClient();
        for(const auto &order : orders) {
            insertOrder(req, order, true);
        }
        req->session()->insert("orders", orders);
        auto resp = HttpResponse::newRedirectionResponse("/order");
        callback(resp);
    }
}

void Customer::cartManage(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto DbPtr = app().getDbClient();
        Result goods_ret = DbPtr->execSqlSync(
                "select * from `cart` inner join `goods` on `goods`.`id` = `cart`.`goods_id` where `uid`=?",
                req->session()->get<string>("uid")
        );

        vector<unordered_map<string, string>> goods_data;
        HttpViewData data;

        for(auto &goods : goods_ret){
            goods_data.emplace_back();
            goods_data.back()["goods_id"] = to_string(goods["goods_id"].as<int>());
            goods_data.back()["num"] = to_string(goods["num"].as<int>());
            goods_data.back()["goods_name"] = goods["name"].as<string>();
            double price = goods["price"].as<double>();
            double discount = goods["discount"].as<double>() / 10;
            if(discount == 0) {
                discount = 1;
            }
            price *= discount;
            goods_data.back()["goods_price"] = MyUtils::double2str(price, 2);
            goods_data.back()["goods_pic"] = goods["pic_path"].as<string>();
        }
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        data.insert("goods_data", goods_data);
        auto resp = HttpResponse::newHttpViewResponse("shopping_cart_del.csp", data);
        callback(resp);
    } else if(req->method() == Post) {
        auto param = req->getParameters();
        auto DbPtr = app().getDbClient();
        for(const auto &it : param) {
            auto goods_id = MyUtils::split(it.first, "@")[1];
            *DbPtr  << "delete from `cart` where `uid`=? and `goods_id`=?"
                    << req->session()->get<string>("uid") << goods_id
                    >> [=](const Result &r)
                    {
                        LOG_DEBUG << "delete goods <id: " << goods_id << "> success!";
                    }
                    >> [=](const DrogonDbException &e)
                    {
                        LOG_DEBUG << e.base().what();
                        LOG_DEBUG << "delete goods <id: " << goods_id << "> failed!";
                    };
        }
        auto resp = HttpResponse::newRedirectionResponse("/my_cart");
        callback(resp);
    }
}

void Customer::insertOrder(const HttpRequestPtr &req, const Customer::Order &order, bool isCart) {
    auto txn = app().getDbClient()->newTransaction();
    *txn    << "select `stock` from `goods` where `id`=?"
            << order.goods_id
            >> [=](const Result &r)
            {
                if(r.empty()) {
                    LOG_DEBUG << "the goods <id:" << order.goods_id <<" > have been taken off the shelves";
                } else {
                    assert(r.size() == 1);
                    auto stock = r[0]["stock"].as<int>();
                    auto num = MyUtils::str2int(order.num);
                    if(num > stock) {
                        LOG_DEBUG << "there are not so many products";
                    } else {
                        *txn    << "update `goods` set `stock`=`stock`-? where `id`=?"
                                << num << order.goods_id
                                >> [=](const Result &r)
                                {
                                    LOG_DEBUG << "lock goods <id:"<< order.goods_id<<", num:"<<order.num<<"> success";
                                }
                                >> [=](const DrogonDbException &e)
                                {
                                    LOG_DEBUG << "lock goods <id:"<< order.goods_id<<", num:"<<order.num<< "> failed";
                                    LOG_DEBUG << e.base().what();
                                };

                        auto date = trantor::Date::now().toDbStringLocal();
                        *txn    << "insert into `order` (`uid`, `goods_id`, `num`, `date`) values (?,?,?,?)"
                                << req->session()->get<string>("uid") << order.goods_id << order.num << date
                                >> [=](const Result &r)
                                {
                                    thread(orderRoutine, r.insertId()).detach();
                                    LOG_DEBUG << "create new order <id:"<<r.insertId()<<">";
                                }
                                >> [=](const DrogonDbException &e)
                                {
                                    LOG_DEBUG << e.base().what();
                                };

                        if(isCart) {
                            // delete this cart item
                            *txn    << "delete from `cart` where `uid`=? and `goods_id`=?"
                                    << req->session()->get<string>("uid") << order.goods_id
                                    >> [=](const Result &r)
                                    {
                                        LOG_DEBUG << "delete cart item <user:" << req->session()->get<string>("user_name")
                                                  << ", goods_id:" << order.goods_id << "> !";
                                    }
                                    >> [=](const DrogonDbException &e)
                                    {
                                        LOG_DEBUG << e.base().what();
                                    };
                        }
                    }
                }
            }
            >> [=](const DrogonDbException &e)
            {
                LOG_DEBUG << e.base().what();
            };
}

void Customer::myOrder(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto DbPtr = app().getDbClient();
        HttpViewData data;
        vector<unordered_map<string, string>> new_orders, old_orders;

        Result r = DbPtr->execSqlSync("select * from `order` where `uid`=? order by `date` desc", req->session()->get<string>("uid"));
        for(const auto &order : r) {
            vector<unordered_map<string, string>> *orders = &new_orders;
            if(order["state"].as<int>() >= 3) {
                orders = &old_orders;
            }

            orders->emplace_back();
            orders->back()["order_date"] = order["date"].as<string>();
            orders->back()["order_id"] = order["id"].as<string>();
            auto goods_id = order["goods_id"].as<string>();
            orders->back()["goods_id"] = goods_id;
            Result goods = DbPtr->execSqlSync("select `name` from `goods` where `id`=?", goods_id);
            assert(goods.size() == 1);
            orders->back()["goods_name"] = goods[0]["name"].as<string>();
            orders->back()["num"] = order["num"].as<string>();
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
            orders->back()["state_str"] = state_str;
        }
        data.insert("new_order_data", new_orders);
        data.insert("old_order_data", old_orders);
        data.insert("user_name", req->session()->get<string>("user_name"));
        data.insert("user_pic", req->session()->get<string>("user_pic"));
        auto resp = HttpResponse::newHttpViewResponse("my_order.csp", data);
        callback(resp);
    } else if(req->method() == Post) {
        auto param = req->getParameters();
        auto DbPtr = app().getDbClient();
        Result r = DbPtr->execSqlSync("select `account` from `user` where`uid`=?", req->session()->get<string>("uid"));
        assert(r.size() == 1);
        auto account = r[0]["account"].as<string>();
        for(auto &it : param) {
            auto id = MyUtils::split(it.first, "@")[1];
            if(it.second == "acc") {
                *DbPtr  << "update `order` set `state` = 4 where `id`=?"
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
                                        auto goods_id = r[0]["goods_id"].as<string>();
                                        auto num = r[0]["num"].as<int>();
                                        auto uid = req->session()->get<string>("uid");
                                        *txn    << "select `account`, `price`, `discount`"
                                                   "from `user` inner join ( `shop` inner join `goods` on `goods`.`shop_id` = `shop`.`id` ) "
                                                   "on `user`.`uid`=`shop`.`uid`"
                                                   "where `goods`.`id`=?"
                                                << goods_id
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
                                                                << r[0]["account"].as<string>() << account << date << total
                                                                >> [=](const Result &r)
                                                                {
                                                                    LOG_DEBUG << "create bill <id:" << r.insertId() <<  " > success !";
                                                                }
                                                                >> [=](const DrogonDbException &e)
                                                                {
                                                                    LOG_DEBUG << e.base().what();
                                                                };
                                                    } else {
                                                        LOG_ERROR << "There should have been an account of goods <id:" << goods_id << ">";
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
        auto resp = HttpResponse::newRedirectionResponse("/index_customer");
        callback(resp);
    }
}

void Customer::rePay(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback, std::string id) {
    auto DbPtr = app().getDbClient();
    Result r = DbPtr->execSqlSync("select * from `order` where `id`=?", id);
    assert(r.size() == 1);
    Order order(r[0]["goods_id"].as<string>(), r[0]["num"].as<string>());
    order.order_id = id;
    r = DbPtr->execSqlSync("select * from `goods` where `id`=?",order.goods_id);
    assert(r.size() == 1);
    auto price = r[0]["price"].as<double>();
    auto discount = r[0]["discount"].as<double>() / 10;
    if(discount == 0){
        discount = 1.0;
    }
    order.total = MyUtils::double2str(price * discount * MyUtils::str2int(order.num), 2);

    vector<Order> orders;
    orders.push_back(order);

    req->session()->insert("orders", orders);
    auto resp = HttpResponse::newRedirectionResponse("/order");
    callback(resp);
}

void Customer::searchByName(const HttpRequestPtr &req, function<void(const HttpResponsePtr &)> &&callback) {
    auto name = req->getParameter("search");
    auto DbPtr = app().getDbClient();
    Result r = DbPtr->execSqlSync("select * from goods where `name` like '%" + name + "%'");
    HttpViewData data;
    vector<unordered_map<string, string>> goods_data;
    for(auto &goods : r){
        goods_data.emplace_back();
        goods_data.back()["goods_id"] = to_string(goods["id"].as<int>());
        goods_data.back()["goods_name"] = goods["name"].as<string>();
        double price = goods["price"].as<double>();
        double discount = goods["discount"].as<double>() / 10;
        if(discount == 0) {
            discount = 1;
        }
        price *= discount;
        goods_data.back()["goods_price"] = MyUtils::double2str(price, 2);
        goods_data.back()["goods_stock"] = to_string(goods["stock"].as<int>());
        goods_data.back()["goods_pic"] = goods["pic_path"].as<string>();
        goods_data.back()["goods_category"] = goods["category"].as<string>();
        goods_data.back()["goods_sales"] = to_string(goods["sales_num"].as<int>());
    }
    data.insert("goods_data", goods_data);
    data.insert("user_pic", req->session()->get<string>("user_pic"));
    data.insert("user_name", req->session()->get<string>("user_name"));
    auto resp = HttpResponse::newHttpViewResponse("index_customer.csp", data);
    callback(resp);
}


void orderRoutine(int order_id) {
    std::chrono::seconds time(60);
    this_thread::sleep_for(time);
    {
        auto txn = app().getDbClient()->newTransaction();
        *txn    << "select * from `order` where `id`=? and `state`=0"
                << order_id
                >> [=](const Result &r)
                {
                    if(!r.empty()) {
                        auto goods_id = r[0]["goods_id"].as<int>();
                        auto num = r[0]["num"].as<int>();

                        // delete timeout order
                         *txn   << "delete from `order` where `id`=?"
                                << order_id
                                >> [=](const Result &r)
                                {
                                    LOG_DEBUG << "timeout of unpaid order <id :" << order_id << ">";
                                }
                                >> [=](const DrogonDbException &e)
                                {
                                    LOG_DEBUG << e.base().what();
                                };

                         // unlock goods stock
                         *txn   << "update `goods` set `stock`=`stock`+? where `id`=?"
                                << num << goods_id
                                >> [=](const Result &r)
                                {
                                    LOG_DEBUG << "recover goods stock of goods <id:" << goods_id << ", num:" << num << ">";
                                }
                                >> [=](const DrogonDbException &e)
                                {
                                    LOG_DEBUG << e.base().what();
                                };
                    }
                }
                >> [=](const DrogonDbException &e)
                {
                    LOG_DEBUG << e.base().what();
                };
    }
}
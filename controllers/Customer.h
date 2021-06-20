#pragma once

#include <drogon/HttpController.h>

#include <utility>

using namespace drogon;

class Customer : public drogon::HttpController<Customer> {
public:
    METHOD_LIST_BEGIN
        //use METHOD_ADD to add your custom processing function here;
        //METHOD_ADD(Customer::get,"/{2}/{1}",Get);//path is /Customer/{arg2}/{arg1}
        //METHOD_ADD(Customer::your_method_name,"/{1}/{2}/list",Get);//path is /Customer/{arg1}/{arg2}/list
        //ADD_METHOD_TO(Customer::your_method_name,"/absolute/path/{1}/{2}/list",Get);//path is /absolute/path/{arg1}/{arg2}/list
        ADD_METHOD_TO(Customer::index, "/index_customer", Get, "Log", "SessionControl", "RemoveOrders");
        ADD_METHOD_TO(Customer::goodsDetail, "/goods_detail?id={}", Get, Post, "Log", "SessionControl");
        ADD_METHOD_TO(Customer::purchase, "/purchase?id={}&num={}", Get, "Log", "SessionControl");
        ADD_METHOD_TO(Customer::addToCart, "/add_to_cart?id={}&num={}", Get, "Log", "SessionControl");
        ADD_METHOD_TO(Customer::order, "/order", Get, Post, "Log", "SessionControl");
        ADD_METHOD_TO(Customer::cart, "/my_cart", Get, Post, "Log", "SessionControl", "RemoveOrders");
        ADD_METHOD_TO(Customer::cartManage, "/my_cart_manage", Get, Post, "Log", "SessionControl");
    METHOD_LIST_END

    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req,std::function<void (const HttpResponsePtr &)> &&callback,int p1,std::string p2);
    // void your_method_name(const HttpRequestPtr& req,std::function<void (const HttpResponsePtr &)> &&callback,double p1,int p2) const;
    void index(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    void goodsDetail(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int id);

    void purchase(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int id, int num);

    void addToCart(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, int id, int num);

    void order(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    void cart(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);

    void cartManage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);


private:
    struct Order {
        std::string goods_id;
        std::string num;
        std::string order_id;
        std::string total;
        Order(std::string goods_id, std::string num):goods_id(std::move(goods_id)), num(std::move(num)){}
        Order() = default;
    };

    void insertOrder(const HttpRequestPtr &req, const Order &order, bool isCart);
};

void orderRoutine(int order_id);
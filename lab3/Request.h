#pragma once

#include <vector>

#define INVENTORY_SIZE 100

#define MAX_BUY_ITEM	    8
#define MAX_QUANTITY	    100
#define MAX_BUDGET	    3000000
#define MIN_BUDGET          5000
#define MAX_PRICE	    1000000
#define MAX_SHIPPING_COST   10000

// Forward declaration. Do not remove!!
class EStore;

enum SupplierRequestTypes {
    ADD_ITEM = 0,
    REMOVE_ITEM,
    ADD_STOCK,
    CHANGE_ITEM_PRICE,
    CHANGE_ITEM_DISCOUNT,
    SET_SHIPPING_COST,
    SET_STORE_DISCOUNT,
    NUM_SUPPLIER_REQUEST_TYPES
};

struct AddItemReq
{
    EStore* store;

    int item_id;
    int quantity;
    double price;
    double discount;
};

struct RemoveItemReq
{
    EStore* store;

    int item_id;
};

struct AddStockReq
{
    EStore* store;

    int item_id;
    int additional_stock;
};

struct ChangeItemPriceReq
{
    EStore* store;

    int item_id;
    double new_price;
};

struct ChangeItemDiscountReq
{
    EStore* store;

    int item_id;
    double new_discount;
};

struct SetShippingCostReq
{
    EStore* store;

    double new_cost;
};

struct SetStoreDiscountReq
{
    EStore* store;

    double new_discount;
};

struct BuyItemReq
{
    EStore* store;

    int item_id;
    double budget;
};

struct BuyManyItemsReq
{
    EStore* store;

    std::vector<int> item_ids;
    double budget;
};


#include <iostream>
#include <cstdlib>
#include <cassert>
#include <set>

#include "RequestHandlers.h"
#include "RequestGenerator.h"

using namespace std;

static int
rand_id()
{
    return sutil_random() % INVENTORY_SIZE;
}

static int
rand_quantity()
{
    return (sutil_random() % MAX_QUANTITY) + 1;
}

static double
rand_price(int max_price_cents)
{
    return (sutil_random() % max_price_cents) / 100.0;
}

static double
rand_discount()
{
    return ((double) sutil_random() / RAND_MAX);
}

static int
rand_request()
{
    return sutil_random() % NUM_SUPPLIER_REQUEST_TYPES;
}

RequestGenerator::
RequestGenerator(TaskQueue* queue)
    : taskQueue(queue), taskCount(0)
{
}

RequestGenerator::
~RequestGenerator()
{
}

void RequestGenerator::
enqueueTasks(int maxTasks, EStore* store)
{
    taskCount = 0;
    while (taskCount < maxTasks || maxTasks < 0)
    {
        taskQueue->enqueue(generateTask(store));
        taskCount++;
        sthread_sleep(0, 100000000);
    }
}

/*
 * ------------------------------------------------------------------
 * enqueueStops --
 *
 *      Enqueue "num" stop requests (i.e. one per worker thread) into
 *      the task queue associated with this request generator.
 *
 *      Hint: Use the stop_handler function declared in
 *      RequestHandlers.h in conjunction with the task queue to
 *      create the stop requests.
 *
 * Results:
 *      Does not return a value.
 *
 * ------------------------------------------------------------------
 */
void RequestGenerator::
enqueueStops(int num)
{
    // TODO: Your code here.
}

SupplierRequestGenerator::
SupplierRequestGenerator(TaskQueue* queue)
    : RequestGenerator(queue)
{ }

Task SupplierRequestGenerator::
generateTask(EStore* store)
{
    Task task;

    // generate a random number to determine the request_type
    // first 30 requests are ADD_ITEM to fill in the store
    int request_type;

    if(taskCount < 30)
        request_type = ADD_ITEM;
    else
        request_type = rand_request();

    switch(request_type)
    {
        case ADD_ITEM:
        {
            AddItemReq* req = new AddItemReq();
            req->store = store;
            req->item_id   = rand_id();
            req->price     = rand_price(MAX_PRICE) + 1;
            req->quantity  = rand_quantity();

            task.handler = add_item_handler;
            task.arg = req;
            break;
        }
        case REMOVE_ITEM:
        {
            RemoveItemReq* req = new RemoveItemReq();
            req->store = store;
            req->item_id   = rand_id();

            task.handler = remove_item_handler;
            task.arg = req;
            break;
        }
        case ADD_STOCK:
        {
            AddStockReq* req = new AddStockReq();
            req->store        = store;
            req->item_id          = rand_id();
            req->additional_stock = rand_quantity();

            task.handler = add_stock_handler;
            task.arg = req;
            break;
        }
        case CHANGE_ITEM_PRICE:
        {
            ChangeItemPriceReq* req = new ChangeItemPriceReq();
            req->store = store;
            req->item_id    = rand_id();
            req->new_price  = rand_price(MAX_PRICE);

            task.handler = change_item_price_handler;
            task.arg = req;
            break;
        }
        case CHANGE_ITEM_DISCOUNT:
        {
            ChangeItemDiscountReq* req = new ChangeItemDiscountReq();
            req->store = store;
            req->item_id       = rand_id();
            req->new_discount  = rand_discount();

            task.handler = change_item_discount_handler;
            task.arg = req;
            break;
        }
        case SET_SHIPPING_COST:
        {
            SetShippingCostReq* req = new SetShippingCostReq();
            req->store = store;
            req->new_cost  = rand_price(MAX_SHIPPING_COST);

            task.handler = set_shipping_cost_handler;
            task.arg = req;
            break;
        }
        case SET_STORE_DISCOUNT:
        {
            SetStoreDiscountReq* req = new SetStoreDiscountReq();
            req->store    = store;
            req->new_discount = rand_discount();

            task.handler = set_store_discount_handler;
            task.arg = req;
            break;
        }
        default:
        {
            cerr << "Request type is undefined. Can not generate this type of requests." << endl;
            assert(false);
            break;
        }
    } // !switch

    return task;
}

CustomerRequestGenerator::
CustomerRequestGenerator(TaskQueue* queue, bool inFineMode)
    : RequestGenerator(queue), fineMode(inFineMode)
{ }

Task CustomerRequestGenerator::
generateTask(EStore* store)
{
    Task task;

    if (!fineMode)
    {
        BuyItemReq* req = new BuyItemReq();
        req->store = store;
        req->item_id   = rand_id();
        req->budget    = rand_price(MAX_BUDGET) + MIN_BUDGET;

        task.handler = buy_item_handler;
        task.arg = req;
    }
    else
    {
        BuyManyItemsReq* req = new BuyManyItemsReq();

        int num_buy_item = (sutil_random() % MAX_BUY_ITEM) + 1;

        set<int> order;
        for(int i = 0; i < num_buy_item; i++)
            order.insert(rand_id());

        req->store = store;
        req->item_ids.insert(req->item_ids.begin(), order.begin(), order.end());
        req->budget = rand_price(MAX_BUDGET) + MIN_BUDGET;;

        task.handler = buy_many_items_handler;
        task.arg = req;
    }
    return task;
}


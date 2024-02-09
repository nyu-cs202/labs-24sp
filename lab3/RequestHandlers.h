#pragma once

void add_item_handler(void *args);
void remove_item_handler(void *args);
void add_stock_handler(void *args);
void change_item_price_handler(void *args);
void change_item_discount_handler(void *args);
void set_shipping_cost_handler(void *args);
void set_store_discount_handler(void *args);

void buy_item_handler(void *args);
void buy_many_items_handler(void *args);

void stop_handler(void *args);

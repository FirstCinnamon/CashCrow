#include <iostream>
#include "pqxx/pqxx"
#include "db.hpp"

db::DBConnection con("localhost", "postgres", "postgres", "PASSWORD");

void doAccountTest() {
    auto id = con.selectIdFromAccountSecurity("a");
    auto ret = con.selectFromAccountInfo(1);
}

void doAddTradeHistoryTest() {
    con.insertTradeHistory("abcd", 10000, 1);
}

void doUpsertStockTest() {
    con.upsertOwnedStock(1, "abc", 100);
    con.upsertOwnedStock(1, "abc", 100);
}

int main(void) {
    doUpsertStockTest();
    //db::DBConnection con("host=localhost user=postgres dbname=postgres password=PASSWORD port=5432 connect_timeout=10");
    //con.insertAccount("a", "b", "c");
    //auto id = con.selectIdFromAccountSecurity("a");
    //auto ret = con.selectFromAccountInfo(1);
    //auto ret = con.selectFromAccountSecurity("a");
    //con.insertAccount("abc", "dcv", "asd");
}

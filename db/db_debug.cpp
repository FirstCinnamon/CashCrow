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

void doSelectFromOwnedStock() {
    auto dict= con.selectFromOwnedStock(1);
    auto abc = dict["abc"];
}

void doReduceOwnedStockTest() {
    con.upsertOwnedStock(1, "abc", -10);
}

void doChangeAccountTest() {
    con.changeAccount(1, -30.5);
}

void doDeleteSession() {
    con.deleteSession(1);
}

void doSidToUidTest() {
    if (con.tryInsertSession(1))
    {
        auto uid = con.sidToUid(1);
    }
}

void doS() {
    if (con.tryInsertSession(1))
    {
        auto uid = con.sidToUid(1);
    }
}

int main(void) {
    con.changeAccount(1, -30.5);
    //db::DBConnection con("host=localhost user=postgres dbname=postgres password=PASSWORD port=5432 connect_timeout=10");
    //con.insertAccount("a", "b", "c");
    //auto id = con.selectIdFromAccountSecurity("a");
    //auto ret = con.selectFromAccountInfo(1);
    //auto ret = con.selectFromAccountSecurity("a");
    //con.insertAccount("abc", "dcv", "asd");
}

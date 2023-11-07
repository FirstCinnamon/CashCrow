#include <iostream>
#include "pqxx/pqxx"
#include "db.hpp"

db::DBConnection con("localhost", "postgres", "crow", "1234");

void doAccountTest() {
    con.insertAccount("1", "1", "1");
    con.insertAccount("2", "2", "2");
    con.insertAccount("3", "3", "3");
    // auto id = con.selectIdFromAccountSecurity("\" OR 1=1");
    std::cout << con.selectIdFromAccountSecurity("1");
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

void doS() {
    auto a = con.getMaxSid();
}

void doBankAccountTest() {
    //con.insertBankAccount(1, "cda", 100);
    //auto vec = con.selectFromBankAccount(1);
    con.increaseBankAccount(2, -30.5);
}

int main(void) {
    // db::DBConnection con("host=localhost user=postgres dbname=crow password=1234 port=5432 connect_timeout=10");
    doS();
    // con.insertAccount("a", "b", "c");
    //auto id = con.selectIdFromAccountSecurity("a");
    //auto ret = con.selectFromAccountInfo(1);
    //auto ret = con.selectFromAccountSecurity("a");
    //con.insertAccount("abc", "dcv", "asd");
}

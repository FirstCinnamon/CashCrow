#pragma once
#include <pqxx/pqxx>
#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <pqxx/except.hxx>

#include "../rand.hpp"

#define SID_LEN 32

std::vector<std::tuple<std::string, int, float>> jointProduct(
    const std::map<std::string, int>& map1, const std::map<std::string, float>& map2) {

    std::vector<std::tuple<std::string, int, float>> result;

    for (const auto& pair1 : map1) {
        const std::string& key1 = pair1.first;
        int value1 = pair1.second;

        auto it2 = map2.find(key1);
        if (it2 != map2.end()) {
            float value2 = it2->second;
            result.push_back(std::make_tuple(key1, value1, value2));
        }
    }

    return result;
}

namespace db
{

    struct AccountPassword {
        std::string salt;
        std::string hash;
    };

    struct BankAccount {
        int id;
        std::string bank_name;
        float balance;
    };

    class DBConnection {
    public:
        DBConnection(const std::string& host) {
            init(host);
        }
        DBConnection(const std::string& host,
            const std::string& user,
            const std::string& dbname,
            const std::string& password) {
            auto formatted = string_format(
                "host=%s user=%s dbname=%s password=%s port=5432 connect_timeout=10",
                host.c_str(), user.c_str(), dbname.c_str(), password.c_str());
            init(formatted);
        }

#pragma region Login

        void insertAccount(const std::string& username, const std::string& salt, const std::string& hash) {
            pqxx::work w(*c);
            pqxx::params param(username, salt, hash);
            pqxx::result result = w.exec_prepared("insert_account", param);
            w.commit();
        }

        int selectIdFromAccountSecurity(const std::string& username) {
            pqxx::work w(*c);
            pqxx::params param(username);
            pqxx::row r = w.exec_prepared1("select_from_account_security", username);
            int ret = r["id"].as<int>();
            w.commit();
            return ret;
        }

        AccountPassword selectFromAccountSecurity(const std::string& username) {
            pqxx::work w(*c);
            pqxx::params param(username);
            pqxx::result result = w.exec_prepared("select_from_account_security", username);
            if (result.empty()) {
                return AccountPassword{};
            }
            w.commit();
            return AccountPassword{ result[0]["salt"].as<std::string>(), result[0]["hash"].as<std::string>() };
        }

        void updatePassword(int uid, const std::string& salt, const std::string& hash) {
            pqxx::work w(*c);
            pqxx::params param(uid, salt, hash);
            pqxx::result result = w.exec_prepared("update_password", param);
            w.commit();
        }

        void tryInsertSession(int uid) {
            pqxx::work w(*c);
            pqxx::params param(uid);
            pqxx::row row = w.exec_prepared1("exist_session_already", param);

            if (!row[0].as<bool>()) {
                std::string sid{gen_rand_str(SID_LEN)};
                pqxx::result result{};
                do {
                    pqxx::params param(sid);
                    result = w.exec_prepared("is_valid_sid", param);
                    w.commit();
                } while (result.size() == 1);

                pqxx::params param(sid, uid);
                w.exec_prepared("insert_session", param);
            }
            w.commit();
        }

        void deleteSession(int uid) {
            pqxx::work w(*c);
            pqxx::params param(uid);
            w.exec_prepared("delete_session", param);
            w.commit();
        }

        int sidToUid(std::string sid) {
            pqxx::work w(*c);
            pqxx::params param(sid);
            pqxx::row row = w.exec_prepared1("sid_to_uid", param);
            int ret = row[0].as<int>();
            w.commit();
            return ret;
        }

        std::string uidToSid(const int& uid)
        {
            pqxx::work w(*c);
            pqxx::params param(uid);
            pqxx::row row{w.exec_prepared1("uid_to_sid", param)};
            w.commit();
            return row[0].as<std::string>();
        }

        bool isValidSid(const std::string& sid)
        {
            pqxx::work w(*c);
            pqxx::params param(sid);
            pqxx::result result{w.exec_prepared("is_valid_sid", param)};
            w.commit();
            return result.size() == 1;
        }

#pragma endregion
        float selectFromAccountInfo(int id) {
            pqxx::work w(*c);
            pqxx::params param(id);
            pqxx::row r = w.exec_prepared1("select_from_account_info", param);
            float ret = r[1].as<float>();
            w.commit();
            return ret;
        }

        std::map<std::string, int> selectFromOwnedStock(int ownerId) {
            pqxx::work w(*c);
            pqxx::params param(ownerId);
            pqxx::result result = w.exec_prepared("select_from_owned_stock", param);

            std::map<std::string, int> ret;
            for (const auto& row : result) {
                std::string name = row["name"].as<std::string>();
                int num = row["num"].as<int>();
                ret[name] = num;
            }
            w.commit();
            return ret;
        }

        std::map<std::string, float> selectFromAvgPrice(int ownerId) {
            pqxx::work w(*c);
            pqxx::params param(ownerId);
            pqxx::result result = w.exec_prepared("select_from_avg_price", param);

            std::map<std::string, float> ret;
            for (const auto& row : result) {
                std::string name = row["product"].as<std::string>();
                int num = row["price_avg"].as<float>();
                ret[name] = num;
            }
            w.commit();
            return ret;
        }

        std::vector<BankAccount> selectFromBankAccount(int id) {
            pqxx::work w(*c);
            pqxx::params param(id);
            pqxx::result result = w.exec_prepared("select_from_bank_account", param);
            std::vector<BankAccount> bankAccounts;
            for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                BankAccount account;
                account.id = row["id"].as<int>();
                account.bank_name = row["bank_name"].as<std::string>();
                account.balance = row["balance"].as<float>();

                bankAccounts.push_back(account);
            }
            w.commit();
            return bankAccounts;
        }

        void increaseBankAccount(int id, float balance) {
            pqxx::work w(*c);
            pqxx::params param(id, balance);
            pqxx::result result = w.exec_prepared("increase_bank_account", param);
            w.commit();
        }

        void insertBankAccount(int owner_id, const std::string& bank_name, float balance)
        {
            pqxx::work w(*c);
            pqxx::params param(owner_id, bank_name, balance);
            pqxx::result result = w.exec_prepared("insert_bank_account", param);
            w.commit();
        }

        void insertTradeHistory(const std::string& product, float price, int buyer_id) {
            pqxx::work w(*c);
            pqxx::params param(product, price, buyer_id);
            pqxx::result result = w.exec_prepared("insert_trade_history", param);
            w.commit();
        }

        void upsertOwnedStock(int owner_id, const std::string& name, int num) {
            pqxx::work w(*c);
            pqxx::params param(owner_id, name, num);
            pqxx::result result = w.exec_prepared("upsert_owned_stock", param);
            w.commit();
        }

        void upsertAvgPrice(int owner_id, const std::string& name, float avg_price) {
            pqxx::work w(*c);
            pqxx::params param(owner_id, name, avg_price);
            pqxx::result result = w.exec_prepared("upsert_avg_price", param);
            w.commit();
        }

        void changeAccount(int owner_id, float num) {
            pqxx::work w(*c);
            pqxx::params param(owner_id, num);
            pqxx::result result = w.exec_prepared("increase_account", param);
            w.commit();
        }

        ~DBConnection() {
            a.store(true);
            workerThread.join();
            delete c;
        }
    private:
        pqxx::connection* c;
        std::atomic_bool a;
        std::thread workerThread;

        void init(const std::string& options)
        {
            c = new pqxx::connection(options);
            if (c->is_open()) {
                std::cout << "Opened database successfully: " << c->dbname() << std::endl;
            }
            else {
                std::cerr << "Failed to open database" << std::endl;
                return;
            }
            prepare();

            a.store(false);
            auto cs = new pqxx::connection(options);
            setInterval(a, static_cast<size_t>(1000) , [this, cs]() {deleteIfExpiryOver(cs); });
        }

        void deleteIfExpiryOver(pqxx::connection* con) {
            pqxx::work w(*con);
            w.exec("DELETE FROM session WHERE expiry < NOW();");
            w.commit();
        }

        template <class F, class... Args>
        void setInterval(std::atomic_bool& cancelToken, size_t interval, F&& f, Args&&... args) {
            cancelToken.store(true);
            auto cb = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            workerThread = std::thread([=, &cancelToken]()mutable {

                while (cancelToken.load()) {
                    cb();
                    std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                }
                });
        }

        template<typename ... Args>
        std::string string_format(const std::string& format, Args ... args)
        {
            size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
            if (size <= 0) { throw std::runtime_error("Error during formatting."); }
            std::unique_ptr<char[]> buf(new char[size]);
            snprintf(buf.get(), size, format.c_str(), args ...);
            return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
        }



        void prepare() {
            //account
            pqxx::work w(*c);
            w.exec("PREPARE insert_account(varchar(20), char(20), char(64)) AS INSERT INTO account_security(username, salt, hash) VALUES($1, $2, $3);");
            w.exec("PREPARE select_from_account_info AS SELECT * FROM account_info WHERE id = $1;");
            w.exec("PREPARE update_password(int, char(20), char(64)) AS UPDATE account_security SET salt = $2, hash = $3 WHERE id = $1;");

            w.exec(R"(
PREPARE increase_account (int, float) AS
UPDATE account_info SET account_balance = account_balance + $2
WHERE id = $1;
)");

            w.exec("PREPARE select_from_account_security(varchar(20)) AS SELECT * FROM account_security WHERE username = $1;");

            w.exec("PREPARE insert_session(char(32), int) AS INSERT INTO session(sid, uid, expiry) VALUES($1, $2, NOW() + INTERVAL '1 hour');");
            w.exec(R"(
PREPARE exist_session_already(int) AS
SELECT COUNT(*) > 0
FROM session
WHERE uid = $1;
)");

            w.exec("PREPARE uid_to_sid(int) AS SELECT sid FROM session WHERE uid = $1;");

            w.exec("PREPARE is_valid_sid(char(32)) AS SELECT * FROM session WHERE sid = $1;");

            w.exec(R"(
PREPARE delete_session(int) AS DELETE FROM session WHERE uid = $1
)");
            w.exec("PREPARE sid_to_uid(char(32)) AS SELECT uid FROM session WHERE sid = $1;");

            //bank_account
            w.exec("PREPARE select_from_bank_account(int) AS SELECT * FROM bank_account WHERE owner_id = $1;");
            w.exec(R"(PREPARE increase_bank_account(int, float4) AS
UPDATE bank_account SET balance = balance + $2
WHERE id = $1)");
            w.exec(R"(
PREPARE insert_bank_account(int, varchar(20), float) AS
INSERT INTO bank_account(owner_id, bank_name, balance) VALUES($1, $2, $3);
)");

            //trade_history
            w.exec("PREPARE select_from_trade_history(int) AS SELECT * FROM trade_history WHERE id = $1;");
            w.exec(R"(
PREPARE insert_trade_history(varchar(20), float4, int) AS
INSERT INTO trade_history (product, time_traded, price, buyer_id) VALUES($1, CURRENT_TIMESTAMP, $2, $3);
)");

            //owned_stock
            w.exec(R"(
PREPARE select_from_owned_stock(int) AS
SELECT name, num FROM owned_stock WHERE owner_id = $1;
)");
            w.exec(R"(
PREPARE upsert_owned_stock (int, varchar(20), int) AS
INSERT INTO owned_stock (owner_id, name, num)
VALUES ($1, $2, $3)
ON CONFLICT (owner_id, name)
DO UPDATE SET num = owned_stock.num + $3;
)");

            // avg price
            w.exec(R"(
PREPARE select_from_avg_price(int) AS
SELECT product, price_avg FROM avg_price WHERE buyer_id = $1;
)");

            w.exec(R"(
PREPARE upsert_avg_price (int, varchar(20), float) AS
INSERT INTO avg_price (product, price_avg, buyer_id)
VALUES ($2, $3, $1)
ON CONFLICT (product, buyer_id)
DO UPDATE SET price_avg = $3;
)");

            w.exec(R"(
PREPARE reduce_owned_stock(int, varchar, int) AS
UPDATE owned_stock
SET num = num - $3
WHERE owner_id = $1 AND name = $2;
)");
            w.commit();
        }
    };

}

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
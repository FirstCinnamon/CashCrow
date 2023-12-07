#include "crow.h"
#include "crow/app.h"
#include "crow/common.h"
#include "crow/http_response.h"
#include "crow/mustache.h"
#include "crow/middlewares/cors.h"
#include "crow/query_string.h"
#include "crow/middlewares/session.h"
#include "crow/json.h"
#include "rapidcsv.h"

#include <string>
#include <vector>
#include <pqxx/pqxx>
#include <limits>
#include <stdexcept>
#include <fstream>
#include <sstream>

#include <cstdio>
#include <cstring>
#include <ctime>

#include "db/db.hpp"
#include "crypto.hpp"
#include "rand.hpp"

#define ROOT_URL "https://localhost:18080/"
#define SALT_LEN 20
#define USRNAME_MAX 20
#define PASSWD_MIN 8
#define PASSWD_MAX 26

static db::DBConnection trade("localhost", "postgres", "crow", "1234");

std::string price_now(const std::string& company) {
    std::string src{"price/csv/now" + company + ".csv"};
    rapidcsv::Document doc(src.data(), rapidcsv::LabelParams(-1, -1));
    std::vector<std::string> price = doc.GetColumn<std::string>(1);
    return price.back();
}

bool same_length_cmp(const std::string& p_l, const std::string& p_r)
{
    bool ret{};

    if (p_l.length() != p_r.length()) {
        return false;
    }

    for (int i{}; i < p_l.length(); ++i) {
        ret |= p_l[i] ^ p_r[i];
    }

    return ret == 0;
}

int main() {
    crow::mustache::set_global_base("html");

    using Session = crow::SessionMiddleware<crow::InMemoryStore>;
    crow::App<crow::CookieParser, Session, crow::CORSHandler> app{Session{
            crow::CookieParser::Cookie("session").max_age(/*one day*/ 24 * 60 * 60).path("/"),
            4,
            crow::InMemoryStore{}
    }};

    app.ssl_file("host.crt", "host.key");

    CROW_ROUTE(app, "/")([&](const crow::request &req)
    {
        crow::response response{};

        auto &session = app.get_context<Session>(req);

        const std::string sid{session.get<std::string>("sid")};
        if (sid.empty()) {
            auto page = crow::mustache::load("login.html");
            crow::mustache::context my_context;
            my_context["title"] = "Login";
            response.write(page.render_string(my_context));
        } else {
            if (trade.isValidSid(sid)) {
                response.redirect("/dashboard");
            } else {
                session.remove("sid");
                response.redirect("/");
            }
        }

        return response;
    });

    CROW_ROUTE(app, "/authenticate").methods(crow::HTTPMethod::POST)([&](const crow::request &req)
    {
        crow::response response{};

        auto& session{app.get_context<Session>(req)};

        const std::string myauth{req.get_header_value("Authorization")};
        std::string mycreds{};
        try {
            mycreds = myauth.substr(6);
        } catch (const std::out_of_range& e) {
            return crow::response(crow::status::BAD_REQUEST);
        }
        const std::string d_mycreds{crow::utility::base64decode(mycreds, mycreds.size())};

        const size_t found{d_mycreds.find(':')};
        if (found == std::string::npos) {
            return crow::response(crow::status::BAD_REQUEST);
        }

        const std::string username{d_mycreds.substr(0, found)};
        const std::string password{d_mycreds.substr(found+1)};

        for (const char& e : username) {
            if (!isascii(e) || iscntrl(e) || isspace(e)) {
                auto page = crow::mustache::load("login_failure.html");
                response.write(page.render_string());
                return response;
            }
        }

        for (const char& e : password) {
            if (!isascii(e) || iscntrl(e) || isspace(e)) {
                auto page = crow::mustache::load("login_failure.html");
                response.write(page.render_string());
                return response;
            }
        }

        db::AccountPassword account_pass{};
        try {
            account_pass = trade.selectFromAccountSecurity(username);
        } catch (const std::exception& e) {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }

        if (account_pass.hash.empty() || account_pass.salt.empty()) {
            auto page = crow::mustache::load("login_failure.html");
            response.write(page.render_string());
        } else {
            const std::string login_salt = account_pass.salt;

            const std::string login_hash{sha256(login_salt + password)};
            if (login_hash.empty()) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            if (same_length_cmp(login_hash, account_pass.hash)) {
                int uid{};
                try {
                    uid = trade.selectIdFromAccountSecurity(username);
                } catch (const std::exception& e) {
                    return crow::response(crow::status::INTERNAL_SERVER_ERROR);
                }

                try {
                    trade.tryInsertSession(uid);
                } catch (const std::exception& e) {
                    std::cout << e.what() << std::endl;
                    return crow::response(crow::status::INTERNAL_SERVER_ERROR);
                }

                const std::string sid{trade.uidToSid(uid)};
                session.set<std::string>("sid", sid);

                response.add_header("HX-Redirect", ROOT_URL);
            } else {
                auto page = crow::mustache::load("login_failure.html");
                response.write(page.render_string());
            }
        }

        return response;
    });

    CROW_ROUTE(app, "/logout").methods(crow::HTTPMethod::POST)([&](const crow::request &req)
    {
        crow::response response{};
        auto &session = app.get_context<Session>(req);
        session.remove("sid");
        response.add_header("HX-Redirect", ROOT_URL);
        return response;
    });

    CROW_ROUTE(app, "/dashboard")([&](const crow::request &req)
    {
        crow::response response{};

        auto &session = app.get_context<Session>(req);

        const std::string sid{session.get<std::string>("sid")};
        if (!sid.empty() && trade.isValidSid(sid)) {
            auto page = crow::mustache::load("index.html");
            crow::mustache::context my_context;
            my_context["title"] = "Dashboard";
            response.write(page.render_string(my_context));
        } else {
            session.remove("sid");
            response.redirect("/");
        }

        return response;
    });

    CROW_ROUTE(app, "/profile")([&](const crow::request &req)
    {
        crow::response response{};

        auto &session = app.get_context<Session>(req);

        const std::string sid{session.get<std::string>("sid")};
        if (!sid.empty() && trade.isValidSid(sid)) {
            auto page = crow::mustache::load("profile.html");
            crow::mustache::context my_context;
            my_context["title"] = "Profile";
            response.write(page.render_string(my_context));
        } else {
            session.remove("sid");
            response.redirect("/");
        }

        return response;
    });

    CROW_ROUTE(app, "/getUserFinancialData")([&](const crow::request& req)
    {
        crow::response response{};

        auto& session = app.get_context<Session>(req);

        std::string sid{session.get<std::string>("sid")};
        if (!sid.empty() && trade.isValidSid(sid)) {
            int uid{};
            try {
                uid = trade.sidToUid(sid);
            } catch (const std::exception& e) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            std::vector<db::BankAccount> bankAccounts{};
            try {
                bankAccounts = trade.selectFromBankAccount(uid);
            } catch (const std::exception& e) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            if (bankAccounts.empty()) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            crow::json::wvalue financial_data{};
            financial_data["bankAccounts"] = {
                    {{"id", bankAccounts[0].id}, {"accountName", bankAccounts[0].bank_name}, {"balance", bankAccounts[0].balance}},
                    {{"id", bankAccounts[1].id}, {"accountName", bankAccounts[1].bank_name}, {"balance", bankAccounts[1].balance}}
            };

            float totalBalance = trade.selectFromAccountInfo(uid);
            financial_data["totalBalance"] = totalBalance;

            response = financial_data;
        } else {
            session.remove("sid");
            response.redirect("/");
        }

        return response;
    });

    CROW_ROUTE(app, "/press_change_password")([&](const crow::request &req)
    {
        crow::response response{};

        auto& session = app.get_context<Session>(req);

        const std::string sid{session.get<std::string>("sid")};
        if (!sid.empty() && trade.isValidSid(sid)) {
            auto page = crow::mustache::load("change_password.html");
            crow::mustache::context my_context;
            my_context["title"] = "Change Password";
            response.write(page.render_string(my_context));
        } else {
            session.remove("sid");
            response.redirect("/");
        }

        return response;
    });

    CROW_ROUTE(app, "/ChangePassword").methods("POST"_method)([&](const crow::request& req)
    {
        crow::response response{};

        auto &session = app.get_context<Session>(req);

        std::string sid{session.get<std::string>("sid")};
        if (!sid.empty() && trade.isValidSid(sid)) {
            const crow::query_string formData = req.get_body_params();

            std::string new_passwd = formData.get("password");

            crow::mustache::context change_passwd_context{};

            for (const char& e : new_passwd) {
                if (!isascii(e) || iscntrl(e) || isspace(e)) {
                    change_passwd_context["error_message"] = "Invalid password! Password must be valid ASCII with no whitespaces or control characters.";
                    auto page = crow::mustache::load("change_password_failure.html");

                    response.write(page.render_string(change_passwd_context));
                    return response;
                }
            }

            if (new_passwd.length() < PASSWD_MIN) {
                change_passwd_context["error_message"] = "New password is too short! Minimum 8 characters";
                auto page = crow::mustache::load("change_password_failure.html");

                response.write(page.render_string(change_passwd_context));
                return response;
            } else if (new_passwd.length() > PASSWD_MAX) {
                change_passwd_context["error_message"] = "New password is too long! Maximum 26 characters";
                auto page = crow::mustache::load("change_password_failure.html");

                response.write(page.render_string(change_passwd_context));
                return response;
            }

            const std::string salt{gen_rand_str(SALT_LEN)};
            if (salt.empty()) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            const std::string hash{sha256(salt + new_passwd)};
            if (hash.empty()) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            int uid{};
            try {
                uid = trade.sidToUid(sid);
            } catch (const std::exception& e) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            try {
                trade.updatePassword(uid, salt, hash);
            } catch (const std::exception& e) {
                return crow::response(crow::status::INTERNAL_SERVER_ERROR);
            }

            auto page = crow::mustache::load("change_password_success.html");
            response.write(page.render_string());
        } else {
            session.remove("sid");
            response.redirect("/");
        }

        return response;
    });

    CROW_ROUTE(app, "/profile_action").methods("POST"_method)([&](const crow::request& req) {
        crow::response response{};

        auto &session = app.get_context<Session>(req);

        std::string sid{session.get<std::string>("sid")};
        if (!sid.empty() && trade.isValidSid(sid)) {
            try {
                const crow::query_string postData = req.get_body_params();

                std::string action = postData.get("action");
                std::string amount = postData.get("amount");
                std::string accountId = postData.get("accountId");

                auto& session{ app.get_context<Session>(req) };
                std::string sid{ session.get<std::string>("sid") };
                int Uid = trade.sidToUid(sid);

                std::string responseMessage;
                if (action == "deposit") {
                    std::vector<db::BankAccount> bankAccounts = trade.selectFromBankAccount(Uid);

                    int id1 = bankAccounts[0].id;
                    int id2 = bankAccounts[1].id;

                    float check = 0;
                    float amount_float = stof(amount);
                    bool is_id1 = false;

                    if (stoi(accountId) == id1) {
                        check = bankAccounts[0].balance;
                        is_id1 = true;
                    }
                    else
                        check = bankAccounts[1].balance;

                    if (check >= amount_float) {
                        trade.changeAccount(Uid, amount_float);
                        if(is_id1)
                            trade.increaseBankAccount(id1, -1 * amount_float);
                        else
                            trade.increaseBankAccount(id2, -1 * amount_float);
                        responseMessage = "Successfully deposited $" + amount;
                    } else
                        responseMessage = "Invalid Amount : " + amount;


                } else if (action == "withdraw") {
                    std::vector<db::BankAccount> bankAccounts = trade.selectFromBankAccount(Uid);

                    int id1 = bankAccounts[0].id;
                    int id2 = bankAccounts[1].id;

                    float check = 0;
                    float amount_float = stof(amount);
                    bool is_id1 = false;
                    if (stoi(accountId) == id1)
                        is_id1 = true;

                    check = trade.selectFromAccountInfo(Uid);

                    if (check >= amount_float) {
                        trade.changeAccount(Uid, -amount_float);
                        if(is_id1)
                            trade.increaseBankAccount(id1, amount_float);
                        else
                            trade.increaseBankAccount(id2, amount_float);
                        responseMessage = "Successfully withdrew $" + amount;
                    } else
                        responseMessage = "Invalid Amount : " + amount;

                } else {
                    responseMessage = "Action not recognized";
                }
                return crow::response(200, responseMessage);
            } catch (const std::exception& e) {
                std::cerr << "Exception caught in /profile_action: " << e.what() << std::endl;
                return crow::response(500, "Internal Server Error");
            }
        } else {
            session.remove("sid");
            response.code = 303;
            response.redirect("/");
            return response;
        }
    });

    CROW_ROUTE(app, "/trading")([&](const crow::request &req)
    {
        crow::response response{};
        auto &session = app.get_context<Session>(req);

        const std::string sid{session.get<std::string>("sid")};
        if (!sid.empty() && trade.isValidSid(sid)) {
            auto page = crow::mustache::load("trading.html");
            crow::mustache::context my_context;
            my_context["title"] = "Trading";
            my_context["company"] = "A";
            response.write(page.render_string(my_context));
        } else {
            session.remove("sid");
            response.redirect("/");
        }

        return response;
    });

    CROW_ROUTE(app, "/price/<string>")([](std::string company){
        crow::response foo;
        foo.set_static_file_info("price/csv/now" + company + ".csv");
        return foo;
    });

    CROW_ROUTE(app, "/reload_price/<string>")([](std::string company){
        crow::response response{};
        auto page = crow::mustache::load("chart.html");
        crow::mustache::context my_context;
        my_context["company"] = company.data();
        response.write(page.render_string(my_context));
        return response;
    });

    CROW_ROUTE(app, "/price_now/<string>")([](std::string company){
        return price_now(company);
    });

    CROW_ROUTE(app, "/time_now")([](){
        time_t timer = time(NULL);
        struct tm* t = localtime(&timer);
        std::string time_now = std::to_string(t->tm_hour) + ":" + std::to_string(t->tm_min);
        return time_now;
    });

    CROW_ROUTE(app, "/trade_company/<string>")([](std::string company){
        crow::response response{};
        auto page = crow::mustache::load("trade_company.html");
        crow::mustache::context my_context;
        my_context["company"] = company.data();
        response.write(page.render_string(my_context));
        return response;
    });

    CROW_ROUTE(app, "/owned_stocks").methods(crow::HTTPMethod::POST)([&](const crow::request &req) -> crow::response
    {
        crow::response response{};
        auto &session = app.get_context<Session>(req);

        std::string sid{session.get<std::string>("sid")};
        if (!trade.isValidSid(sid) || sid.empty()) {
            session.remove("sid");
            response.redirect("/");
            return response;
        }
        int uid = trade.sidToUid(sid);
        const crow::query_string ret = req.get_body_params();
        std::string company = ret.get("company");
        std::map<std::string, int> owned = trade.selectFromOwnedStock(uid);
        if (owned.find(company) == owned.end()) {
            response.write("0");
            return response;
        }
        int owned_stock_number = owned[company];
        response.write(std::to_string(owned_stock_number));
        return response;
  });

    CROW_ROUTE(app, "/trade").methods(crow::HTTPMethod::POST)([&](const crow::request &req) -> crow::response
    {
        crow::response response{};

        auto &session = app.get_context<Session>(req);
            std::string hidden_elem_company = R"(<input type="hidden" id="company" name="company" value="{{company}}">)";

        std::string sid{session.get<std::string>("sid")};
        if (!trade.isValidSid(sid) || sid.empty()) {
            session.remove("sid");
            response.redirect("/");
            return response;
        }

        const crow::query_string ret = req.get_body_params();
        std::string str_amount = ret.get("amount");
        std::string company = ret.get("company");
        std::string action = ret.get("action");

        int amount = std::atoi(str_amount.c_str());
        if (amount <= 0) {
            response.write("Invalid amount entered.");
            return response;
        }

        int uid = trade.sidToUid(sid);
        float price = std::atof(price_now(company).c_str());
        float product_price = amount * price;
        if (action == "buy") {
            float balance = trade.selectFromAccountInfo(uid);
            if (balance < product_price) {
                return "You're lacking money! Your account balance is: " + std::to_string(balance);
            }
            trade.insertTradeHistory(company, price, uid);

            std::map<std::string, int> owned = trade.selectFromOwnedStock(uid);
            int num_owned;
            if (owned.find(company) == owned.end()) {num_owned = 0;} else {num_owned = owned[company];}
            std::map<std::string, float> avgs_old = trade.selectFromAvgPrice(uid);
            float price_avg_old;
            if (avgs_old.find(company) == avgs_old.end()) {price_avg_old = 0;} else {price_avg_old = avgs_old[company];}
            float avg_price = (static_cast<float>(num_owned) * price_avg_old + product_price) / (num_owned + amount);
            trade.upsertAvgPrice(uid, company, avg_price);

            trade.upsertOwnedStock(uid, company, amount);
            trade.changeAccount(uid, -product_price);
            balance = trade.selectFromAccountInfo(uid);
            return "Purchased " + std::to_string(amount) + "stocks! Your account balance is: " + std::to_string(balance);
        } else if (action == "sell") {
            std::map<std::string, int> owned = trade.selectFromOwnedStock(uid);
            if (owned.find(company) == owned.end()) {
                return "You do not have stocks in company " + company;
            }
            int owned_stock_number = trade.selectFromOwnedStock(uid)[company];
            if (owned_stock_number < amount) {
            return "You're lacking stocks! You have " + std::to_string(owned_stock_number) + "stocks of company " + company;
            }
            trade.insertTradeHistory(company, price, uid);
            trade.upsertOwnedStock(uid, company, -amount);
            trade.changeAccount(uid, product_price);
            float balance = trade.selectFromAccountInfo(uid);
            return "Sold " + std::to_string(amount) + "stocks! Your account balance is: " + std::to_string(balance);
        } else {
            return {400, "invalid action!"};
        }
    });

    CROW_ROUTE(app, "/portfolio")([&](const crow::request &req)
    {
        crow::response response{};

        auto &session = app.get_context<Session>(req);

        std::string sid{session.get<std::string>("sid")};
        if (!sid.empty() && trade.isValidSid(sid)) {
            auto page = crow::mustache::load("portfolio.html");
            crow::mustache::context my_context;
            my_context["title"] = "Portfolio";
            response.write(page.render_string(my_context));
        } else {
            session.remove("sid");
            response.redirect("/");
        }

        return response;
    });

    CROW_ROUTE(app, "/api/stocks").methods("GET"_method)([&](const crow::request& req) {
        auto& session{ app.get_context<Session>(req) };
        std::string sid{ session.get<std::string>("sid") };
        int uid = trade.sidToUid(sid);

        auto stocks = trade.selectFromOwnedStock(uid);
        auto avg = trade.selectFromAvgPrice(uid);
        auto tuples = jointProduct(stocks, avg);
        std::stringstream ss;
        if (!tuples.empty())
        {
            ss << "[";
            for (const auto& t : tuples) {
                std::string bankName = std::get<0>(t);
                int count = std::get<1>(t);
                if (count == 0)
                    continue;
                float avg = std::get<2>(t);
                float curPrice = atof(price_now(bankName).c_str());
                float totalValue = count * avg,
                    returnDollars = count * (curPrice - avg),
                    returnPercent = (curPrice - avg) / avg * 100;
                std::string jsonRow =
                    string_format(R"({"companyName": "%s", "sharesOwned": %d, "totalValue": %f, "returnDollars": %f, "returnPercent": %f})",
                        bankName.c_str(), count, totalValue, returnDollars, returnPercent);
                ss << jsonRow << ",";
            }
        }
        std::string str = ss.str();
        if (str.length() > 0)
        {
            str.pop_back();
            str.push_back(']');
        }

        std::string stockDataJson = R"([
        {"companyName": "Company A", "sharesOwned": 100, "totalValue": 1500.00, "returnDollars": 150.00, "returnPercent": 10.00},
        {"companyName": "Company B", "sharesOwned": 200, "totalValue": 3000.00, "returnDollars": 300.00, "returnPercent": 10.00},
        {"companyName": "Company C", "sharesOwned": 150, "totalValue": 2250.00, "returnDollars": 250.00, "returnPercent": 12.50},
        {"companyName": "Company D", "sharesOwned": 180, "totalValue": 3600.00, "returnDollars": 360.00, "returnPercent": 11.00}
    ])";
        crow::response response;
        response.set_header("Content-Type", "application/json");
        response.write(str);
        return response;
        });

    CROW_ROUTE(app, "/goto_register")([]()
    {
        auto page = crow::mustache::load("register.html");
        crow::mustache::context my_context;
        my_context["title"] = "Register";
        return page.render(my_context);
    });

    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)([](const crow::request &req)
    {
        crow::response response{};

        const std::string myauth{req.get_header_value("Authorization")};
        std::string mycreds{};
        try {
            mycreds = myauth.substr(6);
        } catch (const std::out_of_range& e) {
            return crow::response(crow::status::BAD_REQUEST);
        }
        const std::string d_mycreds{crow::utility::base64decode(mycreds, mycreds.size())};

        const size_t found{d_mycreds.find(':')};
        if (found == std::string::npos) {
            return crow::response(crow::status::BAD_REQUEST);
        }

        const std::string username{d_mycreds.substr(0, found)};
        const std::string password{d_mycreds.substr(found+1)};
        const std::string email{req.get_body_params().get("email")};

        crow::mustache::context register_context{};

        for (const char& e : username) {
            if (!isascii(e) || iscntrl(e) || isspace(e)) {
                register_context["error_message"] = "Invalid username! Username must be valid ASCII with no whitespaces or control characters.";
                register_context["email"] = email;
                register_context["username"] = username;
                auto page = crow::mustache::load("register_failure.html");

                response.write(page.render_string(register_context));
                return response;
            }
        }

        for (const char& e : password) {
            if (!isascii(e) || iscntrl(e) || isspace(e)) {
                register_context["error_message"] = "Invalid password! Password must be valid ASCII with no whitespaces or control characters.";
                register_context["email"] = email;
                auto page = crow::mustache::load("register_failure.html");

                response.write(page.render_string(register_context));
                return response;
            }
        }

        if (username.length() > USRNAME_MAX) {
            register_context["error_message"] = "Username is too long! Maximum 20 characters";
            register_context["email"] = email;
            auto page = crow::mustache::load("register_failure.html");

            response.write(page.render_string(register_context));
            return response;
        } else if (password.length() < PASSWD_MIN) {
            register_context["error_message"] = "Password is too short! Minimum 8 characters";
            register_context["email"] = email;
            auto page = crow::mustache::load("register_failure.html");

            response.write(page.render_string(register_context));
            return response;
        } else if (password.length() > PASSWD_MAX) {
            register_context["error_message"] = "Password is too long! Maximum 26 characters";
            register_context["email"] = email;
            auto page = crow::mustache::load("register_failure.html");

            response.write(page.render_string(register_context));
            return response;
        }

        const std::string salt{gen_rand_str(SALT_LEN)};
        if (salt.empty()) {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }

        const std::string hash{sha256(salt + password)};
        if (hash.empty()) {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }

        try {
            trade.insertAccount(username, salt, hash);
        } catch (const pqxx::unique_violation& e) {
            register_context["error_message"] = "User with username " + username + " already exists!";
            register_context["username"] = "";
            register_context["email"] = email;
            auto page = crow::mustache::load("register_failure.html");

            response.write(page.render_string(register_context));
            return response;
        } catch (const std::exception& e) {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }

        register_context["username"] = username;

        auto page = crow::mustache::load("register_success.html");
        response.write(page.render_string(register_context));

        return response;
    });

    app.port(18080).multithreaded().run();
}

int exploit() {
	  printf("[Team 4] Dummy Function for PoC\n");
    return 0;
}

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

#include <cstdio>
#include <cstring>
#include <ctime>

#include "db/db.hpp"
#include "crypto.hpp"
#include "rand.hpp"

#define ROOT_URL "http://localhost:18080/"
#define SALT_LEN 20
#define PASSWD_MIN 8
#define PASSWD_MAX 26

// redirect to root: if logined then dashboard, else login page
crow::response redirect() {
    crow::response res{};
    res.redirect("/");
    return res;
}

bool is_sid_valid(const std::string& sid) {
    // check out DB so that given sid is matched with a user
    // for test purpose, only sid=1234 is valid
    // NOTE: avoid strcmp()
    if (sid == "1234") { // change this condition later
        return true;
    } else {
        return false;
    }
}

std::string price_now(const std::string& company) {
    std::string src{"price/csv/now" + company + ".csv"};
    rapidcsv::Document doc(src.data(), rapidcsv::LabelParams(-1, -1));
    std::vector<std::string> price = doc.GetColumn<std::string>(1);
    return price.back();
}

int main() {
    // Global Template directory
    crow::mustache::set_global_base("html");

    // Define the crow application with session and CORSHandler middleware
    // InMemoryStore stores all entries in memory
    using Session = crow::SessionMiddleware<crow::InMemoryStore>;
    crow::App<crow::CookieParser, Session, crow::CORSHandler> app{Session{
            // customize cookies
            crow::CookieParser::Cookie("session").max_age(/*one day*/ 24 * 60 * 60).path("/"),
            // set session id length (small value only for demonstration purposes)
            4,
            // init the store
            crow::InMemoryStore{}
    }};

    // Define the endpoint at the root directory
    CROW_ROUTE(app, "/")
            ([&](const crow::request &req) {
                crow::response response("");

                // get session as middleware context
                auto &session = app.get_context<Session>(req);

                // get session id(sid) which is temporarily an integer
                // return empty string if value is now found
                std::string string_v = session.get<std::string>("sid");
                if (string_v.empty()) {
                    // if sid is not set(not yet login-ed), so show login page
                    // for test purpose, root page is login page
                    auto page = crow::mustache::load("login.html");
                    crow::mustache::context my_context;
                    my_context["title"] = "Login";
                    response.write(page.render_string(my_context));
                    return response;
                } else {
                    // session id exists, so go check if sid is valid and show user's dashboard
                    if (is_sid_valid(string_v)) {
                        // valid sid, so load user's dashboard page
                        response.redirect("/dashboard");
                        return response;
                    } else {
                        // invalid sid, so remove sid and redirect to root directory
                        session.remove("sid");
                        return redirect();
                    }
                }
            });

    // for test purpose, login is only done if uid=test and pw=test
    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)([&](const crow::request &req)
    {
        crow::response response{};

        // get session as middleware context
        auto& session{app.get_context<Session>(req)};

        const std::string myauth{req.get_header_value("Authorization")};
        const std::string mycreds{myauth.substr(6)};
        const std::string d_mycreds{crow::utility::base64decode(mycreds, mycreds.size())};

        const size_t found{d_mycreds.find(':')};
        if (found == std::string::npos) {
            return crow::response(crow::status::BAD_REQUEST);
        }

        // TODO: find a way to limit username/password length from the frontend side of things or else susceptible to DOS attack
        const std::string username{d_mycreds.substr(0, found)};
        const std::string password{d_mycreds.substr(found+1)};

        // TODO: retrieve salt + hash from account_security using username lookup
        // TODO: change this if condition in the future
        if (username == "test" && password == "test") {
            // success, so give user an sid
            // sid generation should be randomized

            // TODO: remove the below temporary line once we have DB functionality
            session.set<std::string>("sid", "1234");
            // session.set<std::string>("sid", std::to_string(generate_random_num()));

            // redirect to root page
            response.add_header("HX-Redirect", ROOT_URL);

            return response;
        }

        auto page = crow::mustache::load("login_failure.html");
        response.write(page.render_string());

        return response;
    });

    CROW_ROUTE(app, "/logout")
            .methods(crow::HTTPMethod::POST)([&](
                    const crow::request &req) {
                crow::response response("");
                auto &session = app.get_context<Session>(req);
                session.remove("sid");
                response.add_header("HX-Redirect", ROOT_URL);
                return response;
            });

    CROW_ROUTE(app, "/dashboard")
            ([&](const crow::request &req) {
                crow::response response("");

                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                std::string string_v = session.get<std::string>("sid");

                // go check if sid is valid and show user's dashboard
                if (is_sid_valid(string_v) && !string_v.empty()) {
                    // valid sid, so user's contents
                    // needs more work to show customized page
                    auto page = crow::mustache::load("index.html");
                    crow::mustache::context my_context;
                    my_context["title"] = "Dashboard";
                    response.write(page.render_string(my_context));
                    return response;
                } else {
                    // invalid sid, so remove sid and redirect to root directory
                    session.remove("sid");
                    return redirect();
                }
            });

    CROW_ROUTE(app, "/profile")
            ([&](const crow::request &req) {
                crow::response response("");

                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                std::string string_v = session.get<std::string>("sid");

                // go check if sid is valid and show user's dashboard
                if (is_sid_valid(string_v) && !string_v.empty()) {
                    // valid sid, so user's contents
                    // needs more work to show customized page
                    auto page = crow::mustache::load("profile.html");
                    crow::mustache::context my_context;
                    my_context["title"] = "Profile";

                    response.write(page.render_string(my_context));
                    return response;
                } else {
                    // invalid sid, so remove sid and redirect to root directory
                    session.remove("sid");
                    return redirect();
                }
            });

    CROW_ROUTE(app, "/press_change_password")
            ([&](const crow::request &req) {
                crow::response response("");

                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                std::string string_v = session.get<std::string>("sid");

                auto page = crow::mustache::load("change_password.html");
                crow::mustache::context my_context;
                my_context["title"] = "Change Password";

                response.write(page.render_string(my_context));
                return response;
            });

    CROW_ROUTE(app, "/ChangePassword").methods("POST"_method)
            ([&](const crow::request& req) {
                crow::response response;

                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                std::string sid = session.get<std::string>("sid");

                // go check if sid is valid
                if (is_sid_valid(sid) && !sid.empty()) {

                    // Parse the POST request body
                    const crow::query_string formData = req.get_body_params();

                    // Extract the new password from the form data
                    std::string newPassword = formData.get("password");
                    std::cout << "New password: " << newPassword << std::endl;

                    bool passwordChanged = true;

                    if (passwordChanged) {
                        response.body = "<script>window.location.href='/profile';</script>";
                        response.code = 200; // OK Status
                        return response;
                } else {
                        response.code = 303;
                        response.add_header("Location", "/error");
                        return response;
                    }
                } else {
                    // invalid sid, so remove sid and redirect to login or home page
                    session.remove("sid");
                    response.code = 303;
                    response.add_header("Location", "/login"); // Assuming "/login" is your login route
                    return response;
                }
            });


    CROW_ROUTE(app, "/profile_action").methods("POST"_method)([&](const crow::request& req) {
        crow::response response;
        // get session as middleware context
        auto &session = app.get_context<Session>(req);
        std::string sid = session.get<std::string>("sid");

        // go check if sid is valid
        if (is_sid_valid(sid) && !sid.empty()) {

            try {
                const crow::query_string postData = req.get_body_params();

                std::string action = postData.get("action");
                std::string amount = postData.get("amount");
                std::string accountId = postData.get("accountId");

                // Log the received data
                std::cout << "Action: " << action << ", Amount: " << amount << ", Account ID: " << accountId << std::endl;


                // If everything is okay, send a success response
                std::string responseMessage;
                if (action == "deposit") {
                    responseMessage = "Successfully deposited $" + amount;
                } else if (action == "withdraw") {
                    responseMessage = "Successfully withdrew $" + amount;
                } else {
                    // If action is not recognized
                    responseMessage = "Action not recognized";
                }
                return crow::response(200, responseMessage);


            } catch (const std::exception& e) {
                // Log the exception and send a response with the error
                std::cerr << "Exception caught in /profile_action: " << e.what() << std::endl;
                return crow::response(500, "Internal Server Error");
            }

        } else {
            // invalid sid, so remove sid and redirect to login or home page
            session.remove("sid");
            response.code = 303;
            response.add_header("Location", "/login"); // Assuming "/login" is your login route
            return response;
        }
    });

    CROW_ROUTE(app, "/get_balance")
            ([&](const crow::request &req) {

                crow::response response;
                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                std::string sid = session.get<std::string>("sid");

                // go check if sid is valid
                if (is_sid_valid(sid) && !sid.empty()) {

                    // Session 체크 및 잔액 조회 로직 구현
                    double amount = 2; // SQL 쿼리

                    std::stringstream ss;
                    ss << "$" << std::fixed << std::setprecision(2) << amount;
                    return crow::response(ss.str());

                } else {
                    // invalid sid, so remove sid and redirect to login or home page
                    session.remove("sid");
                    response.code = 303;
                    response.add_header("Location", "/login"); // Assuming "/login" is your login route
                    return response;
                }
            });


    CROW_ROUTE(app, "/deleteAccount")
            ([&](const crow::request& req) {
                crow::response response;

                // get session as middleware context
                auto &session = app.get_context<Session>(req);

                // 계정을 삭제하는 로직

                bool deleteSuccess = true;

                if (deleteSuccess) {
                    // 계정이 성공적으로 삭제되었다는 메시지를 문자열로 반환
                    response = crow::response(200, "Account successfully deleted.");
                } else {
                    // 오류 메시지를 반환
                    response = crow::response(400, "Failed to delete account.");
                }

                session.remove("sid");
                return redirect();

            });




    // Start of codes about trading
    CROW_ROUTE(app, "/trading")
            ([&](const crow::request &req) {
                crow::response response("");
                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                std::string string_v = session.get<std::string>("sid");

                // go check if sid is valid and show user's page
                if (is_sid_valid(string_v) && !string_v.empty()) {
                    // valid sid, so user's contents
                    // needs more work to show customized page
                    auto page = crow::mustache::load("trading.html");
                    crow::mustache::context my_context;
                    my_context["title"] = "Trading";
                    // insert company name
                    my_context["company"] = "A";
                    response.write(page.render_string(my_context));
                    return response;
                } else {
                    // invalid sid, so remove sid and redirect to root directory
                    session.remove("sid");
                    return redirect();
                }
            });

    CROW_ROUTE(app, "/price/<string>")([](std::string company){
        crow::response foo;
        foo.set_static_file_info("price/csv/now" + company + ".csv");
        return foo;
    });

    CROW_ROUTE(app, "/reload_price/<string>")([](std::string company){
        crow::response response("");
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
        crow::response response("");
        auto page = crow::mustache::load("trade_company.html");
        crow::mustache::context my_context;
        my_context["company"] = company.data();
        response.write(page.render_string(my_context));
        return response;
  });

    CROW_ROUTE(app, "/trade")
            .methods(crow::HTTPMethod::POST)
                    ([&](const crow::request &req) -> crow::response {

                        crow::response response("");
                        auto &session = app.get_context<Session>(req);
                        std::string string_v = session.get<std::string>("sid");

                        if (!is_sid_valid(string_v) || string_v.empty()) {
                            // invalid sid, so remove sid and redirect to root directory
                            session.remove("sid");
                            return redirect();
                        }

                        // valid sid, so proceed with trade
                        const crow::query_string ret = req.get_body_params();
                        std::string str_amount = ret.get("amount");
                        std::string company = ret.get("company");
                        std::string action = ret.get("action");
                        // int uid = trade.get_uid_from_sid(string_v); string->int.

                        // initial validation for amount
                        int amount = std::atoi(str_amount.c_str());
                        if (amount <= 0) {
                            // parsing error; atoi does not throw exception
                            response.write("Invalid amount entered.");
                            return response;
                        }

                        db::DBConnection exec("dbname=crow user=postgres password=1234 host=localhost");
//                        std::string company_name = "A";

                        // trade
                        float price = static_cast<float>(std::atoi(price_now(company).c_str()));
                        float product_price = amount * price;
                        // 다중탭 띄우면 race condition 가능? transaction이 보호되나? 중간에 누가 침입가능?
                        db::DBConnection trade("localhost", "postgres", "crow", "1234");
                        trade.insertAccount("dd", "dd", "dd"); // temp code
                        if (action == "buy") {
                            // 현재 예치금 잔액 확인 후 product_price보다 예치금이 적으면 에러
                            // float balance = trade.selectFromBankAccount(uid); 타입오류 수정하시기
                            // if (balance < product_price) {
                            // // error
                            // return "You're lacking money! Your account balance is: " + std::to_string(balance)
                            // }
                            // trade.insertTradeHistory("A", 12, 1);
                            // trade.upsertStocks();
                            // reduce account balance
                            // trade.updateAccountBalance(uid, A, -product_price);
                            // get updated balance
                            // balance = trade.selectFromBankAccount(uid); 타입오류 수정하시기
                            return "bought " + std::to_string(amount) + "! Your account balance is: ";// + std::to_string(balance);
                        } else if (action == "sell") {
                            // 현재 보유수 확인. 주식 갖고 있지 않거나 amount보다 적으면 에러
                            // int owned_stock_number = trade.selectfromownedstock(uid, company)
                            // if (owned_stock_number < amount) {
                            // // error
                            // return "You're lacking stocks! Your have " + std::to_string(owned_stock_number) + "stocks of " + company;
                            // }
                            // trade.insertTradeHistory("A", 12, -1);
                            // trade.down_stocks();
                            // increase account balance
                            // trade.updateAccountBalance(uid, A, +product_price);
                            // get updated balance
                            // balance = trade.selectFromBankAccount(uid); 타입오류 수정하시기
                            return "sold " + std::to_string(amount) + "! Your account balance is: ";// + std::to_string(balance);
                        } else {
                            return {400, "invalid action!"};
                        }


                    });
    // End of code about trading

    CROW_ROUTE(app, "/portfolio")
            ([&](const crow::request &req) {
                crow::response response("");
                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                std::string string_v = session.get<std::string>("sid");

                // go check if sid is valid and show user's dashboard
                if (is_sid_valid(string_v) && !string_v.empty()) {
                    // valid sid, so user's contents
                    // needs more work to show customized page
                    auto page = crow::mustache::load("portfolio.html");
                    crow::mustache::context my_context;
                    my_context["title"] = "Portfolio";
                    response.write(page.render_string(my_context));
                    return response;
                } else {
                    // invalid sid, so remove sid and redirect to root directory
                    session.remove("sid");
                    return redirect();
                }
            });

    CROW_ROUTE(app, "/goto_register")
            ([]() {
                auto page = crow::mustache::load("register.html");
                crow::mustache::context my_context;
                my_context["title"] = "Register";
                return page.render(my_context);
            });

    CROW_ROUTE(app, "/register").methods(crow::HTTPMethod::POST)([](const crow::request &req)
    {
        crow::response response{};

        const std::string myauth{req.get_header_value("Authorization")};
        const std::string mycreds{myauth.substr(6)};
        const std::string d_mycreds{crow::utility::base64decode(mycreds, mycreds.size())};

        const size_t found{d_mycreds.find(':')};
        if (found == std::string::npos) {
            return crow::response(crow::status::BAD_REQUEST);
        }

        // TODO: find a way to limit username/password length from the frontend side of things or else susceptible to DOS attack
        const std::string username{d_mycreds.substr(0, found)};
        const std::string password{d_mycreds.substr(found+1)};

        // Uncomment below line if we actually need to use email
        // const std::string email{req.get_body_params().get("email")};

        crow::mustache::context register_context{};
        register_context["username"] = username;

        // TODO: Check DB for duplicate username and return response with error_message if duplicate

        if (password.length() < PASSWD_MIN) {
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

        const std::string salt{generate_salt(SALT_LEN)};
        if (salt.empty()) {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }

        const std::string hash{sha256(salt + password)};
        if (hash.empty()) {
            return crow::response(crow::status::INTERNAL_SERVER_ERROR);
        }

        // TODO: Insert into account_security table here
        // TODO: Check for error when inserting

        auto page = crow::mustache::load("register_success.html");
        response.write(page.render_string(register_context));

        return response;
    });

    // Set the port, set the app to run on multiple threads, and run the app
    app.port(18080).multithreaded().run();
}

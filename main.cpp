#include "crow.h"
#include "crow/app.h"
#include "crow/common.h"
#include "crow/http_response.h"
#include "crow/mustache.h"
#include "crow/middlewares/cors.h"
#include "crow/query_string.h"
#include "crow/middlewares/session.h"
#include "rapidcsv.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <fstream>
#include <crow/json.h>
#include <pqxx/pqxx>
#include "db/db.hpp"

#define ROOT_URL "http://localhost:18080/"

// redirect to root: if logined then dashboard, else login page
crow::response redirect() {
    crow::response res;
    res.redirect("/");
    return res;
}

bool is_sid_valid(std::string &sid) {
    // check out DB so that given sid is matched with a user
    // for test purpose, only sid=1234 is valid
    if (strcmp("1234", sid.data()) == 0) { // change this condition later
        return true;
    } else {
        return false;
    }
}

std::string price_now(std::string company) {
    std::string src = "price/csv/now" + company + ".csv";
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
    CROW_ROUTE(app, "/login")
            .methods(crow::HTTPMethod::POST)([&](
                    const crow::request &req) {
                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                const crow::query_string ret = req.get_body_params();
                crow::response response("");

                if (strcmp(ret.get("uname"), "test") == 0 && strcmp(ret.get("pass"), "test") == 0) {
                    // success, so give user an sid
                    // sid generation should be randomized
                    session.set<std::string>("sid", "1234");
                    // redirect to root page
                    response.add_header("HX-Redirect", ROOT_URL);
                    return response;
                }
                // login failed, show retry message with uname field filled
                crow::mustache::context my_context;
                my_context["uname"] = ret.get("uname");
                auto page = crow::mustache::load("login_failure.html");
                response.write(page.render_string(my_context));
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


// Handle POST request on "/profile_action"
    CROW_ROUTE(app, "/profile_action").methods("POST"_method)([&](const crow::request& req) {
        const crow::query_string postData = req.get_body_params();

        std::string id = "1";

        db::DBConnection exec("dbname=crow user=postgres password=1234 host=localhost");

        std::string action = postData.get("action");
        std::string amount = postData.get("amount");
        std::string accountId = postData.get("accountId");


        std::cout << "Action: " << action << ", Amount: " << amount << ", Account ID: " << accountId << std::endl;

        return crow::response(200, "Action processed");
    });




    // Start of codes about trading
    CROW_ROUTE(app, "/trading")
            ([&](const crow::request &req) {
                crow::response response("");
                // get session as middleware context
                auto &session = app.get_context<Session>(req);
                std::string string_v = session.get<std::string>("sid");

                // go check if sid is valid and show user's dashboard
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
            .methods("POST"_method)
                    ([&](const crow::request &req) -> crow::response {

                        // parsing POST data
                        auto get_value = [&](const std::string &key) -> std::string {
                            auto key_pos = req.body.find(key + "=");
                            if (key_pos == std::string::npos) return "";
                            auto start_pos = key_pos + key.size() + 1;
                            auto end_pos = req.body.find("&", start_pos);
                            if (end_pos == std::string::npos) end_pos = req.body.size();
                            return req.body.substr(start_pos, end_pos - start_pos);
                        };

                        // get action and amount value
                        auto action = get_value("action");
                        auto amount = get_value("amount");

                        db::DBConnection exec("dbname=crow user=postgres password=1234 host=localhost");
//                        std::string company_name = "A";

                        // trade
                        if (action == "buy") {
                            return {"bought " + amount + "!"};
                        } else if (action == "sell") {
                            return {"sold " + amount + "!"};
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

    // for test purpose, register is only done if uid=test and pw=test
    CROW_ROUTE(app, "/register")
            .methods(crow::HTTPMethod::POST)([](
                    const crow::request &req) {
                const crow::query_string ret = req.get_body_params();
                crow::response response("");

                if (strcmp(ret.get("username"), "test") == 0 && strcmp(ret.get("password"), "test") == 0) {
                    // success and show welcom message with link to login page
                    crow::mustache::context my_context;
                    my_context["username"] = ret.get("username");
                    auto page = crow::mustache::load("register_success.html");
                    response.write(page.render_string(my_context));
                    return response;
                }

                // validation failed: eg) password not long enough, username duplication etc
                // error message
                crow::mustache::context my_context;
                my_context["error_message"] = "THIS IS USEFUL ERROR MESSAGE";
                auto page = crow::mustache::load("register_failure.html");
                response.write(page.render_string(my_context));
                return response;
            });

    // Set the port, set the app to run on multiple threads, and run the app
    app.port(18080).multithreaded().run();
}

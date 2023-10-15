#include "crow.h"
#include "crow/app.h"
#include "crow/common.h"
#include "crow/http_response.h"
#include "crow/mustache.h"
#include "crow/middlewares/cors.h"
#include "crow/query_string.h"
#include <cstring>
#include <ctime>
#include <string>

int main()
{
    // Global Template directory
    crow::mustache::set_global_base("html");

    // Define the crow application
    crow::App<crow::CORSHandler> app;

    // Define the endpoint at the root directory
    CROW_ROUTE(app, "/")([](){
        // for test purpose, root page is login page
        auto page = crow::mustache::load("login.html");
        return page.render();
    });

    // for test purpose, login is only done if uid=test and pw=test
    CROW_ROUTE(app, "/login")
        .methods(crow::HTTPMethod::POST)([](
                const crow::request& req
            ) {
          const crow::query_string ret = req.get_body_params();
          crow::response response("");

          if (strcmp(ret.get("uname"), "test") == 0
          && strcmp(ret.get("pass"), "test") == 0) {
            // success and redirect to index.html
            response.add_header("HX-Redirect", "http://localhost:18080/success_login");
            return response;
          }
      // login failed
      crow::mustache::context my_context;
        my_context["uname"] = ret.get("uname");
      auto page = crow::mustache::load("loginfailure.html");
        response.write(page.render_string(my_context));
      return response;

    });

    CROW_ROUTE(app, "/success_login")([](){
        auto page = crow::mustache::load("index.html");
        return page.render();
    });

    CROW_ROUTE(app, "/goto_login")([](){
          crow::response response("");
          response.add_header("HX-Redirect", "http://localhost:18080/");
      return response;
    });

    CROW_ROUTE(app, "/register")([](){
        auto page = crow::mustache::load("register.html");
        return page.render();
    // modulating html required
    });

    // Set the port, set the app to run on multiple threads, and run the app
    app.port(18080).multithreaded().run();
}

cmake_minimum_required(VERSION 3.23)
project(CashCrow)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 프로젝트 내 include 폴더를 포함
include_directories(${CMAKE_SOURCE_DIR}/include)

# Include directories for libpqxx
include_directories(/opt/homebrew/opt/libpqxx/include)
include_directories(/opt/homebrew/Cellar/openssl@3/3.1.4/include)

# Include directories for libpq (if needed)
include_directories(/opt/homebrew/opt/libpq/include)

# Add the paths for the libraries
link_directories(/opt/homebrew/opt/libpqxx/lib)
link_directories(/opt/homebrew/opt/libpq/lib)
link_directories(/opt/homebrew/Cellar/openssl@3/3.1.4/lib)

# CashCrow 실행 파일에 소스 코드 추가 (추가된 부분)
add_executable(CashCrow main.cpp crypto.cpp rand.cpp)

# libpq와 libpqxx 라이브러리를 링크
target_link_libraries(CashCrow pqxx pq crypto)

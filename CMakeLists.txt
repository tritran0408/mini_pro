cmake_minimum_required(VERSION 3.19)
# set the project name
project(Tutorial)

if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
    endif(COMMAND cmake_policy)
# add the executable
add_executable(Server hellobrowser.c)
FIND_LIBRARY(MICROHTTPD_LIB microhttpd)
FIND_LIBRARY(PTHREAD_LIB pthread)
FIND_LIBRARY(LIBJWT_LIBRARIES jwt)

target_link_libraries(Server ${MICROHTTPD_LIB})
target_link_libraries(Server ${PTHREAD_LIB})
target_link_libraries(Server ${LIBJWT_LIBRARIES})


add_executable(Client client.c)
FIND_LIBRARY(CURL_LIBRARIES curl)
target_link_libraries(Client ${CURL_LIBRARIES})
target_link_libraries(Client ${PTHREAD_LIB})
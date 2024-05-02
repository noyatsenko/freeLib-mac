
find_package( Qt5 REQUIRED COMPONENTS Widgets )

add_executable(demo2 demo2.cpp sendemail.cpp sendemail.h sendemail.ui)
target_link_libraries(demo2 SmtpMime Qt5::Widgets)
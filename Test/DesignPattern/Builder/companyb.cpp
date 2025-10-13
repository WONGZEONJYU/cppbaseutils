#include <companyb.hpp>

SystemConfig & CompanyB::buildSystemConfig() {
    builder.setMySQL("mysql://127.0.0.1/", "xiaomu", "xiaomumemeda");
    builder.setRedis("redis://127.0.0.1/", "xiaomuredis", "xiaomuredispw");
    builder.setKafka("", "", "");
    return builder.getSystemConfig();
}


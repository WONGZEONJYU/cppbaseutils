#include <companya.hpp>

SystemConfig & CompanyA::buildSystemConfig() {
    builder.setMySQL("mysql://127.0.0.1/", "xiaomu", "xiaomumemeda");
    builder.setRedis("", "", "");
    builder.setKafka("kafka://127.0.0.1", "xiaomukafka", "xiaomukafkapw");
    return builder.getSystemConfig();
}

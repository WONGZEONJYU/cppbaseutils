#include <iostream>
#include <companya.hpp>
#include <companyb.hpp>

int main() {
    SystemConfigBuilder builder {};
    builder.setMySQL("mysql://127.0.0.1/", "xiaomu", "xiaomumemeda");
    builder.setRedis("redis://127.0.0.1/", "xiaomuredis", "xiaomuredispw");
    builder.setKafka("kafka://127.0.0.1", "xiaomukafka", "xiaomukafkapw");
    auto const config { builder.getSystemConfig() };

    std::cout
        << "Mysql URL: " << config.m_MySQL_URL << "\n"
        << "Mysql USER: " << config.m_MySQL_USER << "\n"
        << "Mysql PW: " << config.m_MySQL_PW << "\n\n"
        << "Redis URL: " << config.m_Redis_URL << "\n"
        << "Redis USER: " << config.m_Redis_USER << "\n"
        << "Redis PW: " << config.M_Redis_PW << "\n\n"
        << "Kafka URL: " << config.M_Kafka_URL << "\n"
        << "Kafka USER: " << config.M_Kafka_USER << "\n"
        << "Kafka PW: " << config.M_Kafka_PW << "\n";

    CompanyA companyA{};
    auto const configA { companyA.buildSystemConfig() };

    CompanyB companyB;
    auto const configB { companyB.buildSystemConfig() };

    return 0;
}

#ifndef SYSTEM_CONFIG_HPP
#define SYSTEM_CONFIG_HPP 1

#include <string>

struct SystemConfig {
    constexpr SystemConfig() = default;
    explicit SystemConfig(std::string MySQL_URL, std::string MySQL_USER, std::string MySQL_PW,
        std::string Redis_URL, std::string Redis_USER, std::string Redis_PW,
        std::string Kafka_URL, std::string Kafka_USER, std::string Kafka_PW);

    std::string m_MySQL_URL{}
        ,m_MySQL_USER{}
        ,m_MySQL_PW{}

        ,m_Redis_URL{}
        ,m_Redis_USER{}
        ,M_Redis_PW{}

        ,M_Kafka_URL{}
        ,M_Kafka_USER{}
        ,M_Kafka_PW{};
};

#endif

#include <systemconfig.hpp>
#include <utility>

SystemConfig::SystemConfig(std::string MySQL_URL, std::string MySQL_USER, std::string MySQL_PW,
        std::string Redis_URL, std::string Redis_USER, std::string Redis_PW,
        std::string Kafka_URL, std::string Kafka_USER, std::string Kafka_PW)

:m_MySQL_URL{std::move(MySQL_URL)},m_MySQL_USER{std::move(MySQL_USER)},m_MySQL_PW{std::move(MySQL_PW)}
,m_Redis_URL{std::move(Redis_URL)},m_Redis_USER{std::move(Redis_USER)},M_Redis_PW{std::move(Redis_PW)}
,M_Kafka_URL{std::move(Kafka_URL)},M_Kafka_USER{std::move(Kafka_USER)},M_Kafka_PW{std::move(Kafka_PW)}
{}

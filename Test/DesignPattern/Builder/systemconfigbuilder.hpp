#ifndef SYSTEM_CONFIGBUILDER_HPP
#define SYSTEM_CONFIGBUILDER_HPP 1

#include <systemconfig.hpp>

class SystemConfigBuilder {
    SystemConfig config{};
public:
    int setMySQL(const std::string & MySQL_URL, const std::string & MySQL_USER, const std::string & MySQL_PW);
    int setRedis(const std::string & Redis_URL, const std::string & Redis_USER, const std::string & Redis_PW);
    int setKafka(const std::string & Kafka_URL, const std::string & Kafka_USER, const std::string & Kafka_PW);
    SystemConfig & getSystemConfig() noexcept;
};

#endif

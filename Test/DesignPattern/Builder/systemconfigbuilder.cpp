#include <systemconfigbuilder.hpp>

SystemConfig & SystemConfigBuilder::getSystemConfig() noexcept
{ return config; }

int SystemConfigBuilder::setMySQL(const std::string & MySQL_URL, const std::string & MySQL_USER, const std::string & MySQL_PW)
{
    config.m_MySQL_USER = MySQL_USER;
    config.m_MySQL_URL = MySQL_URL;
    config.m_MySQL_PW = MySQL_PW;
    return 0;
}

int SystemConfigBuilder::setRedis(const std::string& Redis_URL, const std::string& Redis_USER, const std::string& Redis_PW)
{
    config.m_Redis_USER = Redis_USER;
    config.m_Redis_URL = Redis_URL;
    config.M_Redis_PW = Redis_PW;
    return 0;
}

int SystemConfigBuilder::setKafka(const std::string& Kafka_URL, const std::string& Kafka_USER, const std::string& Kafka_PW)
{
    config.M_Kafka_USER = Kafka_USER;
    config.M_Kafka_URL = Kafka_URL;
    config.M_Kafka_PW = Kafka_PW;
    return 0;
}

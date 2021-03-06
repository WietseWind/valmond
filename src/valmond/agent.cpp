#include "agent.hpp"

// collectors
#include "collectors/all.hpp"

// crypto
#include "crypto/signature.hpp"

// common
#include "common/http.hpp"

Agent::Agent(Config* cfg) : config(cfg)
{
}

void
Agent::sendData(std::string data)
{
    // get server endpoint from config
    const std::string endpoint = config->getServerURL();
    const std::string apiUrl = endpoint + "/api/system/v2/";

    // create http instance
    Http http(apiUrl);
    Http::Response resp;

    // POST data
    resp = http.POST(data);

    if (!resp.success)
    {
        std::cout << "Send data error: " << resp.error << std::endl;
    }
}

int
Agent::beat()
{
    // collect data
    auto report = collectors::all::collectAllData();

    std::cout << "Metrics collected Interval: " << config->getHeartbeatInterval() << std::endl;

    // json string convertor
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    builder.settings_["precision"] = 2;

    // sign collected data
    auto const secret = config->getValidationSecret();
    const std::string reportStrBeforeSignature = Json::writeString(builder, report);
    auto secretKey = crypto::SecretKey(secret);
    auto signature = crypto::Signature(secretKey, reportStrBeforeSignature);

    // convert signature to string
    std::stringstream sstream;
    sstream << signature;
    std::string signatureString = sstream.str();

    // add signature to the data
    report["signature"] = signatureString;

    // convert the whole report to str
    std::string reportStr = Json::writeString(builder, report);

    // send data to the backend
    sendData(reportStr);

    return 0;
}

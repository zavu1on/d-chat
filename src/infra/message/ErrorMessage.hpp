#pragma once
#include "Message.hpp"

namespace message
{
struct ErrorMessageResponsePayload
{
    std::string error;
};

class ErrorMessageResponse : public Message
{
protected:
    ErrorMessageResponsePayload payload;

public:
    ErrorMessageResponse();
    ErrorMessageResponse(const std::string& id,
                         const peer::UserPeer& from,
                         const peer::UserPeer& to,
                         uint64_t timestamp,
                         const std::string& error);
    ErrorMessageResponse(const json& jData);

    void serialize(json& jData) const override;
    const ErrorMessageResponsePayload& getPayload() const;

    static ErrorMessageResponse create(const peer::UserPeer& from,
                                       const peer::UserPeer& to,
                                       const std::string& error);
};
}  // namespace message
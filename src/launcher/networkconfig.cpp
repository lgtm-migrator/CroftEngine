#include "networkconfig.h"

#include "paths.h"
#include "serialization/serialization.h"
#include "serialization/vector.h"
#include "serialization/yamldocument.h"

#include <algorithm>
#include <regex>

namespace launcher
{

void NetworkConfig::serialize(const serialization::Serializer<NetworkConfig>& ser)
{
  ser(S_NV("socket", socket),
      S_NV("username", username),
      S_NV("authToken", authToken),
      S_NV("sessionId", sessionId),
      S_NV("color", color));
}

NetworkConfig NetworkConfig::load()
{
  serialization::YAMLDocument<true> doc{findUserDataDir().value() / "network.yaml"};
  NetworkConfig cfg{};
  doc.load("config", cfg, cfg);
  return cfg;
}

void NetworkConfig::save()
{
  serialization::YAMLDocument<false> doc{findUserDataDir().value() / "network.yaml"};
  doc.save("config", *this, *this);
  doc.write();
}

bool NetworkConfig::isValid() const
{
  if(std::count(socket.begin(), socket.end(), ':') != 1)
    return false;
  if(username.empty())
    return false;

  static const std::regex tokenRegex{"^[a-f0-9]{32}$"};
  std::smatch matches;
  if(!std::regex_match(authToken, matches, tokenRegex))
    return false;
  if(!std::regex_match(sessionId, matches, tokenRegex))
    return false;

  return true;
}
} // namespace launcher

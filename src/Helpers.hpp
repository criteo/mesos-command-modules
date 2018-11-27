#ifndef __HELPERS_HPP__
#define __HELPERS_HPP__

#include <stout/json.hpp>
#include <stout/protobuf.hpp>
#include <stout/result.hpp>

namespace criteo {
namespace mesos {

template <class Proto>
Result<Proto> jsonToProtobuf(const std::string& output) {
  if (output.empty()) return Error("No content to parse");

  auto outputJsonTry = JSON::parse(output);
  if (outputJsonTry.isError()) {
    return Error("Malformed JSON. " + outputJsonTry.error());
  }

  auto outputJson = outputJsonTry.get();
  if (!outputJson.is<JSON::Object>()) {
    return Error("Malformed Protobuf. JSON object is expected.");
  }

  auto proto = ::protobuf::parse<Proto>(outputJson);
  if (proto.isError()) {
    return Error("Error while converting JSON to protobuf. " + proto.error());
  }
  return proto;
}
}  // namespace mesos
}  // namespace criteo

#endif

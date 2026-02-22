#include "keychain-helper.hpp"

#ifndef __APPLE__

// Stub implementations for non-macOS platforms.
// These are no-ops; stream keys fall back to plaintext config storage.

namespace keychain {

std::string make_service_name(const std::string &output_name)
{
	return std::string(KEYCHAIN_SERVICE_PREFIX) + output_name;
}

bool store_secret(const std::string &, const std::string &, const std::string &)
{
	return false;
}

std::string retrieve_secret(const std::string &, const std::string &)
{
	return "";
}

bool delete_secret(const std::string &, const std::string &)
{
	return false;
}

} // namespace keychain

#endif // !__APPLE__

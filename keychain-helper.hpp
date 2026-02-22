#pragma once

#include <string>

// Service name prefix for keychain entries
#define KEYCHAIN_SERVICE_PREFIX "com.aitum.multistream."

namespace keychain {

/**
 * Store a secret (stream key) in the macOS Keychain.
 * On non-macOS platforms, this is a no-op that returns false.
 *
 * @param service_name  The keychain service name (e.g. "com.aitum.multistream.Twitch")
 * @param account       The account name (e.g. the output name)
 * @param secret        The secret to store (e.g. the stream key)
 * @return true on success, false on failure
 */
bool store_secret(const std::string &service_name, const std::string &account, const std::string &secret);

/**
 * Retrieve a secret (stream key) from the macOS Keychain.
 * On non-macOS platforms, this is a no-op that returns an empty string.
 *
 * @param service_name  The keychain service name
 * @param account       The account name
 * @return The secret string, or empty string on failure
 */
std::string retrieve_secret(const std::string &service_name, const std::string &account);

/**
 * Delete a secret from the macOS Keychain.
 * On non-macOS platforms, this is a no-op that returns false.
 *
 * @param service_name  The keychain service name
 * @param account       The account name
 * @return true on success, false on failure/not found
 */
bool delete_secret(const std::string &service_name, const std::string &account);

/**
 * Build a keychain service name from an output name.
 * Returns "com.aitum.multistream.<output_name>"
 */
std::string make_service_name(const std::string &output_name);

/**
 * Build a keychain service name for a vertical canvas output.
 * Returns "com.aitum.multistream.vertical.<output_name>"
 */
std::string make_vertical_service_name(const std::string &output_name);

} // namespace keychain

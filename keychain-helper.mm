#include "keychain-helper.hpp"

#ifdef __APPLE__

#include <Security/Security.h>
#include <CoreFoundation/CoreFoundation.h>
#include <obs.h>

namespace keychain {

std::string make_service_name(const std::string &output_name)
{
	return std::string(KEYCHAIN_SERVICE_PREFIX) + output_name;
}

std::string make_vertical_service_name(const std::string &output_name)
{
	return std::string(KEYCHAIN_SERVICE_PREFIX) + "vertical." + output_name;
}

bool store_secret(const std::string &service_name, const std::string &account, const std::string &secret)
{
	if (service_name.empty() || secret.empty())
		return false;

	CFStringRef cf_service =
		CFStringCreateWithCString(kCFAllocatorDefault, service_name.c_str(), kCFStringEncodingUTF8);
	CFStringRef cf_account = CFStringCreateWithCString(kCFAllocatorDefault, account.c_str(), kCFStringEncodingUTF8);
	CFDataRef cf_secret = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)secret.c_str(), (CFIndex)secret.length());

	if (!cf_service || !cf_account || !cf_secret) {
		if (cf_service)
			CFRelease(cf_service);
		if (cf_account)
			CFRelease(cf_account);
		if (cf_secret)
			CFRelease(cf_secret);
		return false;
	}

	// First try to update an existing item
	const void *query_keys[] = {kSecClass, kSecAttrService, kSecAttrAccount};
	const void *query_values[] = {kSecClassGenericPassword, cf_service, cf_account};
	CFDictionaryRef query =
		CFDictionaryCreate(kCFAllocatorDefault, query_keys, query_values, 3, &kCFTypeDictionaryKeyCallBacks,
				   &kCFTypeDictionaryValueCallBacks);

	const void *update_keys[] = {kSecValueData};
	const void *update_values[] = {cf_secret};
	CFDictionaryRef update = CFDictionaryCreate(kCFAllocatorDefault, update_keys, update_values, 1,
						    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

	OSStatus status = SecItemUpdate(query, update);
	CFRelease(update);

	if (status == errSecItemNotFound) {
		// Item doesn't exist yet, add it
		CFRelease(query);
		const void *add_keys[] = {kSecClass, kSecAttrService, kSecAttrAccount, kSecValueData};
		const void *add_values[] = {kSecClassGenericPassword, cf_service, cf_account, cf_secret};
		CFDictionaryRef add_dict =
			CFDictionaryCreate(kCFAllocatorDefault, add_keys, add_values, 4, &kCFTypeDictionaryKeyCallBacks,
					   &kCFTypeDictionaryValueCallBacks);

		status = SecItemAdd(add_dict, nullptr);
		CFRelease(add_dict);
	} else {
		CFRelease(query);
	}

	CFRelease(cf_service);
	CFRelease(cf_account);
	CFRelease(cf_secret);

	if (status != errSecSuccess) {
		blog(LOG_WARNING, "[Aitum Multistream] Keychain store failed for service '%s': OSStatus %d",
		     service_name.c_str(), (int)status);
		return false;
	}

	blog(LOG_DEBUG, "[Aitum Multistream] Keychain: stored secret for service '%s'", service_name.c_str());
	return true;
}

std::string retrieve_secret(const std::string &service_name, const std::string &account)
{
	if (service_name.empty())
		return "";

	CFStringRef cf_service =
		CFStringCreateWithCString(kCFAllocatorDefault, service_name.c_str(), kCFStringEncodingUTF8);
	CFStringRef cf_account = CFStringCreateWithCString(kCFAllocatorDefault, account.c_str(), kCFStringEncodingUTF8);

	if (!cf_service || !cf_account) {
		if (cf_service)
			CFRelease(cf_service);
		if (cf_account)
			CFRelease(cf_account);
		return "";
	}

	const void *query_keys[] = {kSecClass, kSecAttrService, kSecAttrAccount, kSecReturnData, kSecMatchLimit};
	const void *query_values[] = {kSecClassGenericPassword, cf_service, cf_account, kCFBooleanTrue, kSecMatchLimitOne};
	CFDictionaryRef query =
		CFDictionaryCreate(kCFAllocatorDefault, query_keys, query_values, 5, &kCFTypeDictionaryKeyCallBacks,
				   &kCFTypeDictionaryValueCallBacks);

	CFTypeRef result = nullptr;
	OSStatus status = SecItemCopyMatching(query, &result);
	CFRelease(query);
	CFRelease(cf_service);
	CFRelease(cf_account);

	if (status != errSecSuccess || !result) {
		if (status != errSecItemNotFound) {
			blog(LOG_WARNING, "[Aitum Multistream] Keychain retrieve failed for service '%s': OSStatus %d",
			     service_name.c_str(), (int)status);
		}
		if (result)
			CFRelease(result);
		return "";
	}

	CFDataRef data = (CFDataRef)result;
	std::string secret((const char *)CFDataGetBytePtr(data), (size_t)CFDataGetLength(data));
	CFRelease(result);

	blog(LOG_DEBUG, "[Aitum Multistream] Keychain: retrieved secret for service '%s'", service_name.c_str());
	return secret;
}

bool delete_secret(const std::string &service_name, const std::string &account)
{
	if (service_name.empty())
		return false;

	CFStringRef cf_service =
		CFStringCreateWithCString(kCFAllocatorDefault, service_name.c_str(), kCFStringEncodingUTF8);
	CFStringRef cf_account = CFStringCreateWithCString(kCFAllocatorDefault, account.c_str(), kCFStringEncodingUTF8);

	if (!cf_service || !cf_account) {
		if (cf_service)
			CFRelease(cf_service);
		if (cf_account)
			CFRelease(cf_account);
		return false;
	}

	const void *query_keys[] = {kSecClass, kSecAttrService, kSecAttrAccount};
	const void *query_values[] = {kSecClassGenericPassword, cf_service, cf_account};
	CFDictionaryRef query =
		CFDictionaryCreate(kCFAllocatorDefault, query_keys, query_values, 3, &kCFTypeDictionaryKeyCallBacks,
				   &kCFTypeDictionaryValueCallBacks);

	OSStatus status = SecItemDelete(query);
	CFRelease(query);
	CFRelease(cf_service);
	CFRelease(cf_account);

	if (status != errSecSuccess && status != errSecItemNotFound) {
		blog(LOG_WARNING, "[Aitum Multistream] Keychain delete failed for service '%s': OSStatus %d",
		     service_name.c_str(), (int)status);
		return false;
	}

	blog(LOG_DEBUG, "[Aitum Multistream] Keychain: deleted secret for service '%s'", service_name.c_str());
	return true;
}

} // namespace keychain

#endif // __APPLE__

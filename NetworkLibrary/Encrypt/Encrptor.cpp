#pragma comment(lib,"libsodium.lib")
#define SODIUM_STATIC
#include "Encrptor.h"
#include "OS/MyWindow.h"
#include "DebugTool/Log.h"
#include "libsodium/include/sodium.h"
Encrptor::Encrptor():_rnGenerator(_rd())
{
}

std::string Encrptor::MakeRandomString()
{
	std::string loginToken(64, '\0');
	uint64_t randomValue = GetTickCount64();
	size_t i = 0;
	while (i < loginToken.size()) {
		randomValue ^= _dis(_rnGenerator);
		std::memcpy(&loginToken[i], &randomValue, sizeof(randomValue));
		i += sizeof(randomValue);
	}
	char nonZeroByte = 0x01 | randomValue;
	for (int i = 0; i < loginToken.size() - 1; i++)
	{
		if (loginToken[i] == '\0')
		{
			loginToken[i] = nonZeroByte;
		}
	}
	return loginToken;
}

std::string Encrptor::HashWithSalt(const std::string& str, const std::string& salt)
{
	std::string combined = str + salt;
	unsigned char hashedPassword[crypto_hash_sha512_BYTES];
	// 패스워드를 해시
	if (crypto_hash_sha512(hashedPassword,reinterpret_cast<const unsigned char*>(combined.c_str()),combined.size())!=0)
	{
		Log::LogOnFile(Log::SYSTEM_LEVEL,"sha512 error\n");
		DebugBreak();
	}
	std::string encoded(sodium_base64_encoded_len(crypto_hash_sha512_BYTES, sodium_base64_VARIANT_ORIGINAL), '\0');
	sodium_bin2base64(encoded.data(), encoded.size(), hashedPassword, crypto_hash_sha512_BYTES, sodium_base64_VARIANT_ORIGINAL);
	return encoded;
}

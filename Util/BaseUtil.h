#pragma once
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <string>
#include <string_view>
namespace HS {
	namespace Util {
		const auto CONTROLBUFFERMAXSIZE = 125;
		inline bool getFin(unsigned char *frame) { return frame[0] & 128; }
		inline void setFin(unsigned char *frame, unsigned char val) { frame[0] = (val & 128) | (~128 & frame[0]); }

		inline bool getMask(unsigned char *frame) { return frame[1] & 128; }
		inline void setMask(unsigned char *frame, unsigned char val) { frame[1] = (val & 128) | (~128 & frame[1]); }
		inline unsigned char getpayloadLength1(unsigned char *frame) { return frame[1] & 127; }
		inline unsigned short getpayloadLength2(unsigned char *frame) { return *reinterpret_cast<unsigned short *>(frame + 2); }
		inline unsigned long long int getpayloadLength8(unsigned char *frame) { return *reinterpret_cast<unsigned long long int *>(frame + 2); }

		inline void setpayloadLength1(unsigned char *frame, unsigned char val) { frame[1] = (val & 127) | (~127 & frame[1]); }
		inline void setpayloadLength2(unsigned char *frame, unsigned short val) { *reinterpret_cast<unsigned short *>(frame + 2) = val; }
		inline void setpayloadLength8(unsigned char *frame, unsigned long long int val) { *reinterpret_cast<unsigned long long int *>(frame + 2) = val; }
		inline unsigned char getOpCode(unsigned char *frame) { return *frame & 15; }
		inline void setOpCode(unsigned char *frame, unsigned char val) { frame[0] = (val & 15) | (~15 & frame[0]); }
		inline bool getrsv3(unsigned char *frame) { return *frame & 16; }
		inline bool getrsv2(unsigned char *frame) { return *frame & 32; }
		inline bool getrsv1(unsigned char *frame) { return *frame & 64; }
		inline void setrsv3(unsigned char *frame, unsigned char val) { frame[0] = (val & 16) | (~16 & frame[0]); }
		inline void setrsv2(unsigned char *frame, unsigned char val) { frame[0] = (val & 32) | (~32 & frame[0]); }
		inline void setrsv1(unsigned char *frame, unsigned char val) { frame[0] = (val & 64) | (~64 & frame[0]); }
		inline bool DidPassMaskRequirement(unsigned char *h, bool isServer)
		{
			if (isServer) {
				return getMask(h);
			}
			else {
				return !getMask(h);
			}
		}
		inline size_t AdditionalBodyBytesToRead(bool isServer)
		{
			if (isServer) {
				return 4;
			}
			else {
				return 0;
			}
		}
		template <class type> void SHA1(const type &input, type &hash)
		{
			SHA_CTX context;
			SHA1_Init(&context);
			SHA1_Update(&context, &input[0], input.size());

			hash.resize(160 / 8);
			SHA1_Final((unsigned char *)&hash[0], &context);
		}
		template <class type> type SHA1(const type &input)
		{
			type hash;
			SHA1(input, hash);
			return hash;
		}
		inline void set_MaskBitForSending(unsigned char *frame, bool isServer)
		{
			if (isServer) {
				setMask(frame, 0x00);
			}
			else {
				setMask(frame, 0xff);
			}
		}
		bool isValidUtf8(unsigned char *s, size_t length);
		template <typename T> T ntoh(T u)
		{
			static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
			union {
				T u;
				unsigned char u8[sizeof(T)];
			} source, dest;
			source.u = u;
			for (size_t k = 0; k < sizeof(T); k++)
				dest.u8[k] = source.u8[sizeof(T) - k - 1];

			return dest.u;
		}
		template <typename T> T hton(T u)
		{
			static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
			union {
				T u;
				unsigned char u8[sizeof(T)];
			} source, dest;
			source.u = u;
			for (size_t k = 0; k < sizeof(T); k++)
				dest.u8[sizeof(T) - k - 1] = source.u8[k];

			return dest.u;
		}
		template <class type> type Base64encode(const type &ascii)
		{
			BIO *bio, *b64;
			BUF_MEM *bptr;

			b64 = BIO_new(BIO_f_base64());
			BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
			bio = BIO_new(BIO_s_mem());
			BIO_push(b64, bio);
			BIO_get_mem_ptr(b64, &bptr);

			// Write directly to base64-buffer to avoid copy
			int base64_length = static_cast<int>(round(4 * ceil((double)ascii.size() / 3.0)));
			type base64;
			base64.resize(base64_length);
			bptr->length = 0;
			bptr->max = base64_length + 1;
			bptr->data = (char *)&base64[0];

			BIO_write(b64, &ascii[0], static_cast<int>(ascii.size()));
			BIO_flush(b64);

			// To keep &base64[0] through BIO_free_all(b64)
			bptr->length = 0;
			bptr->max = 0;
			bptr->data = nullptr;

			BIO_free_all(b64);
			return base64;
		}
		template <class type> std::string Base64decode(const type &base64)
		{
			// Resize ascii, however, the size is a up to two bytes too large.
			std::string ascii;
			ascii.resize((6 * base64.size()) / 8);
			BIO *b64, *bio;

			b64 = BIO_new(BIO_f_base64());
			BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
			bio = BIO_new_mem_buf((char *)&base64[0], static_cast<int>(base64.size()));
			bio = BIO_push(b64, bio);

			int decoded_length = BIO_read(bio, &ascii[0], static_cast<int>(ascii.size()));
			ascii.resize(decoded_length);

			BIO_free_all(b64);
			return ascii;
		}
	}
}

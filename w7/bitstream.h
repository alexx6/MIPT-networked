#include <vector>
#include <cstring>


class Bitstream
{
public:

	Bitstream()
	{
		bits = std::vector<char>();
	}

	template<typename Type>
	Bitstream(const Type* data, size_t size)
	{
		bits = std::vector<char>(size);
		memcpy(bits.data(), data, size);
	}

	template<typename Type>
	void write(const Type& val)
	{
		bits.resize(bits.size() + sizeof(Type));
		memcpy(bits.data() + pos, &val, sizeof(Type));
		pos += sizeof(Type);
	}

	template<typename Type>
	void read(Type& val)
	{
		memcpy(&val, bits.data() + pos, sizeof(Type));
		pos += sizeof(Type);
	}

	char* get()
	{
		return bits.data();
	}

	size_t size()
	{
		return bits.size();
	}
	
	void writePackedUint32(uint32_t val)
	{
		//0 - 8, 1 - 16, 2 - 32
		uint8_t type = val < (1 << 6) ? 0 :
					   val < (1 << 14) ? 1 : 2;
		//Setting two highest bits of first byte to type
		uint8_t b = type << 6;
		//Writing every byte (1, 2 or 4)
		for (int s = (1 << type) - 1; s >= 0; --s)
		{
			//Adding each byte to b and then writing it. The first written byte includes type in two highest bits
			b += val >> 8 * s & 0xff;
			write<uint8_t>(b);
			b = 0;
		}
	}

	void readPackedUint32(uint32_t& val)
	{
		//Clearing resulting value
		val = 0;
		uint8_t b;
		//Reading first byte
		read<uint8_t>(b);
		//Getting type
		uint8_t type = b >> 6;
		//adding first byte to result (without type)
		val += b & 0b00111111;
		//For remaining bytes we shift result and add each byte to it
		for (int s = 1; s < 1 << type; ++s)
		{
			val <<= 8;
			read<uint8_t>(b);
			val += b;
		}
	}

private:

	std::vector<char> bits;
	size_t pos = 0;

};
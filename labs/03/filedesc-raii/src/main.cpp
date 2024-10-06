#include <iostream>
#include <unistd.h>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <cstddef>
#include <fcntl.h>
#include <cstdlib>

class FileDesc
{
	constexpr static int InvalidDesc = -1;
public:
	FileDesc() = default;

	explicit FileDesc(int desc)
		: m_desc(desc == InvalidDesc || desc >= 0
			? desc
			: throw std::invalid_argument("Invalid file descriptor"))
	{
	}

	FileDesc(const FileDesc&) = delete;
	FileDesc& operator=(const FileDesc&) = delete;

	FileDesc(FileDesc&& other) noexcept
		: m_desc(std::exchange(other.m_desc, InvalidDesc))
	{
	}

	FileDesc& operator=(FileDesc&& rhs)
	{
		if (this != &rhs)
		{
			Swap(rhs);
			rhs.Close();
		}
		return *this;
	}

	~FileDesc()
	{
		try
		{
			Close();
		}
		catch (...)
		{
		}
	}

	void Swap(FileDesc& other)
	{
		std::swap(m_desc, other.m_desc);
	}

	bool IsOpen() const noexcept
	{
		return m_desc != InvalidDesc;
	}

	void Close()
	{
		if (IsOpen())
		{
			if (close(m_desc) != 0)
			{
				throw std::system_error(errno, std::generic_category());
			}
			m_desc = InvalidDesc;
		}
	}

	void Open(const char* pathName, int flags)
	{
		Close();
		if ((m_desc = open(pathName, flags)) == -1)
		{
			throw std::system_error(errno, std::generic_category());
		}
	}

	size_t Read(void* buffer, size_t length)
	{
		EnsureOpen();
		if (auto bytesRead = read(m_desc, buffer, length); bytesRead != -1)
		{
			return static_cast<size_t>(bytesRead);
		}
		throw std::system_error(errno, std::generic_category());
	}

	size_t Write(const void* buffer, size_t length)
	{
		EnsureOpen();
		if (auto bytesWritten = write(m_desc, buffer, length); bytesWritten != -1)
		{
			return static_cast<size_t>(bytesWritten);
		}
		throw std::system_error(errno, std::generic_category());
	}
private:
	void EnsureOpen()
	{
		if (!IsOpen())
		{
			throw std::logic_error("File is not open");
		}
	}
	int m_desc = InvalidDesc;
};

int main(int argc, const char* argv[])
{
	if (argc == 1)
	{
		return EXIT_FAILURE;
	}
	try
	{
		FileDesc x;
		x.Open(argv[1], O_RDONLY);

		char buffer[10];
		size_t readBytes = x.Read(buffer, sizeof(buffer) - 1);
		buffer[readBytes] = '\0';
		std::cout << buffer << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}

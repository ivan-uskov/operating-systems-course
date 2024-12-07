#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace
{

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

	int get() const noexcept { return m_desc; }

	size_t Read(void* buffer, size_t length)
	{
		EnsureOpen();
		if (auto bytesRead = read(m_desc, buffer, length); bytesRead != -1)
		{
			return static_cast<size_t>(bytesRead);
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

class Socket
{
public:
	explicit Socket(FileDesc fd)
		: m_fd{ std::move(fd) }
	{
	}

	size_t Read(void* buffer, size_t length)
	{
		return m_fd.Read(buffer, length);
	}

	size_t Send(const void* buffer, size_t len, int flags)
	{
		const auto result = send(m_fd.get(), buffer, len, flags);
		if (result == -1)
		{
			throw std::system_error(errno, std::generic_category());
		}
		return result;
	}

private:
	FileDesc m_fd;
};

class Acceptor
{
public:
	Acceptor(const sockaddr_in& addr, int queueSize)
	{
		if (bind(m_fd.get(), reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
		{
			throw std::system_error(errno, std::generic_category());
		}
		if (listen(m_fd.get(), queueSize) != 0)
		{
			throw std::system_error(errno, std::generic_category());
		}
	}

	// Ждёт первое подключение к сокету
	// https://man7.org/linux/man-pages/man2/accept.2.html
	Socket Accept()
	{
		sockaddr_in clientAddress{};
		socklen_t clientLength = sizeof(clientAddress);
		FileDesc clientFd{ accept(m_fd.get(), reinterpret_cast<sockaddr*>(&clientAddress), &clientLength) };
		return Socket{ std::move(clientFd) };
	}

private:
	FileDesc m_fd{ socket(AF_INET, SOCK_STREAM, /*protocol*/ 0) };
};

constexpr int Port = 8080;
constexpr size_t BufferSize = 1024;

} // namespace

int main()
{
	try
	{
		sockaddr_in serverAddr{
			.sin_family = AF_INET,
			.sin_port = htons(Port),
			.sin_addr = { .s_addr = INADDR_ANY },
			// The sin_port and sin_addr members are stored in network byte order.
		};

		Acceptor acceptor{ serverAddr, 5 };

		std::cout << "Listening to the port " << Port << std::endl;

		while (true)
		{
			std::cout << "Accepting" << std::endl;
			auto clientSocket = acceptor.Accept();
			std::cout << "Accepted" << std::endl;
			char buffer[BufferSize];
			for (size_t bytesRead; (bytesRead = clientSocket.Read(&buffer, sizeof(buffer))) > 0;)
			{
				clientSocket.Send(buffer, bytesRead, 0);
			}
			std::cout << "Client disconnected" << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}

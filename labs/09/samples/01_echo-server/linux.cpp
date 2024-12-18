#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <variant>

namespace
{

namespace fs = std::filesystem;

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

struct ClientMode
{
	std::string address;
	uint16_t port;
};

struct ServerMode
{
	uint16_t port;
};

struct HelpMode
{
};

using ProgramMode = std::variant<HelpMode, ClientMode, ServerMode>;

constexpr uint16_t Port = 8080;
constexpr size_t BufferSize = 1024;

ProgramMode ParseCommandLine(int argc, char* argv[])
{
	if (argc < 2)
	{
		throw std::runtime_error(
			"Invalid command line. Type " + fs::path(argv[0]).filename().string() + " -h for help.");
	}

	if (argc == 2 && std::string(argv[1]) == "-h")
	{
		return HelpMode{};
	}

	if (std::string(argv[1]) == "server")
	{
		if (argc != 3)
		{
			throw std::runtime_error("Invalid server command line parameters");
		}
		unsigned long port = std::stoul(argv[2]);
		if (port < 1 || port >= std::numeric_limits<uint16_t>::max())
		{
			throw std::runtime_error("Invalid port");
		}
		return ServerMode{ .port = static_cast<uint16_t>(port) };
	}

	if (std::string(argv[1]) == "client")
	{
		if (argc != 4)
		{
			throw std::runtime_error("Invalid client command line parameters");
		}
		unsigned long port = std::stoul(argv[3]);
		if (port < 1 || port >= std::numeric_limits<uint16_t>::max())
		{
			throw std::runtime_error("Invalid port");
		}
		return ClientMode{ .address = argv[2], .port = static_cast<uint16_t>(port) };
		throw std::runtime_error("Invalid command line");
	}
	throw std::runtime_error("Invalid server command line parameters");
}

void Run(HelpMode)
{
}

void Run(const ServerMode& mode)
{
	sockaddr_in serverAddr{
		.sin_family = AF_INET,
		.sin_port = htons(mode.port),
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

void Run(const ClientMode& mode)
{
	
}

} // namespace

int main(int argc, char* argv[])
{
	try
	{
		auto mode = ParseCommandLine(argc, argv);
		std::visit([](const auto& mode) { Run(mode); }, mode);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}

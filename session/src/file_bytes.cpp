#include <unison/session/file_bytes.hpp>

#include <fstream>
#include <ios>
#include <string_view>

namespace unison::session
{

namespace
{

tl::unexpected<Error> unavailable(std::string_view reason)
{
    return tl::unexpected{Error{ErrorCode::FileUnavailable, reason}};
}

}

tl::expected<void, Error> writeFileBytes(const std::filesystem::path& path, std::span<const std::byte> bytes)
{
    std::ofstream file{path, std::ios::binary | std::ios::trunc};

    if (!file.is_open())
    {
        return unavailable("the file cannot be opened for writing");
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    file.close();

    if (file.fail())
    {
        return unavailable("the file could not be written whole");
    }

    return {};
}

tl::expected<std::vector<std::byte>, Error> readFileBytes(const std::filesystem::path& path)
{
    std::ifstream file{path, std::ios::binary | std::ios::ate};

    if (!file.is_open())
    {
        return unavailable("the file cannot be opened for reading");
    }

    const std::streamoff size = file.tellg();

    if (size < 0)
    {
        return unavailable("the file's size cannot be read");
    }

    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    if (file.fail())
    {
        return unavailable("the file could not be read whole");
    }

    return bytes;
}

}

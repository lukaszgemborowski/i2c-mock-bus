#include <iostream>
#include <vector>
#include <fstream>
#include <cstdint>
#include <sstream>

struct i2c_message {
    std::uint16_t addr;
    std::uint16_t len;
    std::vector<std::uint8_t> data;
};

class i2c_mock
{
public:
    i2c_mock(const std::string &i2c_path) :
        datastream_path_(create_datastream_path(i2c_path)),
        response_path_(create_response_path(i2c_path))
    {
    }

    i2c_message read() const {
        std::fstream f(datastream_path_);
        i2c_message result;

        f.read(reinterpret_cast<char *>(&result.addr), sizeof(i2c_message::addr));
        f.read(reinterpret_cast<char *>(&result.len), sizeof(i2c_message::len));
        
        for (int i = 0; i < result.len; i ++)
            result.data.push_back(f.get());

        return result;
    }

    bool have_data() const { return true; }

private:
    std::string join_strings(const std::string &first, const std::string &second)
    {
        std::stringstream ss (first);

        ss << second;
        return ss.str();
    }

    std::string create_datastream_path(const std::string &base_path)
    {
        return join_strings(base_path, "/datastream");
    }

    std::string create_response_path(const std::string &base_path)
    {
        return join_strings(base_path, "/response");
    }

private:
    std::string datastream_path_;
    std::string response_path_;
    std::vector<unsigned char> datastream_;
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "You need to provide path to sysfs i2c entries" << std::endl;
        return -1;
    }

    i2c_mock mock(argv[1]);

    return 0;
}


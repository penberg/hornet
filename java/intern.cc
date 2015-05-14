#include "hornet/java.hh"

#include <unordered_map>
#include <mutex>

namespace hornet
{

std::mutex _intern_mutex;
std::unordered_map<std::string, std::shared_ptr<string>> _intern;

string* intern_string(std::string str)
{
    std::lock_guard<std::mutex> lock(_intern_mutex);
    auto it = _intern.find(str);
    if (it == _intern.end()) {
        auto intern = std::make_shared<string>(str.c_str());
        _intern.insert({str, intern});
        return intern.get();
    }
    return it->second.get();
}

}

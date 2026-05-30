#include "env.h"
#include <fstream>
#include <unordered_map>
#include <QDir>

static std::unordered_map<std::string, std::string> loadEnv()
{
    // i hate cmake i hate cmake i hate cmake i hate cmake
    std::unordered_map<std::string, std::string> map;
    QDir dir(PROJECT_SOURCE_DIR);
    std::ifstream file(dir.absoluteFilePath(".env").toStdString());
    if (!file.is_open())
        return map;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        auto eq = line.find('=');
        if (eq == std::string::npos)
            continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        map[key] = val;
    }
    return map;
}

std::string getEnv(const std::string &key, const std::string &defaultValue)
{
    static const auto env = loadEnv();
    auto it = env.find(key);
    if (it != env.end())
        return it->second;
    return defaultValue;
}

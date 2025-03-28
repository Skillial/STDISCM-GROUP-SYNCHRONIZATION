#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <cctype>
#include <thread>
#include <vector>
#include <random>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <unordered_map>

using namespace std;

mutex mtx;
condition_variable cv;
map<uint32_t, atomic<bool>> dungeonMap;
map<uint32_t, uint32_t> dungeonUsage;
map<uint32_t, uint32_t> dungeonTimeUsed;

const set<string> validKeys = {"n", "t", "h", "d", "t1", "t2"};

bool isValidUInt(const string &str)
{
    if (str.empty())
        return false;
    for (char ch : str)
    {
        if (!isdigit(ch))
            return false;
    }
    return true;
}

bool readConfig(map<string, uint32_t> &config, const string &filename)
{
    ifstream inputFile(filename);
    if (!inputFile)
    {
        cerr << "Error: Could not open " << filename << endl;
        return false;
    }

    set<string> seenKeys;
    string line, key, valueStr, extra;
    while (getline(inputFile, line))
    {
        istringstream iss(line);
        if (!(iss >> key >> valueStr) || !isValidUInt(valueStr) || (iss >> extra) || validKeys.find(key) == validKeys.end())
        {
            cerr << "Error: Invalid line format, key, or value -> " << line << endl;
            return false;
        }

        if (!seenKeys.insert(key).second)
        {
            cerr << "Error: Duplicate key detected -> " << key << endl;
            return false;
        }
        try
        {
            config[key] = stoul(valueStr);
        }
        catch (const out_of_range &e)
        {
            cerr << "Error: Out of range -> " << valueStr << endl;
            return false;
        }
    }
    inputFile.close();

    if (config.size() != validKeys.size())
    {
        cerr << "Error: Missing required keys." << endl;
        return false;
    }

    if (config["t2"] < config["t1"])
    {
        cerr << "Error: t2 cannot be less than t1." << endl;
        return false;
    }
    if (config["t2"] == 0 || config["t1"] == 0)
    {
        cerr << "Error: t1 and t2 cannot be zero." << endl;
        return false;
    }
    if (config["t2"] > 15)
    {
        cerr << "Error: t2 cannot be greater than 15." << endl;
        return false;
    }

    return true;
}

void printStatus()
{
    cout << "-----------------------------------" << endl;
    cout << "Dungeon Status: " << endl;
    for (const auto &pair : dungeonMap)
    {
        cout << "Dungeon Instance " << pair.first << " is " << (pair.second ? "active" : "empty") << endl;
    }
}

void dungeonInstance(uint32_t id, uint32_t duration, atomic<uint32_t> &totalServed, atomic<uint32_t> &totalTime)
{
    {
        lock_guard<mutex> lock(mtx);

        totalServed++;
        totalTime += duration;
        dungeonUsage[id]++;
        dungeonTimeUsed[id] += duration;
    }

    this_thread::sleep_for(chrono::seconds(duration));

    {
        lock_guard<mutex> lock(mtx);
        dungeonMap[id] = false;
        printStatus();
    }
    cv.notify_one();
}

int main()
{
    map<string, uint32_t> config;
    if (!readConfig(config, "input.txt"))
    {
        return 1;
    }

    uint32_t numParties = min(min(config["t"], config["h"]), static_cast<uint32_t>(floor(config["d"] / 3)));
    cout << "Number of parties: " << numParties << endl;
    if (numParties == 0)
    {
        cerr << "Error: Not enough players to form a single party." << endl;
        return 1;
    }

    atomic<uint32_t> totalServed(0);
    atomic<uint32_t> totalTime(0);

    vector<thread> activeThreads;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(config["t1"], config["t2"]);

    uint32_t threadsToSpawn = min(config["n"], numParties);

    for (uint32_t i = 1; i <= threadsToSpawn; i++)
    {
        dungeonMap[i] = false;
        dungeonUsage[i] = 0;
        dungeonTimeUsed[i] = 0;
    }


    uint32_t lastUsedDungeon = -1;
    for (uint32_t i = 0; i < numParties; i++)
    {
        uint32_t duration = dist(gen);

        unique_lock<mutex> lock(mtx);
        cv.wait(lock, []
                {
            for (const auto& pair : dungeonMap) {
                if (!pair.second) return true;
            }
            return false; });

        uint32_t dungeonID = lastUsedDungeon;
        for (uint32_t j = 1; j <= threadsToSpawn; j++)
        {
            uint32_t index = ((lastUsedDungeon + j) % threadsToSpawn) + 1; 
            if (!dungeonMap[index])
            {
                dungeonID = index;
                lastUsedDungeon = index;
                dungeonMap[index] = true;
                break;
            }
        }
        printStatus();
        activeThreads.emplace_back(dungeonInstance, dungeonID, duration, ref(totalServed), ref(totalTime));
        lock.unlock();
    }

    for (auto &th : activeThreads)
    {
        if (th.joinable())
            th.join();
    }
    cout <<"-----------------------------------" << endl;
    cout <<"-----------------------------------" << endl;
    cout << "Summary:" << endl;
    cout << "Total parties served: " << totalServed.load() << endl;
    cout << "Total time served: " << totalTime.load() << " seconds" << endl;
    cout <<"-----------------------------------" << endl;
    cout << "Dungeon Usage Count:" << endl;
    for (const auto& pair : dungeonUsage) {
        cout << "Dungeon Instance " << pair.first << " was used " << pair.second << " times." << endl;
    }
    cout <<"-----------------------------------" << endl;
    cout << "Dungeon Time Used:" << endl;
    for (const auto& pair : dungeonTimeUsed) {
        cout << "Dungeon Instance " << pair.first << " was active for " << pair.second << " seconds." << endl;
    }
    cout <<"-----------------------------------" << endl;
    cout << "DPS without a party: " << config["d"] - (numParties*3) << endl;
    cout << "Healer without a party: " << config["h"] - numParties << endl;
    cout << "Tanks without a party: " << config["t"] - numParties << endl;

    return 0;
}

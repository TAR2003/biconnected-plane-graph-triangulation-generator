// BenchCommon.hpp - shared helpers for bench_time (Google Benchmark) and
// bench_memory (peak-memory runner). Does NOT modify your algorithm code.
#pragma once
#include "GraphTriangulation.hpp" // pulls in all three algorithm variants

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifdef __linux__
#include <sched.h>
#endif

namespace tb
{
    namespace fs = std::filesystem;

    // ------------------------------------------------------------------ algos
    enum class Algo
    {
        Biconnected,
        BiconnectedWithoutVGS,
        Oneconnected
    };

    inline const char *algoName(Algo a)
    {
        switch (a)
        {
        case Algo::Biconnected:
            return "Biconnected";
        case Algo::BiconnectedWithoutVGS:
            return "BiconnectedWithoutVGS";
        case Algo::Oneconnected:
            return "Oneconnected";
        }
        return "?";
    }

    inline bool parseAlgo(const std::string &s, Algo &out)
    {
        for (Algo a : {Algo::Biconnected, Algo::BiconnectedWithoutVGS, Algo::Oneconnected})
            if (s == algoName(a))
            {
                out = a;
                return true;
            }
        return false;
    }

    // ------------------------------------------------------------------ input
    struct InputGraph
    {
        long long vertexCount = 0;
        std::vector<std::vector<long long>> adjacency;
    };

    // Same file format as your original driver. vertexCount == 0 means invalid.
    inline InputGraph readInput(const std::string &filename)
    {
        std::ifstream in(filename);
        InputGraph g;
        if (!in.is_open() || !(in >> g.vertexCount) || g.vertexCount <= 0)
            return {};
        g.adjacency.resize(g.vertexCount);
        for (auto &nb : g.adjacency)
        {
            long long deg;
            if (!(in >> deg) || deg < 0)
                return {};
            nb.resize(deg);
            for (auto &v : nb)
                if (!(in >> v))
                    return {};
        }
        return g;
    }

    // "Performance" variants: storeTriangulation() is a no-op, exactly like
    // your original timing driver, so we measure the algorithm, not storage.
    inline std::unique_ptr<GraphTriangulation> makeGraph(Algo a, const InputGraph &g, long long limit)
    {
        switch (a)
        {
        case Algo::Biconnected:
            return std::make_unique<GraphTriangulationBiconnectedPerformance>(g.vertexCount, g.adjacency, limit);
        case Algo::BiconnectedWithoutVGS:
            return std::make_unique<GraphTriangulationBiconnectedWithoutVGSPerformance>(g.vertexCount, g.adjacency, limit);
        case Algo::Oneconnected:
            return std::make_unique<GraphTriangulationOneconnectedPerformance>(g.vertexCount, g.adjacency, limit);
        }
        return nullptr;
    }

    inline bool limitHit(const GraphTriangulation &gt, long long limit)
    {
        return gt.totalTriangulations >= limit;
    }
    inline const char *statusString(bool hit)
    {
        return hit ? "Triangulation Limit exceeded" : "Completed";
    }

    // ----------------------------------------------------------------- config
    struct Config
    {
        std::string inputRoot = "input";
        std::string dirBiconnected = "Biconnected";   // used by Biconnected + WithoutVGS
        std::string dirOneconnected = "Oneconnected"; // used by Oneconnected
        std::vector<long long> limits{10, 100, 1000, 10000, 100000, 1000000};
        std::vector<Algo> algos{Algo::Biconnected, Algo::BiconnectedWithoutVGS, Algo::Oneconnected};
        std::vector<std::string> categories; // empty = all
        std::string csv;
        int cpu = -1;
    };

    inline std::vector<std::string> splitList(const std::string &s, char sep = ',')
    {
        std::vector<std::string> out;
        std::string cur;
        std::stringstream ss(s);
        while (std::getline(ss, cur, sep))
            if (!cur.empty())
                out.push_back(cur);
        return out;
    }

    inline bool matchFlag(const std::string &arg, const char *name, std::string &val)
    {
        std::string p = std::string("--") + name + "=";
        if (arg.compare(0, p.size(), p) != 0)
            return false;
        val = arg.substr(p.size());
        return true;
    }

    inline void printFlagHelp(const char *prog, const std::string &defCsv)
    {
        std::cerr
            << "Triangulation benchmark flags (for " << prog << "):\n"
            << "  --tri_input=<dir>            exact input folder to scan (default: input)\n"
            << "                               layout: <dir>/<category>/<files>\n"
            << "  --tri_dir_biconnected=<name> sub-folder for Biconnected + WithoutVGS (default: Biconnected)\n"
            << "  --tri_dir_oneconnected=<name> sub-folder for Oneconnected (default: Oneconnected)\n"
            << "  --tri_limits=10,100,...      triangulation limits, each is a separate pass\n"
            << "                               (default: 10,100,1000,10000,100000,1000000)\n"
            << "  --tri_algos=A,B,...          Biconnected,BiconnectedWithoutVGS,Oneconnected (default: all)\n"
            << "  --tri_categories=c1,c2       only these category folders (default: all)\n"
            << "  --tri_csv=<file>             results CSV, also used for resume (default: " << defCsv << ")\n"
            << "  --tri_cpu=<n>                pin to CPU core n (Linux only)\n"
            << "  --tri_help                   this text\n";
    }

    // Consumes the --tri_* flags from argv (compacting it) and leaves the rest.
    inline bool parseConfig(int &argc, char **argv, Config &cfg, const std::string &defaultCsv)
    {
        cfg.csv = defaultCsv;
        int w = 1;
        try
        {
            for (int i = 1; i < argc; i++)
            {
                std::string a = argv[i], v;
                if (a == "--tri_help")
                {
                    printFlagHelp(argv[0], defaultCsv);
                    return false;
                }
                else if (matchFlag(a, "tri_input", v))
                    cfg.inputRoot = v;
                else if (matchFlag(a, "tri_dir_biconnected", v))
                    cfg.dirBiconnected = v;
                else if (matchFlag(a, "tri_dir_oneconnected", v))
                    cfg.dirOneconnected = v;
                else if (matchFlag(a, "tri_csv", v))
                    cfg.csv = v;
                else if (matchFlag(a, "tri_cpu", v))
                    cfg.cpu = std::stoi(v);
                else if (matchFlag(a, "tri_categories", v))
                    cfg.categories = splitList(v);
                else if (matchFlag(a, "tri_limits", v))
                {
                    cfg.limits.clear();
                    for (auto &t : splitList(v))
                    {
                        long long L = std::stoll(t);
                        if (L <= 0)
                            throw std::runtime_error("limits must be > 0");
                        cfg.limits.push_back(L);
                    }
                }
                else if (matchFlag(a, "tri_algos", v))
                {
                    cfg.algos.clear();
                    for (auto &t : splitList(v))
                    {
                        Algo al;
                        if (!parseAlgo(t, al))
                            throw std::runtime_error("unknown algorithm '" + t + "'");
                        cfg.algos.push_back(al);
                    }
                }
                else
                    argv[w++] = argv[i];
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Bad flag value: " << e.what() << "\n";
            printFlagHelp(argv[0], defaultCsv);
            return false;
        }
        argv[w] = nullptr;
        argc = w;
        std::sort(cfg.limits.begin(), cfg.limits.end());
        cfg.limits.erase(std::unique(cfg.limits.begin(), cfg.limits.end()), cfg.limits.end());
        return true;
    }

    inline void pinToCpu(int cpu)
    {
#ifdef __linux__
        if (cpu < 0)
            return;
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu, &set);
        if (sched_setaffinity(0, sizeof(set), &set) != 0)
            std::cerr << "Warning: could not pin to CPU " << cpu << "\n";
#else
        (void)cpu;
#endif
    }

    // -------------------------------------------------------------------- CSV
    inline std::string csvEscape(const std::string &s)
    {
        if (s.find_first_of(",\"\n") == std::string::npos)
            return s;
        std::string o = "\"";
        for (char c : s)
        {
            if (c == '"')
                o += '"';
            o += c;
        }
        return o + "\"";
    }

    inline std::vector<std::string> csvSplit(std::string line)
    {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        std::vector<std::string> out;
        std::string cur;
        bool q = false;
        for (size_t i = 0; i < line.size(); ++i)
        {
            char ch = line[i];
            if (q)
            {
                if (ch == '"')
                {
                    if (i + 1 < line.size() && line[i + 1] == '"')
                    {
                        cur += '"';
                        ++i;
                    }
                    else
                        q = false;
                }
                else
                    cur += ch;
            }
            else if (ch == '"')
                q = true;
            else if (ch == ',')
            {
                out.push_back(cur);
                cur.clear();
            }
            else
                cur += ch;
        }
        out.push_back(cur);
        return out;
    }

    inline void appendCsvRow(const std::string &path, const std::string &header, const std::vector<std::string> &fields)
    {
        std::error_code ec;
        fs::path parent = fs::path(path).parent_path();
        if (!parent.empty() && !fs::create_directories(parent, ec) && ec)
        {
            std::cerr << "Error: cannot create directory for " << path << ": " << ec.message() << "\n";
            return;
        }
        bool needHeader = !fs::exists(path) || fs::file_size(path) == 0;
        std::ofstream out(path, std::ios::app);
        if (!out.is_open())
        {
            std::cerr << "Error: cannot open " << path << " for writing\n";
            return;
        }
        if (needHeader)
            out << header << '\n';
        for (size_t i = 0; i < fields.size(); i++)
            out << (i ? "," : "") << csvEscape(fields[i]);
        out << '\n';
        out.flush(); // row is on disk before the next case starts
    }

    inline std::string csvRow(const std::vector<std::string> &fields)
    {
        std::string row;
        for (size_t i = 0; i < fields.size(); i++)
        {
            if (i)
                row += ',';
            row += csvEscape(fields[i]);
        }
        return row;
    }

    inline std::string fmtD(double v, int prec)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.*f", prec, v);
        return buf;
    }

    inline std::string nowString()
    {
        std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        return buf;
    }

    // ------------------------------------------------------------ case planning
    struct Case
    {
        Algo algo;
        std::string category, file, path;
        long long limit;
    };

    inline std::string caseKey(const std::string &algo, const std::string &cat, const std::string &file, long long limit)
    {
        return algo + "|" + cat + "|" + file + "|" + std::to_string(limit);
    }
    inline std::string caseKey(const Case &c) { return caseKey(algoName(c.algo), c.category, c.file, c.limit); }
    inline std::string caseBaseKey(const std::string &algo, const std::string &cat, const std::string &file)
    {
        return algo + "|" + cat + "|" + file;
    }

    struct CsvResults
    {
        std::string header;
        int algorithmIndex = -1;
        int categoryIndex = -1;
        int filenameIndex = -1;
        int limitIndex = -1;
        int statusIndex = -1;
        int timestampIndex = -1;
        std::set<std::string> done;
        std::map<std::string, std::map<long long, std::vector<std::string>>> completed;
    };

    inline CsvResults loadCsvResults(const std::string &csvPath)
    {
        CsvResults results;
        std::ifstream in(csvPath);
        std::string line;
        if (!in.is_open() || !std::getline(in, line))
            return results;
        results.header = line;
        auto hdr = csvSplit(line);
        auto idx = [&](const char *n)
        {
            for (size_t i = 0; i < hdr.size(); i++)
                if (hdr[i] == n)
                    return (int)i;
            return -1;
        };
        results.algorithmIndex = idx("algorithm");
        results.categoryIndex = idx("category");
        results.filenameIndex = idx("filename");
        results.limitIndex = idx("limit");
        results.statusIndex = idx("status");
        results.timestampIndex = idx("timestamp");
        if (results.algorithmIndex < 0 || results.categoryIndex < 0 || results.filenameIndex < 0 || results.limitIndex < 0)
            return results;
        int need = std::max(std::max(results.algorithmIndex, results.categoryIndex),
                            std::max(results.filenameIndex, results.limitIndex));
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;
            auto f = csvSplit(line);
            if ((int)f.size() <= need)
                continue;
            try
            {
                long long limit = std::stoll(f[results.limitIndex]);
                results.done.insert(caseKey(f[results.algorithmIndex], f[results.categoryIndex],
                                            f[results.filenameIndex], limit));
                if (results.statusIndex >= 0 && results.statusIndex < (int)f.size() && f[results.statusIndex] == "Completed")
                    results.completed[caseBaseKey(f[results.algorithmIndex], f[results.categoryIndex], f[results.filenameIndex])][limit] = f;
            }
            catch (...)
            {
            }
        }
        return results;
    }

    inline std::set<std::string> loadDoneKeys(const std::string &csvPath)
    {
        return loadCsvResults(csvPath).done;
    }

    inline fs::path algoInputDir(const Config &c, Algo a)
    {
        (void)a;
        return fs::path(c.inputRoot);
    }

    inline std::string getCategoryCsvPath(const std::string &baseCsv, Algo algo, const std::string &category)
    {
        fs::path p(baseCsv);
        std::string stem = p.stem().string();
        std::string ext = p.extension().string();
        if (ext.empty())
            ext = ".csv";
        std::string algoFolder = std::string(algoName(algo)) + "Algo";
        return (p.parent_path() / algoFolder / (stem + "_" + category + ext)).string();
    }

    // Order: limit (outermost) -> algorithm -> category -> file.
    // i.e. one complete pass at limit 10, then one complete pass at limit 100...
    // Cases already present in the CSV are dropped here, so they never run twice.
    inline std::vector<Case> planCases(const Config &cfg)
    {
        struct Item
        {
            Algo algo;
            std::string cat, file, path;
        };
        std::vector<Item> items;
        std::map<std::string, CsvResults> resultsPerAlgoCat;
        for (Algo a : cfg.algos)
        {
            fs::path dir = algoInputDir(cfg, a);
            if (!fs::is_directory(dir))
            {
                std::cerr << "Warning: " << dir.string() << " not found, skipping " << algoName(a) << "\n";
                continue;
            }
            std::vector<std::string> cats;
            for (auto &e : fs::directory_iterator(dir))
                if (e.is_directory() && e.path().filename().string()[0] != '.')
                    cats.push_back(e.path().filename().string());
            std::sort(cats.begin(), cats.end());
            for (auto &cat : cats)
            {
                if (!cfg.categories.empty() && std::find(cfg.categories.begin(), cfg.categories.end(), cat) == cfg.categories.end())
                    continue;

                std::string algoCat = std::string(algoName(a)) + "|" + cat;
                if (resultsPerAlgoCat.find(algoCat) == resultsPerAlgoCat.end())
                    resultsPerAlgoCat[algoCat] = loadCsvResults(getCategoryCsvPath(cfg.csv, a, cat));

                std::vector<std::string> files;
                for (auto &e : fs::recursive_directory_iterator(dir / cat))
                    if (e.is_regular_file() && e.path().filename().string()[0] != '.')
                        files.push_back(fs::relative(e.path(), dir / cat).generic_string());
                std::sort(files.begin(), files.end());
                for (auto &f : files)
                    items.push_back({a, cat, f, (dir / cat / f).string()});
            }
        }
        std::vector<Case> cases;
        size_t total = 0, skipped = 0;
        for (long long limit : cfg.limits)
            for (auto &it : items)
            {
                Case c{it.algo, it.cat, it.file, it.path, limit};
                total++;
                std::string algoCat = std::string(algoName(it.algo)) + "|" + it.cat;
                CsvResults &results = resultsPerAlgoCat[algoCat];
                if (results.done.count(caseKey(c)))
                {
                    skipped++;
                    continue;
                }
                std::string base = caseBaseKey(algoName(c.algo), c.category, c.file);
                auto &completed = results.completed[base];
                auto previous = completed.lower_bound(limit);
                if (previous != completed.begin())
                {
                    --previous;
                    std::vector<std::string> copied = previous->second;
                    copied[results.limitIndex] = std::to_string(limit);
                    if (results.statusIndex >= 0 && results.statusIndex < (int)copied.size())
                        copied[results.statusIndex] = "Completed";
                    if (results.timestampIndex >= 0 && results.timestampIndex < (int)copied.size())
                        copied[results.timestampIndex] = nowString();
                    std::string csvPath = getCategoryCsvPath(cfg.csv, c.algo, c.category);
                    appendCsvRow(csvPath, results.header, copied);
                    completed[limit] = copied;
                    results.done.insert(caseKey(c));
                    skipped++;
                    std::cerr << "Copied completed result for " << algoName(c.algo) << "/" << c.category << "/" << c.file
                              << " from limit " << previous->first << " to limit " << limit << " in " << csvPath << "\n";
                    continue;
                }
                cases.push_back(std::move(c));
            }
        std::cerr << "Cases: " << total << " total, " << skipped << " already in CSV (skipped), "
                  << cases.size() << " to run.\n";
        return cases;
    }
} // namespace tb

#include "praxis/frontend/parser.hpp"
#include "praxis/frontend/preprocessor.hpp"
#include "praxis/runtime/interpreter.hpp"
#include "praxis/semantics/analyzer.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

namespace praxis {
Program prepare(const Source &s, Preprocessor &pp, bool script = false, std::set<std::string> names={}) {
    auto p = Parser(pp.run(s),std::move(names)).parse(script);
    p.iostream = pp.iostream;
    p.strings = pp.string;
    return p;
}
int repl() {
    Program history;
    history.script = true;
    Preprocessor pp;
    auto runtime = std::make_unique<Interpreter>(std::cout);
    std::string line, buffer;
    int submission = 0;
    std::cout << "Praxis M2 REPL (:quit, :reset; finish multiline input with an empty line)\n";
    while (std::getline(std::cin, line)) {
        if (buffer.empty() && line == ":quit")
            break;
        if (buffer.empty() && line == ":reset") {
            runtime->finish();
            history = Program{};
            history.script = true;
            pp = Preprocessor{};
            runtime = std::make_unique<Interpreter>(std::cout);
            std::cout << "State reset.\n";
            continue;
        }
        buffer += line + '\n';
        int braces = 0;
        bool quoted = false, escaped = false;
        for (char c : buffer) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (c == '\\') {
                escaped = true;
                continue;
            }
            if (c == '"') {
                quoted = !quoted;
                continue;
            }
            if (!quoted) {
                if (c == '{')
                    ++braces;
                if (c == '}')
                    --braces;
            }
        }
        if (!line.empty() && (braces > 0 || quoted))
            continue;
        if (buffer == "\n") {
            buffer.clear();
            continue;
        }
        try {
            auto candidatePP = pp;
            std::set<std::string> names;for(auto& i:history.items)if(i.record)names.insert(i.record->name.text);
            auto chunk = prepare({"<repl:"+std::to_string(++submission)+">",buffer},candidatePP,true,names);
            auto candidate = history;
            size_t start = candidate.items.size();
            candidate.items.insert(candidate.items.end(), chunk.items.begin(), chunk.items.end());
            candidate.iostream = candidatePP.iostream;
            candidate.strings = candidatePP.string;
            Analyzer{}.check(candidate);
            history = std::move(candidate);
            pp = std::move(candidatePP);
            runtime->run(history, start);
        } catch (const Error &e) {
            std::cerr << diagnostic(e) << '\n';
        }
        buffer.clear();
    }
    runtime->finish();
    if (!buffer.empty()) {
        std::cerr << "<repl>:1:1: parse: incomplete submission\n";
        return 1;
    }
    return 0;
}
} // namespace praxis
int main(int argc, char **argv) {
    using namespace praxis;
    try {
        if (argc == 2 && std::string(argv[1]) == "--version") {
            std::cout << "Praxis 0.1.0+M2 (class/template subset)\n";
            return 0;
        }
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) {
            std::cout << "Usage: praxis --version | --help | repl | run FILE [-I DIR] | check FILE "
                         "[-I DIR] [--profile] [--max-calls=N] [--max-template-depth=N] [--max-instantiations=N] [--max-objects=N] [--max-steps=N]\n";
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "repl")
            return repl();
        if (argc < 3 || (std::string(argv[1]) != "run" && std::string(argv[1]) != "check")) {
            std::cerr << "praxis: invalid command; use --help\n";
            return 2;
        }
        Preprocessor pp;Limits limits;bool profiling=false;
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if(a=="--profile"){profiling=true;continue;}
            if(a.starts_with("--max-")){
                auto eq=a.find('=');if(eq==std::string::npos)fail({"<cli>"},"option","limit requires =NUMBER");auto digits=a.substr(eq+1);
                if(digits.empty()||!std::all_of(digits.begin(),digits.end(),[](char c){return c>='0'&&c<='9';}))fail({"<cli>"},"option","invalid resource limit");
                size_t value=0;try{value=std::stoull(digits);}catch(...){fail({"<cli>"},"option","resource limit out of range");}
                auto key=a.substr(0,eq);size_t maximum=0;
                if(key=="--max-calls"){maximum=256;limits.calls=value;}else if(key=="--max-template-depth"){maximum=64;limits.templateDepth=value;}else if(key=="--max-instantiations"){maximum=4096;limits.instantiations=value;}else if(key=="--max-objects"){maximum=1000000;limits.objects=value;}else if(key=="--max-steps"){maximum=1000000000;limits.steps=value;}else fail({"<cli>"},"option","unknown resource limit");
                if(!value||value>maximum)fail({"<cli>"},"option","resource limit outside supported range");continue;
            }
            if (a == "-I" && i + 1 < argc)
                pp.paths.emplace_back(argv[++i]);
            else if (a.starts_with("-I") && a.size() > 2)
                pp.paths.emplace_back(a.substr(2));
            else {
                std::cerr << "praxis: invalid option: " << a << '\n';
                return 2;
            }
        }
        using Clock=std::chrono::steady_clock;
        auto start=Clock::now();auto tokens=pp.run(SourceManager{}.read(argv[2]));auto preprocessed=Clock::now();
        auto p=Parser(std::move(tokens)).parse();p.iostream=pp.iostream;p.strings=pp.string;auto parsed=Clock::now();
        Analyzer{limits}.check(p);auto analyzed=Clock::now();Interpreter runtime(std::cout,limits);int code=0;
        if(std::string(argv[1])=="run")code=runtime.run(p);auto executed=Clock::now();
        if(profiling){
            auto ms=[](auto a,auto b){return std::chrono::duration<double,std::milli>(b-a).count();};auto stat=runtime.statistics();size_t peak=0;
#ifdef _WIN32
            PROCESS_MEMORY_COUNTERS memory{};memory.cb=sizeof(memory);if(GetProcessMemoryInfo(GetCurrentProcess(),&memory,sizeof(memory)))peak=memory.PeakWorkingSetSize;
#endif
            std::cerr<<"{\"preprocess_ms\":"<<ms(start,preprocessed)<<",\"parse_ms\":"<<ms(preprocessed,parsed)<<",\"semantic_ms\":"<<ms(parsed,analyzed)<<",\"runtime_ms\":"<<ms(analyzed,executed)
                <<",\"template_instantiation_ms_inclusive\":"<<p.templateNanoseconds/1e6<<",\"instantiations\":"<<p.instantiations<<",\"cache_hits\":"<<p.cacheHits
                <<",\"live_objects\":"<<runtime.liveObjectCount()<<",\"objects\":"<<stat.objects<<",\"peak_live_objects\":"<<stat.peakObjects<<",\"constructions\":"<<stat.constructions<<",\"copies\":"<<stat.copies<<",\"assignments\":"<<stat.assignments
                <<",\"calls\":"<<stat.calls<<",\"destructors\":"<<stat.destructors<<",\"dispatch_ms_inclusive\":"<<stat.callNanoseconds/1e6<<",\"construction_ms_inclusive\":"<<stat.constructorNanoseconds/1e6
                <<",\"destruction_ms_inclusive\":"<<stat.destructorNanoseconds/1e6<<",\"peak_working_set_bytes\":"<<peak<<"}\n";
        }
        return code;
    } catch (const Error &e) {
        std::cerr << diagnostic(e) << '\n';
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "praxis: internal/resource error: " << e.what() << '\n';
        return 2;
    }
}

#include <iostream>
#include <random>
#include <array>
#include <map>
#include <regex>
#include <iomanip>
#include <barrier>
#include <thread>
#include <source_location>
#include <chrono>
#include <filesystem>
#include <ranges>
#include <variant>
#include <algorithm>
 #define _GLIBCXX_HAVE_STACKTRACE 1
#include <stacktrace>

import TestModule;

static void verifyRandom_(void) { 
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(1.0, 10.0);

    for (int i=0; i<16; ++i)
        std::cout << "[Random] " << dist(mt) << std::endl;

    std::seed_seq seq1({0xf5e5b5c0, 0xdcb8e4b1}),
                  seq2({0xd34295df, 0xba15c4d0});

    std::array<uint32_t, 2> arr1, arr2;
    seq1.generate(arr1.begin(), arr1.end());
    seq2.generate(arr2.begin(), arr2.end());

    std::map<int, int> hist;
    std::uniform_int_distribution<int> dist2(0, 9);
    for (int n = 0; n < 20000; ++n) {
        ++hist[dist2(rd)]; 
    }

    for (auto p : hist) {
        std::cout << "[Random] " << p.first << " : " << std::string(p.second/100, '*') << std::endl;
    }

}

static void verifyRegex_(void) { 

    const std::string rna = "AGAAUGGCGATCGATCGATCGUAACGAGC" ;
    std::cout << "[Regex] " << "rna: " << std::quoted(rna) << '\n' ;

    // [ACGTU]*? - non-greedy match for string between start/stop
    const std::regex re( "[ACGTU]*(AUG)([ACGTU]*?)(UGA|UAG|UAA)[ACGTU]*" ) ;

    std::smatch match ;
    if( std::regex_match( rna, match, re ) )
    {
        std::cout << "[Regex] " << "start: at " << match.position(1) << " found " << std::quoted( match[1].str() ) << std::endl
                  << "[Regex] " <<"  end: at " << match.position(3) << " found " << std::quoted( match[3].str() ) << std::endl
                  << "[Regex] " <<"sequence between start and stop: " << std::quoted( match[2].str() ) << std::endl;
    }

    else std::cout << "[Regex] invalid rna or no start-stop sequence" << std::endl;
}

static void verifyThreads_(void) { 
	#if 0
 const auto workers = { "Anil", "Busara", "Carl" };
 
    auto on_completion = []() noexcept
    {
        // locking not needed here
        static auto phase = "... done\n" "Cleaning up...\n";
        std::cout << phase;
        phase = "... done\n";
    };
 
    std::barrier sync_point(std::ssize(workers), on_completion);
 
    auto work = [&](std::string name)
    {
        std::string product = "  " + name + " worked\n";
        std::cout << product;  // ok, op<< call is atomic
        sync_point.arrive_and_wait();
 
        product = "  " + name + " cleaned\n";
        std::cout << product;
        sync_point.arrive_and_wait();
    };
 
    std::cout << "Starting...\n";
    std::vector<std::jthread> threads;
    threads.reserve(std::size(workers));
    for (auto const& worker : workers)
        threads.emplace_back(work, worker);
        #endif
}

template<typename T1, typename T2>
using mul = std::ratio_multiply<T1, T2>;
 
void verifyChrono_(void) {
	using namespace std::chrono_literals;
 
    using shakes = std::chrono::duration<int, mul<std::deca, std::nano>>;
    using jiffies = std::chrono::duration<int, std::centi>;
    using microfortnights = std::chrono::duration<float,
        mul<mul<std::ratio<2>, std::chrono::weeks::period>, std::micro>>;
    using nanocenturies = std::chrono::duration<float,
        mul<mul<std::hecto, std::chrono::years::period>, std::nano>>;
    using fps_24 = std::chrono::duration<double, std::ratio<1,24>>;
 
    std::cout << "[Chrono] " << "1 second is:\n";
 
    // integer scale conversion with no precision loss: no cast
    std::cout << "[Chrono] " << std::chrono::microseconds(1s).count() << " microseconds\n"
              << "[Chrono] " << shakes(1s).count() << " shakes\n"
              << "[Chrono] " << jiffies(1s).count() << " jiffies\n";
 
    // integer scale conversion with precision loss: requires a cast
    std::cout << "[Chrono] " << std::chrono::duration_cast<std::chrono::minutes>(1s).count()
              << " minutes\n";
 
    // floating-point scale conversion: no cast
    std::cout << "[Chrono] " << microfortnights(1s).count() << " microfortnights\n"
              << "[Chrono] " << nanocenturies(1s).count() << " nanocenturies\n"
              << "[Chrono] " << fps_24(1s).count() << " frames at 24fps" << std::endl;
}

void verifySourceLocation_(const std::source_location location = std::source_location::current()) { 
    std::clog << "[Source Locaton] " << "file: "
              << location.file_name() << "("
              << location.line() << ":"
              << location.column() << ") `"
              << location.function_name() << "`: "
              << std::endl;
}

void verifyFileSystem_(void) {

	//std::cout << "[FileSystem] Current Path: " << std::filesystem::current_path();
    std::string path = "/pc/";
    for (const auto & entry : std::filesystem::recursive_directory_iterator(path))
        std::cout << "[FileSystem] " << entry.path() << std::endl;
}

void verifyRanges_(void) { 
    std::vector<int> input =  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    auto divisible_by_three = [](const int n) {return n % 3 == 0; };
    auto square = [](const int n) {return n * n; };

    auto x = input | std::views::filter(divisible_by_three)
                   | std::views::transform(square);

    for (int i : x)
    {
        std::cout << "[Ranges] " << i << std::endl;
    }

	namespace rv = std::ranges::views;
	std::vector<int> dt = {1, 2, 3, 4, 5, 6, 7};
	std::cout << "[Ranges] ";
	for (int v : dt | rv::reverse | rv::take(3) | rv::reverse) {
	    std::cout << v << ", ";
	}
	std::cout << std::endl;
}

void verifyVariant_(void) {

	struct SampleVisitor
	{
	    void operator()(int i) const { 
	        std::cout << "[Variant] int: " << i << "\n"; 
	    }
	    void operator()(float f) const { 
	        std::cout << "[Variant] float: " << f << "\n"; 
	    }
	    void operator()(const std::string& s) const { 
	        std::cout << "[Variant] string: " << s << "\n"; 
	    }
	};

    std::variant<int, float, std::string> intFloatString;
    static_assert(std::variant_size_v<decltype(intFloatString)> == 3);

    // default initialized to the first alternative, should be 0
    std::visit(SampleVisitor{}, intFloatString);

    // index will show the currently used 'type'
    std::cout << "[Variant] index = " << intFloatString.index() << std::endl;
    intFloatString = 100.0f;
    std::cout << "[Variant] index = " << intFloatString.index() << std::endl;
    intFloatString = "hello super world";
    std::cout << "[Variant] index = " << intFloatString.index() << std::endl;

    // try with get_if:
    if (const auto intPtr (std::get_if<int>(&intFloatString)); intPtr) 
        std::cout << "[Variant] int!" << *intPtr << "\n";
    else if (const auto floatPtr (std::get_if<float>(&intFloatString)); floatPtr) 
        std::cout << "[Variant] float!" << *floatPtr << "\n";

    if (std::holds_alternative<int>(intFloatString))
        std::cout << "[Variant] the variant holds an int!\n";
    else if (std::holds_alternative<float>(intFloatString))
        std::cout << "[Variant] the variant holds a float\n";
    else if (std::holds_alternative<std::string>(intFloatString))
        std::cout << "[Variant] the variant holds a string\n";  

    // try/catch and bad_variant_access
    try 
    {
        auto f = std::get<float>(intFloatString); 
        std::cout << "[Variant] float! " << f << std::endl;
    }
    catch (std::bad_variant_access&) 
    {
        std::cout << "[Variant] our variant doesn't hold float at this moment..." << std::endl;
    }

    // visit:
    std::visit(SampleVisitor{}, intFloatString);
    intFloatString = 10;
    std::visit(SampleVisitor{}, intFloatString);
    intFloatString = 10.0f;
    std::visit(SampleVisitor{}, intFloatString);
}

void verifyStacktrace_(void) { 
#if 0
    auto trace = std::stacktrace::current();
    for (const auto& entry: trace) {
        std::cout << std::to_string(entry) << '\n';
    }
#endif

}

void verifyAlgorithms_(void) { 

	using namespace std;

    // Initializing vector with array values
    int arr[] = {5, 10, 15, 20, 20, 23, 42, 45};
    int n = sizeof(arr)/sizeof(arr[0]);
    vector<int> vect(arr, arr+n);
 
    cout << "[Algorithms] Given Vector is: ";
    for (int i=0; i<n; i++)
        cout << vect[i] << " ";
 
    // modifies vector to its next permutation order
    next_permutation(vect.begin(), vect.end());
    cout << "\n[Algorithms] Vector after performing next permutation: ";
    for (int i=0; i<n; i++)
        cout << vect[i] << " ";
 
    prev_permutation(vect.begin(), vect.end());
    cout << "\n[Algorithms] Vector after performing prev permutation: ";
    for (int i=0; i<n; i++)
        cout << vect[i] << " ";

    std::cout << std::endl;
 
}

int main(int arg, char* argv[]) {
	verifyModule_(12);
	std::cout << "[Module] Module value: " << module_variable << std::endl;
	verifyRandom_();
	verifyRegex_();
	verifyThreads_();
	verifySourceLocation_();
	verifyChrono_();
	verifyFileSystem_();
	verifyRanges_();
	verifyVariant_();
	verifyStacktrace_();
	verifyAlgorithms_();

	return 0;
}
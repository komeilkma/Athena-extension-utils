
#include "./extract_histogram_tag.h"

#include <regex>

namespace {

// Regular expressions for per-LBN histogram names used over time
std::regex const lbRegex_run2("^/?run_\\d+/lb_(\\d+)(/.+)$");
std::regex const lbRegex_run3("^(.+)_LB((\\d+)|\\d+_(\\d+))$");

}

namespace detail {

std::pair<std::string, int> extract_histogram_tag(const std::string& histo_name)
{
    std::smatch what;
    if (std::regex_match(histo_name, what, ::lbRegex_run2)) {
        try {
            int tag = std::stoi(what.str(1));
            return std::make_pair(what[2], tag);
        } catch(std::exception &ex) {
            // should not happen but be defensive and just ignore it
        }
    }
    if (std::regex_match(histo_name, what, ::lbRegex_run3)) {
        std::string const tagStr = what[3].length() > 0 ? what.str(3) : what.str(4);
        try {
            int tag = std::stoi(tagStr);
            return std::make_pair(what[1], tag);
        } catch(std::exception &ex) {
            // should not happen but be defensive and just ignore it
        }
    }
    return std::make_pair(histo_name, -1);
}

} // namespace detail

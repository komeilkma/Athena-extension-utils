#define BOOST_TEST_MODULE test_extract_histogram_tag
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "../src/extract_histogram_tag.h"


using detail::extract_histogram_tag;


BOOST_AUTO_TEST_CASE(test_stripTag_nolbn)
{
    // test case for when histo name does not have LBN in it

    std::pair<std::string, int> result;

    // path and tag are not set is stripTag() returns false
    result = extract_histogram_tag("name");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/EXPERT/name");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/LB/EXPERT/name");
    BOOST_TEST(result.second == -1);
}

BOOST_AUTO_TEST_CASE(test_stripTag_lbn_run2)
{
    // test case for run2 LBN convention

    std::pair<std::string, int> result;

    result = extract_histogram_tag("/run_0/lb_1/name");
    BOOST_TEST(result.first == "/name");
    BOOST_TEST(result.second == 1);

    result = extract_histogram_tag("/run_100/lb_999/EXPERT/name");
    BOOST_TEST(result.first == "/EXPERT/name");
    BOOST_TEST(result.second == 999);

    result = extract_histogram_tag("/run_999/lb_123456789/LB/EXPERT/name");
    BOOST_TEST(result.first == "/LB/EXPERT/name");
    BOOST_TEST(result.second == 123456789);

    // some non-matching cases
    result = extract_histogram_tag("/run_X/lb_1/LB/EXPERT/name");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/run_1/lb_Y/LB/EXPERT/name");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/run_X/lb_Y/LB/EXPERT/name");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/run_1/lb_-1/LB/EXPERT/name");
    BOOST_TEST(result.second == -1);
}

BOOST_AUTO_TEST_CASE(test_stripTag_lbn_run3)
{
    // test case for run3 LBN convention

    std::pair<std::string, int> result;

    result = extract_histogram_tag("name_LB1");
    BOOST_TEST(result.first == "name");
    BOOST_TEST(result.second == 1);

    result = extract_histogram_tag("name_LB1_10");
    BOOST_TEST(result.first == "name");
    BOOST_TEST(result.second == 10);

    result = extract_histogram_tag("/EXPERT/name_LB999");
    BOOST_TEST(result.first == "/EXPERT/name");
    BOOST_TEST(result.second == 999);

    result = extract_histogram_tag("/LB/EXPERT/name_LB999_1001");
    BOOST_TEST(result.first == "/LB/EXPERT/name");
    BOOST_TEST(result.second == 1001);

    // some non-matching cases
    result = extract_histogram_tag("/LB/EXPERT/name_LBN");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/LB/EXPERT/name_LBN_100");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/LB/EXPERT/name_LB1_N");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/LB/EXPERT/name_LB-1");
    BOOST_TEST(result.second == -1);

    result = extract_histogram_tag("/LB/EXPERT/nameLB1");
    BOOST_TEST(result.second == -1);
}

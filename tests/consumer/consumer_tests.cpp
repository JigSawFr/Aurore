// The installed harness: Aurore::test and brls_add_tests()
#include <borealis/test/harness.hpp>

BRLS_TEST(consumer_links)
{
    BRLS_CHECK(1 + 1 == 2);
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}

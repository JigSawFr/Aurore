// The installed harness: Aurore::test and aurore_add_tests()
#include <aurore/test/harness.hpp>

BRLS_TEST(consumer_links)
{
    BRLS_CHECK(1 + 1 == 2);
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}

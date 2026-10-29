// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// The installed harness: Aurore::test and aurore_add_tests()
#include <aurore/test/harness.hpp>

AURORE_TEST(consumer_links)
{
    AURORE_CHECK(1 + 1 == 2);
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}

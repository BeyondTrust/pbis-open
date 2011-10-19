#include <moonunit/moonunit.h>
#include <wc16printf.h>
#include <wc16str.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

MU_TEST(compare, wc16scasecmp)
{
    wchar16_t str1[] = {'a', 'B', 'c', '\0'};
    wchar16_t str2[] = {'A', 'b', 'C', '\0'};
    wchar16_t str3[] = {'a', 'b', 'd', '\0'};

    MU_ASSERT(wc16scasecmp(str1, str2) == 0);
    MU_ASSERT(wc16scasecmp(str2, str3) < 0);
    MU_ASSERT(wc16scasecmp(str3, str1) > 0);
}

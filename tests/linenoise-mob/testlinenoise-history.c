/*
 * Copyright BeyondTrust Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU LESSER GENERAL
 * PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR
 * WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY
 * BEYONDTRUST, PLEASE CONTACT BEYONDTRUST AT beyondtrust.com/contact
 */

#include <stdlib.h>
#include <unity.h>
#include <linenoise.h>


void setUp() {
    /* clear any existing history */
    linenoiseHistorySetMaxLen(10);
}

void tearDown() {
    linenoiseHistoryFree();
}

void testWithNoHistoryLastEntryReturnsNull() {
    TEST_ASSERT_NULL(linenoiseHistoryLastEntry());
}

void testWithNoHistoryHistoryEntryZeroReturnsNull() {
    TEST_ASSERT_NULL(linenoiseHistoryLastEntry(0));
}

void testWithNoHistoryHistoryEntryOneReturnsNull() {
    TEST_ASSERT_NULL(linenoiseHistoryLastEntry(1));
}

void testWithNoHistoryHistoryMatchingReturnsNull() {
    TEST_ASSERT_NULL(linenoiseHistoryMatching("hello"));
}

void testWithNoHistoryHistoryMatchingWithEmptyStringReturnsNull() {
    TEST_ASSERT_NULL(linenoiseHistoryMatching(""));
}

void testWithHistoryLastEntryReturnsLastEntry() {
    const char *entry = "hello world";
    char *lastEntry = NULL;

    linenoiseHistoryAdd(entry);

    lastEntry = linenoiseHistoryLastEntry();
    TEST_ASSERT_EQUAL_STRING(entry, lastEntry);

    free(lastEntry);
}

void testWithHistoryHistoryEntryReturnsNthEntry() {
    const char *entries[3] = { "first", "second", "third" };
    int i = 0;

    for(i=0; i < 3; i++) {
        linenoiseHistoryAdd(entries[i]);
    }

    for(i=0; i < 3; i++) {
        char *entry = linenoiseHistoryEntry(i);
        TEST_ASSERT_EQUAL_STRING(entries[i], entry);
        free(entry);
    }
}

void testWithHistoryHistoryMatchingReturnsNullWhenNoMatch() {
    char *matchingEntry = NULL;

    linenoiseHistoryAdd("frog");
    linenoiseHistoryAdd("turtle");
    linenoiseHistoryAdd("giraffe");

    matchingEntry = linenoiseHistoryMatching("turp");
    TEST_ASSERT_NULL(matchingEntry);
}

void testWhenHistoryMatchingEntryReturnsLastEntryThatMatches() {
    char *matchingEntry = NULL;

    linenoiseHistoryAdd("frog");
    linenoiseHistoryAdd("freeze");
    linenoiseHistoryAdd("frenzy");

    matchingEntry = linenoiseHistoryMatching("fre");
    TEST_ASSERT_EQUAL_STRING("frenzy", matchingEntry);

    free(matchingEntry);
}

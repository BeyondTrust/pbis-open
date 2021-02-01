/*
 * Copyright © BeyondTrust Software 2004 - 2019
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * BEYONDTRUST MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING TERMS AS
 * WELL. IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT WITH
 * BEYONDTRUST, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE TERMS OF THAT
 * SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE APACHE LICENSE,
 * NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU HAVE QUESTIONS, OR WISH TO REQUEST
 * A COPY OF THE ALTERNATE LICENSING TERMS OFFERED BY BEYONDTRUST, PLEASE CONTACT
 * BEYONDTRUST AT beyondtrust.com/contact
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

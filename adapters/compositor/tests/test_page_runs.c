#include "check.h"
#include "server/page_runs.h"

#define PRESENT (UINT64_C(1) << 63)
#define SWAPPED (UINT64_C(1) << 62)
#define FROM_FILE (UINT64_C(1) << 61)
#define FILE_PAGE (PRESENT | FROM_FILE)
#define OWN_PAGE PRESENT
#define MISSING_PAGE UINT64_C(0)

static void test_written_page_splits_runs(void)
{
    const uint64_t entries[] = {OWN_PAGE, FILE_PAGE, FILE_PAGE, OWN_PAGE, FILE_PAGE};
    page_run run = {0};
    CHECK(page_runs_next(entries, 5, &run));
    CHECK_INT_EQ(1, (int) run.first);
    CHECK_INT_EQ(2, (int) run.count);
    CHECK(page_runs_next(entries, 5, &run));
    CHECK_INT_EQ(4, (int) run.first);
    CHECK_INT_EQ(1, (int) run.count);
    CHECK(!page_runs_next(entries, 5, &run));
}

static void test_missing_pages_between_file_pages_join_the_run(void)
{
    const uint64_t entries[] = {MISSING_PAGE, FILE_PAGE, MISSING_PAGE, MISSING_PAGE,
                                FILE_PAGE,    MISSING_PAGE};
    page_run run = {0};
    CHECK(page_runs_next(entries, 6, &run));
    CHECK_INT_EQ(1, (int) run.first);
    CHECK_INT_EQ(4, (int) run.count);
    CHECK(!page_runs_next(entries, 6, &run));
}

static void test_swapped_page_splits_runs(void)
{
    const uint64_t entries[] = {FILE_PAGE, SWAPPED, FILE_PAGE};
    page_run run = {0};
    CHECK(page_runs_next(entries, 3, &run));
    CHECK_INT_EQ(0, (int) run.first);
    CHECK_INT_EQ(1, (int) run.count);
    CHECK(page_runs_next(entries, 3, &run));
    CHECK_INT_EQ(2, (int) run.first);
    CHECK_INT_EQ(1, (int) run.count);
    CHECK(!page_runs_next(entries, 3, &run));
}

static void test_written_and_missing_pages_alone_make_no_run(void)
{
    const uint64_t entries[] = {OWN_PAGE, MISSING_PAGE, FROM_FILE, SWAPPED | FROM_FILE};
    page_run run = {0};
    CHECK(!page_runs_next(entries, 4, &run));
}

static void test_whole_chunk_can_be_one_run(void)
{
    const uint64_t entries[] = {FILE_PAGE, FILE_PAGE};
    page_run run = {0};
    CHECK(page_runs_next(entries, 2, &run));
    CHECK_INT_EQ(0, (int) run.first);
    CHECK_INT_EQ(2, (int) run.count);
    CHECK(!page_runs_next(entries, 2, &run));
}

static void test_counts_only_file_pages(void)
{
    const uint64_t entries[] = {FILE_PAGE, OWN_PAGE, MISSING_PAGE, FILE_PAGE, SWAPPED | FROM_FILE};
    CHECK_INT_EQ(2, (int) page_runs_file_pages(entries, 5));
    CHECK_INT_EQ(0, (int) page_runs_file_pages(NULL, 0));
}

static void test_empty_chunk_has_no_run(void)
{
    page_run run = {0};
    CHECK(!page_runs_next(NULL, 0, &run));
}

int main(void)
{
    const check_case cases[] = {
        {"written_page_splits_runs", test_written_page_splits_runs},
        {"missing_pages_between_file_pages_join_the_run",
         test_missing_pages_between_file_pages_join_the_run},
        {"swapped_page_splits_runs", test_swapped_page_splits_runs},
        {"written_and_missing_pages_alone_make_no_run",
         test_written_and_missing_pages_alone_make_no_run},
        {"whole_chunk_can_be_one_run", test_whole_chunk_can_be_one_run},
        {"counts_only_file_pages", test_counts_only_file_pages},
        {"empty_chunk_has_no_run", test_empty_chunk_has_no_run},
    };
    return check_run(cases, sizeof(cases) / sizeof(cases[0]));
}

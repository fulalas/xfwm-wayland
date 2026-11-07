#include "check.h"
#include "fake_ports.h"

static const wm_rect small = {0, 0, 400, 300};

static wm_desktop *start(fake_world *world)
{
    fake_world_init(world);
    wm_config config = fake_config();
    return fake_desktop_create(world, &config);
}

static wm_window_view view_of(const wm_window *window)
{
    return wm_window_current_view(window);
}

static wm_window *map_typed(wm_desktop *desktop, int id, wm_window_type type)
{
    wm_window_desc desc = fake_desc(id, small);
    desc.type = type;
    return wm_window_map(desktop, &desc);
}

static wm_window *map_child(wm_desktop *desktop, int id, wm_window *parent)
{
    wm_window_desc desc = fake_desc(id, small);
    desc.type = WM_TYPE_UNSPECIFIED;
    desc.parent = parent;
    return wm_window_map(desktop, &desc);
}

static bool starts_resize(wm_desktop *desktop, wm_window *window)
{
    wm_request_resize(desktop, window, WM_EDGE_RIGHT);
    return view_of(window).resizing;
}

static void test_normal_window_is_in_taskbar(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    CHECK(!view_of(window).skip_taskbar);
    CHECK(!view_of(window).skip_pager);
    CHECK(view_of(window).parent == NULL);
    wm_desktop_destroy(desktop);
}

static void test_dialog_with_parent_skips_taskbar(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *parent = fake_map(desktop, 1, small);
    wm_window *dialog = fake_map_dialog(desktop, 2, parent);
    CHECK(view_of(dialog).skip_taskbar);
    CHECK(view_of(dialog).parent == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_dialog_without_parent_is_in_taskbar(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *dialog = map_typed(desktop, 1, WM_TYPE_DIALOG);
    CHECK(!view_of(dialog).skip_taskbar);
    wm_desktop_destroy(desktop);
}

static void test_helper_types_skip_taskbar(void)
{
    static const wm_window_type skipped[] = {
        WM_TYPE_UTILITY, WM_TYPE_TOOLBAR, WM_TYPE_SPLASH, WM_TYPE_MENU,
        WM_TYPE_NOTIFICATION, WM_TYPE_DOCK, WM_TYPE_DESKTOP,
    };
    fake_world world;
    wm_desktop *desktop = start(&world);
    for (size_t i = 0; i < sizeof(skipped) / sizeof(skipped[0]); i++) {
        wm_window *window = map_typed(desktop, (int) i + 1, skipped[i]);
        CHECK(view_of(window).skip_taskbar);
    }
    wm_desktop_destroy(desktop);
}

static void test_utility_skips_pager(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = map_typed(desktop, 1, WM_TYPE_UTILITY);
    CHECK(view_of(window).skip_pager);
    wm_desktop_destroy(desktop);
}

static void test_explicit_skip_taskbar_is_published(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    int calls = world.window_changed_calls;
    wm_request_state(desktop, window, &(wm_state_change) {WM_STATE_SKIP_TASKBAR, 0});
    CHECK(view_of(window).skip_taskbar);
    CHECK(world.window_changed_calls > calls);
    wm_desktop_destroy(desktop);
}

static void test_untyped_child_becomes_dialog(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *parent = fake_map(desktop, 1, small);
    wm_window *child = map_child(desktop, 2, parent);
    wm_window *alone = map_child(desktop, 3, NULL);
    CHECK(view_of(child).skip_taskbar);
    CHECK(!view_of(alone).skip_taskbar);
    wm_desktop_destroy(desktop);
}

static void test_typed_normal_child_stays_in_taskbar(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *parent = fake_map(desktop, 1, small);
    wm_window_desc desc = fake_desc(2, small);
    desc.parent = parent;
    wm_window *child = wm_window_map(desktop, &desc);
    CHECK(!view_of(child).skip_taskbar);
    CHECK(view_of(child).parent == fake_handle(1));
    wm_desktop_destroy(desktop);
}

static void test_later_parent_is_published(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *parent = fake_map(desktop, 1, small);
    wm_window *child = map_child(desktop, 2, NULL);
    wm_window_set_parent(desktop, child, parent);
    CHECK(view_of(child).parent == fake_handle(1));
    CHECK(view_of(child).skip_taskbar);
    wm_window_set_parent(desktop, parent, child);
    CHECK(view_of(parent).parent == NULL);
    wm_desktop_destroy(desktop);
}

static void test_app_id_change_is_published(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window *window = fake_map(desktop, 1, small);
    int calls = world.window_changed_calls;
    wm_window_set_app_id(desktop, window, "other");
    CHECK_INT_EQ(calls + 1, world.window_changed_calls);
    CHECK(view_of(window).app_id != NULL && view_of(window).app_id[0] == 'o');
    wm_desktop_destroy(desktop);
}

static void test_fixed_size_is_not_resizable(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_size_hints hints = {.min = {400, 300}, .max = {400, 300}};
    wm_window *window = fake_map_with_hints(desktop, small, &hints);
    CHECK(!starts_resize(desktop, window));
    wm_desktop_destroy(desktop);
}

static void test_one_free_dimension_is_resizable(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_size_hints hints = {.min = {400, 300}, .max = {800, 300}};
    wm_window *window = fake_map_with_hints(desktop, small, &hints);
    CHECK(starts_resize(desktop, window));
    wm_desktop_destroy(desktop);
}

static void test_min_above_max_is_not_resizable(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_size_hints hints = {.min = {500, 400}, .max = {300, 200}};
    wm_window *window = fake_map_with_hints(desktop, small, &hints);
    CHECK(!starts_resize(desktop, window));
    wm_desktop_destroy(desktop);
}

static void test_max_only_is_resizable(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_size_hints hints = {.min = {0, 0}, .max = {800, 600}};
    wm_window *window = fake_map_with_hints(desktop, small, &hints);
    CHECK(starts_resize(desktop, window));
    wm_desktop_destroy(desktop);
}

static void test_fixed_size_ignores_initial_maximize(void)
{
    fake_world world;
    wm_desktop *desktop = start(&world);
    wm_window_desc desc = fake_desc(1, small);
    desc.size_hints = (wm_size_hints) {.min = {400, 300}, .max = {400, 300}};
    desc.states = WM_STATE_MAXIMIZED;
    wm_window *window = wm_window_map(desktop, &desc);
    CHECK(!view_of(window).maximized);
    wm_desktop_destroy(desktop);
}

static const check_case CASES[] = {
    {"normal_window_is_in_taskbar", test_normal_window_is_in_taskbar},
    {"dialog_with_parent_skips_taskbar", test_dialog_with_parent_skips_taskbar},
    {"dialog_without_parent_is_in_taskbar", test_dialog_without_parent_is_in_taskbar},
    {"helper_types_skip_taskbar", test_helper_types_skip_taskbar},
    {"utility_skips_pager", test_utility_skips_pager},
    {"explicit_skip_taskbar_is_published", test_explicit_skip_taskbar_is_published},
    {"untyped_child_becomes_dialog", test_untyped_child_becomes_dialog},
    {"typed_normal_child_stays_in_taskbar", test_typed_normal_child_stays_in_taskbar},
    {"later_parent_is_published", test_later_parent_is_published},
    {"app_id_change_is_published", test_app_id_change_is_published},
    {"fixed_size_is_not_resizable", test_fixed_size_is_not_resizable},
    {"one_free_dimension_is_resizable", test_one_free_dimension_is_resizable},
    {"min_above_max_is_not_resizable", test_min_above_max_is_not_resizable},
    {"max_only_is_resizable", test_max_only_is_resizable},
    {"fixed_size_ignores_initial_maximize", test_fixed_size_ignores_initial_maximize},
};

int main(void)
{
    return check_run(CASES, sizeof(CASES) / sizeof(CASES[0]));
}

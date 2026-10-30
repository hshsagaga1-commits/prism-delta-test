// Prism starts with the app. Delta is absent from the Mach-O dependencies.
// This helper's sole responsibility is to activate Delta after a successful game join.
#include <objc/runtime.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <stdatomic.h>

typedef void (*JoinFn)(id, SEL, long long);
static JoinFn original_joined = 0;
static atomic_flag delta_loaded = ATOMIC_FLAG_INIT;

static void load_delta_async(void *unused) {
    (void)unused;
    if (atomic_flag_test_and_set(&delta_loaded)) return;
    void *lib = dlopen("@executable_path/libgloop.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!lib) atomic_flag_clear(&delta_loaded);
}
static void joined_game(id self, SEL sel, long long place_id) {
    if (original_joined) original_joined(self, sel, place_id);
    if (place_id > 0) dispatch_async_f(dispatch_get_main_queue(), NULL, load_delta_async);
}
__attribute__((constructor))
static void set_join_hook(void) {
    Class c = objc_getClass("RBGameController");
    SEL sel = sel_registerName("reportGameJoinedWithPlaceID:");
    if (!c) return;
    Method m = class_getInstanceMethod(c, sel);
    if (!m) m = class_getClassMethod(c, sel);
    if (m) original_joined = (JoinFn)method_setImplementation(m, (IMP)joined_game);
}

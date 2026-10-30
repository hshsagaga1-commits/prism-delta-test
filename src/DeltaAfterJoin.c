// Delta stays completely unloaded in menus; Prism is loaded normally at app startup.
// This helper is loaded at app startup, but it does NOT load Delta until the app
// reports a successful transition into an experience.
#include <objc/runtime.h>
#include <dlfcn.h>
#include <stdatomic.h>

typedef void (*JoinedFn)(id, SEL, long long);
static JoinedFn original_joined = 0;
static atomic_flag did_load = ATOMIC_FLAG_INIT;

static void on_joined(id self, SEL selector, long long placeId) {
    if (original_joined) original_joined(self, selector, placeId);
    if (placeId <= 0) return;
    if (!atomic_flag_test_and_set_explicit(&did_load, memory_order_relaxed)) {
        dlopen("@executable_path/libgloop.dylib", RTLD_NOW | RTLD_LOCAL);
    }
}

__attribute__((constructor))
static void install_join_hook(void) {
    Class controller = objc_getClass("RBGameController");
    SEL selector = sel_registerName("reportGameJoinedWithPlaceID:");
    if (!controller || !selector) return;
    Method method = class_getInstanceMethod(controller, selector);
    if (!method) return;
    original_joined = (JoinedFn)method_setImplementation(method, (IMP)on_joined);
}

#include "amulet.h"

static am_action *first = NULL;
static am_action *last = NULL;

static uint64_t action_seq = 0;

am_action::am_action() {
    gnext = NULL;
    gprev = NULL;
    nnext = NULL;
    am_action::node = NULL;
    tag_ref = LUA_NOREF;
    func_ref = LUA_NOREF;
    action_ref = LUA_NOREF;
    priority = am_conf_default_action_priority;
    seq = action_seq++;
    paused = false;
}

void am_schedule_action(am_action *action) {
    if (first == NULL) {
        assert(last == NULL);
        first = action;
        last = action;
    } else {
        am_action *ptr = last;
        while (ptr != NULL) {
            if (action->priority > ptr->priority || ((action->priority == ptr->priority) && (action->seq > ptr->seq))) {
                // action should go after ptr
                action->gnext = ptr->gnext;
                if (ptr->gnext != NULL) {
                    ptr->gnext->gprev = action;
                } else {
                    assert(ptr == last);
                    last = action;
                }
                action->gprev = ptr;
                ptr->gnext = action;
                return;
            }
            ptr = ptr->gprev;
        }
        // action should go at the front
        action->gnext = first;
        assert(first->gprev == NULL);
        first->gprev = action;
        first = action;
    }
}

void am_deschedule_action(am_action *action) {
    if (action->gnext != NULL) {
        action->gnext->gprev = action->gprev;
    } else {
        assert(action == last);
        last = action->gprev;
    }
    if (action->gprev != NULL) {
        action->gprev->gnext = action->gnext;
    } else {
        assert(action == first);
        first = action->gnext;
    }
    action->gnext = NULL;
    action->gprev = NULL;
}

static inline bool action_is_scheduled(am_action *action) {
    if (action->gnext != NULL || action->gprev != NULL) return true;
    if (first == action) {
        assert(last == action);
        return true;
    }
    return false;
}

void am_execute_actions(lua_State *L) {
    am_action *action = first;
    while (action != NULL) {
        assert(action_is_scheduled(action));
        if (action->paused) {
            action = action->gnext;
            continue;
        }
        am_action *next = action->gnext;
        am_scene_node *node = action->node;
        action->push(L);                            // push action so not gc'd if descheduled when run
        node->pushref(L, action->func_ref);         // push action function
        node->push(L);
        lua_call(L, 1, 1);                          // run action function (pops node, function)
        if (!lua_toboolean(L, -1)) {
            // action finished, remove it.

            lua_pop(L, 1); // pop nil/false

            // remove action from schedule (if not already descheduled)
            if (action_is_scheduled(action)) {
                next = action->gnext; // update next in case new action inserted directly after this one
                am_deschedule_action(action);
            }

            // remove action from node
            if (node->action_list == action) {
                node->action_list = action->nnext;
            } else {
                am_action *ptr = node->action_list;
                while (ptr->nnext != action && ptr != NULL) ptr = ptr->nnext;
                if (ptr != NULL) { 
                    ptr->nnext = action->nnext;
                    action->nnext = NULL;
                } else {
                    // action was cancelled
                    assert(action->nnext == NULL);
                    assert(action->action_ref == LUA_NOREF);
                    assert(action->func_ref == LUA_NOREF);
                    assert(action->tag_ref == LUA_NOREF);
                    goto cancelled;
                }
            }
            am_delete_ref(L, -1, action->action_ref);
            am_delete_ref(L, -1, action->func_ref);
            am_delete_ref(L, -1, action->tag_ref);

            cancelled:
            lua_pop(L, 1); // pop action
            action = next;
            continue;
        }
        if (action_is_scheduled(action)) {
            next = action->gnext; // update next in case new action inserted directly after this one
        }
        lua_pop(L, 2);                              // pop return value, action
        action = next;
    }
}

void am_init_actions() {
    first = NULL;
    last = NULL;
    action_seq = 0;
}

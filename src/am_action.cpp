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
            if (action->priority < ptr->priority || (action->priority == ptr->priority && action->seq < ptr->seq)) {
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

void am_execute_actions(lua_State *L) {
    am_action *action = first;
    while (action != NULL) {
        if (action->paused) {
            action = action->gnext;
            continue;
        }
        am_node *node = action->node;
        lua_unsafe_pushuserdata(L, node);           // push node
        am_push_ref(L, -1, action->func_ref);       // push action function
        lua_pushvalue(L, -2);                       // push node
        lua_call(L, 1, 1);                          // run action function (pops node, function)
        if (lua_isnil(L, -1)) {
            // action finished, remove it.
            lua_pop(L, 1); // pop nil
            am_action *next = action->gnext;
            // remove action from schedule
            am_deschedule_action(action);
            // remove action from node
            if (node->action_list == action) {
                node->action_list = action->nnext;
            } else {
                am_action *ptr = node->action_list;
                while (ptr->nnext != action) ptr = ptr->nnext;
                ptr->nnext = action->nnext;
            }
            am_delete_ref(L, -1, action->action_ref);
            lua_pop(L, 1); // pop node
            action = next;
            continue;
        }
        lua_pop(L, 2);                              // pop return value, node
        action = action->gnext;
    }
}

void am_init_actions() {
    first = NULL;
    last = NULL;
    action_seq = 0;
}

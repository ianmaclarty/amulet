#include "amulet.h"

void am_node::render(am_render_state *rstate) {
    if (recursion_limit <= 0) return;
    recursion_limit--;
    int ticket = rstate->trail.get_ticket();
    am_command **ptr = commands;
    while (*ptr != NULL) {
        (*ptr)->execute(rstate);
        ptr++;
    }
    rstate->trail.rewind_to(ticket);
    recursion_limit++;
}

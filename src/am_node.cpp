#include "amulet.h"

void am_node::render(am_render_state *rstate) {
    if (recursion_limit <= 0) return;
    recursion_limit--;
    int ticket = rstate->trail.get_ticket();
    am_command *cmd = command_list->first();
    while (cmd != NULL) {
        cmd->execute(rstate);
        cmd = cmd->next();
    }
    rstate->trail.rewind_to(ticket);
    recursion_limit++;
}
